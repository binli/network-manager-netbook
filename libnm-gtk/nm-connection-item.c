/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * (C) Copyright 2009 Novell, Inc.
 */

#include <string.h>
#include <glib/gi18n.h>
#include <nm-settings-interface.h>
#include <nm-setting-connection.h>
#include <nm-settings-connection-interface.h>
#include <nm-active-connection.h>
#include <nm-utils.h>
#include "nm-connection-item.h"
#include "gconf-helpers.h"

G_DEFINE_TYPE (NMConnectionItem, nm_connection_item, NM_TYPE_LIST_ITEM)

enum {
    PROP_0,
    PROP_CLIENT,
    PROP_CONNECTION,
    PROP_DELETE_ALLOWED,

    LAST_PROP
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), NM_TYPE_CONNECTION_ITEM, NMConnectionItemPrivate))

typedef struct {
    NMClient *client;
    NMSettingsConnectionInterface *connection;
    NMActiveConnection *ac;
    gboolean delete_allowed;
    gboolean connect_pending;

    gulong secrets_requested_id;
    gulong removed_id;
    gulong updated_id;
    gulong acs_changed_id;
    gulong ac_state_changed_id;

    gboolean disposed;
} NMConnectionItemPrivate;

NMClient *
nm_connection_item_get_client (NMConnectionItem *self)
{
    g_return_val_if_fail (NM_IS_CONNECTION_ITEM (self), NULL);

    return GET_PRIVATE (self)->client;
}

NMSettingsConnectionInterface *
nm_connection_item_get_connection (NMConnectionItem *self)
{
    g_return_val_if_fail (NM_IS_CONNECTION_ITEM (self), NULL);

    return GET_PRIVATE (self)->connection;
}

static void
secrets_requested (NMGConfConnection *connection,
                   const char *setting_name,
                   const char **hints,
                   gboolean ask_user,
                   NMNewSecretsRequestedFunc callback,
                   gpointer callback_data,
                   gpointer user_data)
{
    NMConnectionItem *self = NM_CONNECTION_ITEM (user_data);

    if (NM_CONNECTION_ITEM_GET_CLASS (self)->secrets_requested)
        NM_CONNECTION_ITEM_GET_CLASS (self)->secrets_requested (self,
                                                                (NMConnection *) connection,
                                                                setting_name, hints,
                                                                ask_user, callback, callback_data);
    else {
        GError *error;

        error = g_error_new_literal (NM_SETTINGS_INTERFACE_ERROR,
                                     NM_SETTINGS_INTERFACE_ERROR_INTERNAL_ERROR,
                                     "No secrets providers registered");

        callback (NM_SETTINGS_CONNECTION_INTERFACE (connection), NULL, error, callback_data);
        g_error_free (error);
    }
}

static void
connection_removed (NMSettingsConnectionInterface *connection,
                    gpointer data)
{
    nm_connection_item_set_connection (NM_CONNECTION_ITEM (data), NULL);
}

static gboolean
connection_update_id (NMConnectionItem *self)
{
    NMConnection *connection;
    const char *id;

    connection = (NMConnection *) nm_connection_item_get_connection (self);
    if (connection) {
        NMSettingConnection *s_con;

        s_con = (NMSettingConnection *) nm_connection_get_setting (connection, NM_TYPE_SETTING_CONNECTION);
        id = nm_setting_connection_get_id (s_con);
    } else
        id = NULL;

    g_object_set (self, NM_LIST_ITEM_NAME, id, NULL);

    return FALSE;
}

static void
connection_updated (NMSettingsConnectionInterface *connection,
                    GHashTable *new_settings,
                    gpointer data)
{
    /* Delay the update: This callback gets called as a response to DBus signal handler
       and changing the connection item here will much likely initiate some DBus calls
       as well, causing reentrance issues. */
    g_idle_add ((GSourceFunc) connection_update_id, data);
}

