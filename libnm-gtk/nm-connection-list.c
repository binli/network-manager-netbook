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

#include "nm-connection-list.h"

G_DEFINE_TYPE (NMConnectionList, nm_connection_list, GTK_TYPE_LIST_STORE)

enum {
    PROP_0,
    PROP_CLIENT,

    LAST_PROP
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), NM_TYPE_CONNECTION_LIST, NMConnectionListPrivate))

typedef struct {
    NMClient *client;
    GSList *settings;

    gboolean disposed;
} NMConnectionListPrivate;

GtkTreeModel *
nm_connection_list_new (NMClient *client)
{
    g_return_val_if_fail (NM_IS_CLIENT (client), NULL);

    return (GtkTreeModel *) g_object_new (NM_TYPE_CONNECTION_LIST,
                                          NM_CONNECTION_LIST_CLIENT, client,
                                          NULL);
}

/* Connection handling */

static void
connection_updated (NMSettingsConnectionInterface *connection,
                    GHashTable *new_settings,
                    gpointer data)
{
}

static void
connection_removed (NMSettingsConnectionInterface *connection,
                    gpointer data)
{
    g_signal_handlers_disconnect_matched (connection, G_SIGNAL_MATCH_DATA,
                                          0, 0, NULL, NULL, data);
}

static void
connection_added (NMSettingsInterface *settings,
                  NMSettingsConnectionInterface *connection,
                  gpointer data)
{
    g_signal_connect (connection, "updated", G_CALLBACK (connection_updated), data);
    g_signal_connect (connection, "removed", G_CALLBACK (connection_removed), data);
}

static void
connection_handling_cleanup (NMConnectionList *self)
{
    NMConnectionListPrivate *priv = GET_PRIVATE (self);
    GSList *iter;

    for (iter = priv->settings; iter; iter = iter->next) {
        NMSettingsInterface *settings = NM_SETTINGS_INTERFACE (iter->data);
        GSList *connections;
        GSList *connection_iter;

        connections = nm_settings_interface_list_connections (settings);
        for (connection_iter = connections; connection_iter; connection_iter = connection_iter->next)
            connection_removed (NM_SETTINGS_CONNECTION_INTERFACE (connection_iter->data), self);

        g_slist_free (connections);

        g_signal_handlers_disconnect_matched (settings, G_SIGNAL_MATCH_DATA,
                                              0, 0, NULL, NULL, self);
        g_object_unref (settings);
    }

    g_slist_free (priv->settings);
}

/* Device handling */

static void
device_state_changed (NMDevice *device,
                      NMDeviceState new_state,
                      NMDeviceState old_state,
                      NMDeviceStateReason reason)
{
}

static void
device_added (NMClient *client,
              NMDevice *device,
              gpointer user_data)
{
    g_signal_connect (device, "state-changed", G_CALLBACK (device_state_changed), user_data);
}

static void
device_removed (NMClient *client,
                NMDevice *device,
                gpointer user_data)
{
    g_signal_handlers_disconnect_matched (device, G_SIGNAL_MATCH_DATA,
                                          0, 0, NULL, NULL, user_data);
}

static void
device_handling_setup (NMConnectionList *self)
{
    NMConnectionListPrivate *priv = GET_PRIVATE (self);
    const GPtrArray *devices;
    int i;

    g_signal_connect (priv->client, "device-added", G_CALLBACK (device_added), self);
    g_signal_connect (priv->client, "device-removed", G_CALLBACK (device_removed), self);

    devices = nm_client_get_devices (priv->client);
    for (i = 0; devices && devices->len > i; i++)
        device_added (priv->client, NM_DEVICE (g_ptr_array_index (devices, i)), self);
}

static void
device_handling_cleanup (NMConnectionList *self)
{
    NMConnectionListPrivate *priv = GET_PRIVATE (self);
    const GPtrArray *devices;
    int i;

    g_signal_handlers_disconnect_matched (priv->client, G_SIGNAL_MATCH_DATA,
                                          0, 0, NULL, NULL, self);

    devices = nm_client_get_devices (priv->client);
    for (i = 0; devices && devices->len > i; i++)
        device_removed (priv->client, NM_DEVICE (g_ptr_array_index (devices, i)), self);
}

void
nm_connection_list_add_settings (NMConnectionList *self,
                                 NMSettingsInterface *settings)
{
    NMConnectionListPrivate *priv;
    GSList *connections;
    GSList *iter;

    g_return_if_fail (NM_IS_CONNECTION_LIST (self));
    g_return_if_fail (NM_IS_SETTINGS_INTERFACE (settings));

    priv = GET_PRIVATE (self);

    if (priv->settings == NULL)
        /* First setting, set up device handling */
        device_handling_setup (self);

    priv->settings = g_slist_prepend (priv->settings, g_object_ref (settings));
    g_signal_connect (settings, "new-connection", G_CALLBACK (connection_added), self);

    connections = nm_settings_interface_list_connections (settings);
    for (iter = connections; iter; iter = iter->next)
        connection_added (settings, NM_SETTINGS_CONNECTION_INTERFACE (iter->data), self);

    g_slist_free (connections);
}

/*****************************************************************************/

static void
nm_connection_list_init (NMConnectionList *self)
{
}

static void
set_property (GObject *object, guint prop_id,
              const GValue *value, GParamSpec *pspec)
{
    NMConnectionListPrivate *priv = GET_PRIVATE (object);

    switch (prop_id) {
    case PROP_CLIENT:
        /* Construct only */
        priv->client = g_value_dup_object (value);
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
    NMConnectionListPrivate *priv = GET_PRIVATE (object);

    switch (prop_id) {
    case PROP_CLIENT:
        g_value_set_object (value, priv->client);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
dispose (GObject *object)
{
    NMConnectionList *self = NM_CONNECTION_LIST (object);
    NMConnectionListPrivate *priv = GET_PRIVATE (self);

    if (!priv->disposed) {
        connection_handling_cleanup (self);
        device_handling_cleanup (self);

        if (priv->client)
            g_object_unref (priv->client);

        priv->disposed = TRUE;
    }

    G_OBJECT_CLASS (nm_connection_list_parent_class)->dispose (object);
}

static void
nm_connection_list_class_init (NMConnectionListClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (NMConnectionListPrivate));

    object_class->set_property = set_property;
    object_class->get_property = get_property;
    object_class->dispose = dispose;

    /* Properties */
    g_object_class_install_property
        (object_class, PROP_CLIENT,
         g_param_spec_object (NM_CONNECTION_LIST_CLIENT,
                              "NMClient",
                              "NMClient",
                              NM_TYPE_CLIENT,
                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}