static void
ac_state_changed (NMActiveConnection *ac,
                  GParamSpec *pspec,
                  gpointer user_data)
{
    NMListItem *item = NM_LIST_ITEM (user_data);
    NMListItemStatus status;

    if (ac) {
        switch (nm_active_connection_get_state (ac)) {
        case NM_ACTIVE_CONNECTION_STATE_ACTIVATED:
            status = NM_LIST_ITEM_STATUS_CONNECTED;
            break;
        case NM_ACTIVE_CONNECTION_STATE_ACTIVATING:
            status = NM_LIST_ITEM_STATUS_CONNECTING;
            break;
        default:
            status = NM_LIST_ITEM_STATUS_DISCONNECTED;
            break;
        }
    } else
        status = NM_LIST_ITEM_STATUS_DISCONNECTED;

    if (status != nm_list_item_get_status (item))
        g_object_set (G_OBJECT (item), NM_LIST_ITEM_STATUS, status, NULL);
}

static void
set_active_connection (NMConnectionItem *self,
                       NMActiveConnection *ac)
{
    NMConnectionItemPrivate *priv = GET_PRIVATE (self);

    if (priv->ac) {
        g_signal_handler_disconnect (priv->ac, priv->ac_state_changed_id);
        g_object_unref (priv->ac);
    }

    if (ac) {
        priv->ac = g_object_ref (ac);
        priv->ac_state_changed_id = g_signal_connect (priv->ac, "notify::" NM_ACTIVE_CONNECTION_STATE,
                                                      G_CALLBACK (ac_state_changed), self);
    } else {
        priv->ac_state_changed_id = 0;
        priv->ac = NULL;
    }

    ac_state_changed (ac, NULL, self);
}

static void
active_connections_changed (NMClient *client,
                            GParamSpec *pspec,
                            gpointer user_data)
{
    NMConnectionItem *self = NM_CONNECTION_ITEM (user_data);
    NMConnectionItemPrivate *priv = GET_PRIVATE (self);
    NMConnection *connection;
    const GPtrArray *acs;
    const char *path;
    NMActiveConnection *ac = NULL;
    NMConnectionScope scope;
    int i;

    if (!priv->connection)
        return;

    connection = NM_CONNECTION (nm_connection_item_get_connection (self));
    path = nm_connection_get_path (connection);
    scope = nm_connection_get_scope (connection);

    acs = nm_client_get_active_connections (client);
    for (i = 0; acs && i < acs->len; i++) {
        ac = g_ptr_array_index (acs, i);

        if (scope == nm_active_connection_get_scope (ac) && !strcmp (path, nm_active_connection_get_connection (ac)))
            break;

        ac = NULL;
    }

    set_active_connection (self, ac);
}

static gboolean
idle_connect (gpointer data)
{
    nm_list_item_connect (NM_LIST_ITEM (data));

    return FALSE;
}

void
nm_connection_item_set_connection (NMConnectionItem *self,
                                   NMSettingsConnectionInterface *connection)
{
    NMConnectionItemPrivate *priv;
    gboolean show_delete;

    g_return_if_fail (NM_IS_CONNECTION_ITEM (self));

    priv = GET_PRIVATE (self);

    if (priv->connection) {
        set_active_connection (self, NULL);
        g_signal_handler_disconnect (priv->connection, priv->secrets_requested_id);
        g_signal_handler_disconnect (priv->connection, priv->removed_id);
        g_signal_handler_disconnect (priv->connection, priv->updated_id);
        g_signal_handler_disconnect (priv->client, priv->acs_changed_id);
        g_object_unref (priv->connection);
    }

    if (connection) {
        priv->connection = g_object_ref (connection);

        if (NM_IS_GCONF_CONNECTION (connection))
            priv->secrets_requested_id = g_signal_connect (connection,
                                                           "new-secrets-requested",
                                                           G_CALLBACK (secrets_requested),
                                                           self);

        priv->removed_id = g_signal_connect (connection, "removed", G_CALLBACK (connection_removed), self);
        priv->updated_id = g_signal_connect (connection, "updated", G_CALLBACK (connection_updated), self);
        connection_update_id (self);

        priv->acs_changed_id = g_signal_connect (priv->client, "notify::" NM_CLIENT_ACTIVE_CONNECTIONS,
                                                 G_CALLBACK (active_connections_changed), self);
        active_connections_changed (priv->client, NULL, self);
    } else {
        priv->secrets_requested_id = 0;
        priv->removed_id = 0;
        priv->updated_id = 0;
        priv->acs_changed_id = 0;
        priv->connection = NULL;
    }

    g_object_notify (G_OBJECT (self), NM_CONNECTION_ITEM_CONNECTION);

    if (priv->delete_allowed && connection != NULL)
        show_delete = TRUE;
    else
        show_delete = FALSE;

    g_object_set (G_OBJECT (self), NM_LIST_ITEM_SHOW_DELETE, show_delete, NULL);

    if (priv->connect_pending) {
        priv->connect_pending = FALSE;

        if (connection)
            idle_connect (self);
    }
}

void
nm_connection_item_new_connection (NMConnectionItem *self,
                                   NMConnection *connection,
                                   gboolean connect)
{
    g_return_if_fail (NM_IS_CONNECTION_ITEM (self));
    g_return_if_fail (NM_IS_CONNECTION (connection));

    if (connect)
        GET_PRIVATE (self)->connect_pending = TRUE;

    nm_gconf_write_connection (connection, NULL, NULL);
}

static void
create_connection_secrets_received (NMSettingsConnectionInterface *connection,
                                    GHashTable *settings,
                                    GError *error,
                                    gpointer user_data)
{
    GSimpleAsyncResult *result = user_data;

    if (error)
        g_simple_async_result_set_from_error (result, error);
    else {
        /* Create a duplicate of the connection - otherwise we'll loose the secrets as soon as
           this function returns */
        g_simple_async_result_set_op_res_gpointer (result,
                                                   nm_connection_duplicate (NM_CONNECTION (connection)),
                                                   g_object_unref);

        g_object_unref (connection);
    }

    g_simple_async_result_complete_in_idle (result);
    g_object_unref (result);
}

void
nm_connection_item_create_connection (NMConnectionItem *self,
                                      GAsyncReadyCallback callback,
                                      gpointer user_data)
{
    NMConnection *connection;
    GSimpleAsyncResult *result;

    g_return_if_fail (NM_IS_CONNECTION_ITEM (self));

    if (NM_CONNECTION_ITEM_GET_CLASS (self)->create_connection)
        connection = NM_CONNECTION_ITEM_GET_CLASS (self)->create_connection (self);
    else
        connection = NULL;

    if (connection) {
        const char *setting_name;
        GPtrArray *hints_array = NULL;

        result = g_simple_async_result_new (G_OBJECT (self), callback, user_data,
                                            nm_connection_item_create_connection_finish);

        setting_name = nm_connection_need_secrets (connection, &hints_array);
        if (hints_array)
            /* Hints are for weaks, bah. */
            g_ptr_array_free (hints_array, TRUE);

        if (setting_name)
            secrets_requested ((NMGConfConnection *) connection, setting_name, NULL, TRUE,
                               create_connection_secrets_received, result, self);
        else {
            /* No extra secrets needed, we're done */
            g_simple_async_result_set_op_res_gpointer (result, connection, g_object_unref);
            g_simple_async_result_complete_in_idle (result);
        }
    } else {
        result = g_simple_async_result_new_error (G_OBJECT (self), callback, user_data,
                                                  0, 0, "%s", "Could not create the new connection");
        g_simple_async_result_complete_in_idle (result);
    }
}

NMConnection *
nm_connection_item_create_connection_finish (NMConnectionItem *self,
                                             GAsyncResult *result,
                                             GError **error)
{
    NMConnection *connection;

    g_return_val_if_fail (NM_IS_CONNECTION_ITEM (self), NULL);

    if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result), error) ||
        !g_simple_async_result_is_valid (result, G_OBJECT (self), nm_connection_item_create_connection_finish))
        return NULL;

    connection = (NMConnection *) g_simple_async_result_get_op_res_gpointer (G_SIMPLE_ASYNC_RESULT (result));
    if (connection)
        g_object_ref (connection);

    return connection;
}

static void
delete_cb (NMSettingsConnectionInterface *connection,
           GError *error,
           gpointer user_data)
{
    if (error) {
        char *msg;

        msg = g_strdup_printf (_("Could not delete connection: %s"), error->message);
        nm_list_item_warning (NM_LIST_ITEM (user_data), msg);
        g_free (msg);
    }
}

static void
do_delete (NMListItem *item)
{
    NMConnectionItemPrivate *priv = GET_PRIVATE (item);

    if (priv->connection && priv->delete_allowed)
        nm_settings_connection_interface_delete (priv->connection, delete_cb, item);
}

static int
priority (NMListItem *item)
{
    NMConnectionItemPrivate *priv = GET_PRIVATE (item);
    int priority = 0;

    if (priv->ac && nm_active_connection_get_state (priv->ac) == NM_ACTIVE_CONNECTION_STATE_ACTIVATED) {
        if (nm_active_connection_get_default (priv->ac))
            priority += NM_LIST_ITEM_PRIORITY_DEFAULT_ROUTE;

        priority += NM_LIST_ITEM_PRIORITY_ACTIVATED;
    }

    if (priv->connection)
        priority += NM_LIST_ITEM_PRIORITY_CONFIGURED;

    priority += NM_LIST_ITEM_CLASS (nm_connection_item_parent_class)->priority (item);

    return priority;
}

static NMConnection *
create_connection (NMConnectionItem *self)
{
    NMConnection *connection;
    NMSetting *setting;
    char *id;

    connection = nm_connection_new ();

    setting = nm_setting_connection_new ();
    id = nm_utils_uuid_generate ();
    g_object_set (setting, NM_SETTING_CONNECTION_UUID, id, NULL);
    g_free (id);

    nm_connection_add_setting (connection, setting);

    return connection;
}

/*****************************************************************************/

static void
nm_connection_item_init (NMConnectionItem *self)
{
    NMConnectionItemPrivate *priv = GET_PRIVATE (self);

    priv->delete_allowed = TRUE;
}

static void
set_property (GObject *object, guint prop_id,
              const GValue *value, GParamSpec *pspec)
{
    NMConnectionItemPrivate *priv = GET_PRIVATE (object);

    switch (prop_id) {
    case PROP_CLIENT:
        /* Construct only */
        priv->client = g_value_dup_object (value);
        break;
    case PROP_CONNECTION:
        nm_connection_item_set_connection (NM_CONNECTION_ITEM (object), g_value_get_object (value));
        break;
    case PROP_DELETE_ALLOWED:
        priv->delete_allowed = g_value_get_boolean (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
get_property (GObject *object, guint prop_id,
              GValue *value, GParamSpec *pspec)
{
    NMConnectionItem *self = NM_CONNECTION_ITEM (object);

    switch (prop_id) {
    case PROP_CLIENT:
        g_value_set_object (value, GET_PRIVATE (self)->client);
        break;
    case PROP_CONNECTION:
        g_value_set_object (value, nm_connection_item_get_connection (self));
        break;
    case PROP_DELETE_ALLOWED:
        g_value_set_boolean (value, GET_PRIVATE (self)->delete_allowed);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
dispose (GObject *object)
{
    NMConnectionItem *self = NM_CONNECTION_ITEM (object);
    NMConnectionItemPrivate *priv = GET_PRIVATE (self);

    if (!priv->disposed) {
        nm_connection_item_set_connection (self, NULL);
        g_object_unref (priv->client);
        priv->disposed = TRUE;
    }

    G_OBJECT_CLASS (nm_connection_item_parent_class)->dispose (object);
}

static void
nm_connection_item_class_init (NMConnectionItemClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    NMListItemClass *list_class = NM_LIST_ITEM_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (NMConnectionItemPrivate));

    object_class->set_property = set_property;
    object_class->get_property = get_property;
    object_class->dispose = dispose;

    list_class->delete = do_delete;
    list_class->priority = priority;

    klass->create_connection = create_connection;

    /* Properties */
    g_object_class_install_property
        (object_class, PROP_CLIENT,
         g_param_spec_object (NM_CONNECTION_ITEM_CLIENT,
                              "NMClient",
                              "NMClient",
                              NM_TYPE_CLIENT,
                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    g_object_class_install_property
        (object_class, PROP_CONNECTION,
         g_param_spec_object (NM_CONNECTION_ITEM_CONNECTION,
                              "Connection",
                              "Connection",
                              NM_TYPE_SETTINGS_CONNECTION_INTERFACE,
                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

    g_object_class_install_property
        (object_class, PROP_DELETE_ALLOWED,
         g_param_spec_boolean (NM_CONNECTION_ITEM_DELETE_ALLOWED,
                               "delete allowed",
                               "delete allowed",
                               TRUE,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
}
