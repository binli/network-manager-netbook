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

#include <nm-remote-settings-system.h>
#include "nmn-model.h"
#include "nm-list-model.h"
#include "nm-gconf-settings.h"
#include "nm-list-item.h"
#include "nm-ethernet-item.h"
#include "nm-wifi-item.h"
#include "nm-gsm-item.h"
#include "nm-gsm-pin-request-item.h"
#include "nm-cdma-item.h"
#include "nm-bt-item.h"

G_DEFINE_TYPE (NmnModel, nmn_model, GTK_TYPE_TREE_MODEL_FILTER)

enum {
    ETHERNET_TOGGLED,
    WIFI_TOGGLED,
    MODEMS_TOGGLED,
    BT_TOGGLED,
    OFFLINE_MODE_TOGGLED,

    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), NMN_TYPE_MODEL, NmnModelPrivate))

typedef struct {
    NMClient *client;
    NMSettingsInterface *user_settings;
    NMSettingsInterface *system_settings;

    gboolean ethernet_active;
    gboolean wifi_active;
    gboolean modems_active;
    gboolean bt_active;
    gboolean offline_mode_active;

    gboolean disposed;
} NmnModelPrivate;

NmnModel *
nmn_model_new (void)
{
    NmnModel *model;
    NMClient *client;
    NMListModel *child_model;

    client = nm_client_new ();
    child_model = nm_list_model_new (client);
    g_object_unref (client);

    model = (NmnModel *) g_object_new (NMN_TYPE_MODEL,
                                       "child-model", child_model,
                                       NULL);

    g_object_unref (child_model);

    return model;
}

NMClient *
nmn_model_get_client (NmnModel *self)
{
    g_return_val_if_fail (NMN_IS_MODEL (self), NULL);

    return GET_PRIVATE (self)->client;
}

NMSettingsInterface *
nmn_model_get_user_settings (NmnModel *self)
{
    g_return_val_if_fail (NMN_IS_MODEL (self), NULL);

    return GET_PRIVATE (self)->user_settings;
}

NMSettingsInterface *
nmn_model_get_system_settings (NmnModel *self)
{
    g_return_val_if_fail (NMN_IS_MODEL (self), NULL);

    return GET_PRIVATE (self)->system_settings;
}

gboolean
nmn_model_ethernet_get_active (NmnModel *self)
{
    g_return_val_if_fail (NMN_IS_MODEL (self), FALSE);

    return GET_PRIVATE (self)->ethernet_active;
}

static void
ethernet_set_active (NmnModel *self,
                     gboolean active)
{
    GtkTreeModel *child_model;
    GtkTreeIter iter;
    gboolean valid;

    child_model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (self));

    valid = gtk_tree_model_get_iter_first (child_model, &iter);
    while (valid) {
        NMListItem *item = NULL;

        gtk_tree_model_get (child_model, &iter, NM_LIST_MODEL_COL_ITEM, &item, -1);
        if (item) {
            if (NM_IS_ETHERNET_ITEM (item)) {
                gboolean disconnected = nm_list_item_get_status (item) == NM_LIST_ITEM_STATUS_DISCONNECTED;

                if (active && disconnected)
                    nm_list_item_connect (item);
                else if (!active && !disconnected)
                    nm_list_item_disconnect (item);
            }

            g_object_unref (item);
        }

        valid = gtk_tree_model_iter_next (child_model, &iter);
    }
}

void
nmn_model_ethernet_toggled (NmnModel *self,
                            gboolean active)
{
    NmnModelPrivate *priv;

    g_return_if_fail (NMN_IS_MODEL (self));

    priv = GET_PRIVATE (self);
    if (priv->ethernet_active != active) {
        /* FIXME: Save in gconf? */
        priv->ethernet_active = active;
        gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (self));
        ethernet_set_active (self, active);

        g_signal_emit (self, signals[ETHERNET_TOGGLED], 0, active);
    }
}

gboolean
nmn_model_wifi_can_change (NmnModel *self)
{
    g_return_val_if_fail (NMN_IS_MODEL (self), FALSE);

    return nm_client_wireless_hardware_get_enabled (nmn_model_get_client (self));
}

gboolean
nmn_model_wifi_get_active (NmnModel *self)
{
    g_return_val_if_fail (NMN_IS_MODEL (self), FALSE);

    return GET_PRIVATE (self)->wifi_active;
}

static void
wifi_toggled_internal (NmnModel *self, gboolean active)
{
    NmnModelPrivate *priv = GET_PRIVATE (self);

    if (priv->wifi_active != active) {
        /* FIXME: Save in gconf? */
        priv->wifi_active = active;
        gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (self));

        g_signal_emit (self, signals[WIFI_TOGGLED], 0, active);
    }
}

void
nmn_model_wifi_toggled (NmnModel *self,
                        gboolean active)
{
    g_return_if_fail (NMN_IS_MODEL (self));

    wifi_toggled_internal (self, active);
    nm_client_wireless_set_enabled (nmn_model_get_client (self), active);
}

gboolean
nmn_model_modems_get_active (NmnModel *self)
{
    g_return_val_if_fail (NMN_IS_MODEL (self), FALSE);

    return GET_PRIVATE (self)->modems_active;
}

static void
modems_deactive (NmnModel *self)
{
    GtkTreeModel *child_model;
    GtkTreeIter iter;
    gboolean valid;

    child_model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (self));
    valid = gtk_tree_model_get_iter_first (child_model, &iter);
    while (valid) {
        NMListItem *item = NULL;

        gtk_tree_model_get (child_model, &iter, NM_LIST_MODEL_COL_ITEM, &item, -1);
        if (item) {
            if ((NM_IS_GSM_ITEM (item) || NM_IS_CDMA_ITEM (item)) && 
                nm_list_item_get_status (item) != NM_LIST_ITEM_STATUS_DISCONNECTED)

                nm_list_item_disconnect (item);

            g_object_unref (item);
        }

        valid = gtk_tree_model_iter_next (child_model, &iter);
    }
}

void
nmn_model_modems_toggled (NmnModel *self,
                          gboolean active)
{
    NmnModelPrivate *priv;

    g_return_if_fail (NMN_IS_MODEL (self));

    priv = GET_PRIVATE (self);
    if (priv->modems_active != active) {
        /* FIXME: Save in gconf? */
        priv->modems_active = active;
        gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (self));
        if (!active)
            modems_deactive (self);

        g_signal_emit (self, signals[MODEMS_TOGGLED], 0, active);
    }
}

gboolean
nmn_model_bt_get_active (NmnModel *self)
{
    g_return_val_if_fail (NMN_IS_MODEL (self), FALSE);

    return GET_PRIVATE (self)->bt_active;
}

static void
bt_deactive (NmnModel *self)
{
    GtkTreeModel *child_model;
    GtkTreeIter iter;
    gboolean valid;

    child_model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (self));
    valid = gtk_tree_model_get_iter_first (child_model, &iter);
    while (valid) {
        NMListItem *item = NULL;

        gtk_tree_model_get (child_model, &iter, NM_LIST_MODEL_COL_ITEM, &item, -1);
        if (item) {
            if (NM_IS_BT_ITEM (item) && nm_list_item_get_status (item) != NM_LIST_ITEM_STATUS_DISCONNECTED)
                nm_list_item_disconnect (item);

            g_object_unref (item);
        }

        valid = gtk_tree_model_iter_next (child_model, &iter);
    }
}

void
nmn_model_bt_toggled (NmnModel *self,
                      gboolean active)
{
    NmnModelPrivate *priv;

    g_return_if_fail (NMN_IS_MODEL (self));

    priv = GET_PRIVATE (self);
    if (priv->bt_active != active) {
        /* FIXME: Save in gconf? */
        priv->bt_active = active;
        gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (self));
        if (!active)
            bt_deactive (self);

        g_signal_emit (self, signals[BT_TOGGLED], 0, active);
    }
}

gboolean
nmn_model_offline_mode_get_active (NmnModel *self)
{
    g_return_val_if_fail (NMN_IS_MODEL (self), FALSE);

    return GET_PRIVATE (self)->offline_mode_active;
}

static void
offline_mode_toggled_internal (NmnModel *self, gboolean active)
{
    NmnModelPrivate *priv = GET_PRIVATE (self);

    if (priv->offline_mode_active != active) {
        /* FIXME: Save in gconf? */
        priv->offline_mode_active = active;
        gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (self));

        g_signal_emit (self, signals[OFFLINE_MODE_TOGGLED], 0, active);
    }
}

void
nmn_model_offline_mode_toggled (NmnModel *self,
                                gboolean active)
{
    g_return_if_fail (NMN_IS_MODEL (self));

    offline_mode_toggled_internal (self, active);
    nm_client_sleep (nmn_model_get_client (self), active);
}

static void
nm_client_state_changed (NMClient *client,
                         GParamSpec *gobject,
                         gpointer user_data)
{
    offline_mode_toggled_internal (NMN_MODEL (user_data), nm_client_get_state (client) == NM_STATE_ASLEEP);
}

static void
nm_wireless_state_changed (NMClient *client,
                           GParamSpec *gobject,
                           gpointer user_data)
{
    wifi_toggled_internal (NMN_MODEL (user_data), nm_client_wireless_get_enabled (client));
}

static gboolean
model_row_visible_func (GtkTreeModel *model,
                        GtkTreeIter  *iter,
                        gpointer      data)
{
    NMListItem *item = NULL;
    gboolean visible = FALSE;

    gtk_tree_model_get (model, iter, NM_LIST_MODEL_COL_ITEM, &item, -1);
    if (item) {
        NmnModelPrivate *priv = GET_PRIVATE (data);

        if (priv->offline_mode_active)
            visible = FALSE;
        else if (NM_IS_WIFI_ITEM (item))
            visible = priv->wifi_active;
        else if (NM_IS_ETHERNET_ITEM (item))
            visible = priv->ethernet_active;
        else if (NM_IS_GSM_ITEM (item) || NM_IS_GSM_PIN_REQUEST_ITEM (item))
            visible = priv->modems_active;
        else if (NM_IS_CDMA_ITEM (item))
            visible = priv->modems_active;
        else if (NM_IS_BT_ITEM (item))
            visible = priv->bt_active;

        g_object_unref (item);
    }

    return visible;
}

static gboolean
request_dbus_name (DBusGConnection *bus)
{
    DBusGProxy *proxy;
    GError *error = NULL;
    int request_name_result;

    dbus_connection_set_change_sigpipe (TRUE);
    dbus_connection_set_exit_on_disconnect (dbus_g_connection_get_connection (bus), FALSE);

    proxy = dbus_g_proxy_new_for_name (bus,
                                       "org.freedesktop.DBus",
                                       "/org/freedesktop/DBus",
                                       "org.freedesktop.DBus");

    if (!dbus_g_proxy_call (proxy, "RequestName", &error,
                            G_TYPE_STRING, NM_DBUS_SERVICE_USER_SETTINGS,
                            G_TYPE_UINT, DBUS_NAME_FLAG_DO_NOT_QUEUE,
                            G_TYPE_INVALID,
                            G_TYPE_UINT, &request_name_result,
                            G_TYPE_INVALID)) {
        g_warning ("Could not acquire the NetworkManagerUserSettings service.\n"
                   "  Message: '%s'", error->message);
        g_error_free (error);
        return FALSE;
    }

    if (request_name_result != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
        g_warning ("Could not acquire the NetworkManagerUserSettings service "
                   "as it is already taken.  Return: %d",
                   request_name_result);
        return FALSE;
    }

    return TRUE;
}

static void
nmn_model_init (NmnModel *model)
{
    NmnModelPrivate *priv = GET_PRIVATE (model);

    gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (model),
                                            model_row_visible_func, model, NULL);

    /* FIXME: Load from gconf? */
    priv->ethernet_active = TRUE;
    priv->wifi_active = TRUE;
    priv->modems_active = TRUE;
    priv->bt_active = TRUE;
    priv->offline_mode_active = FALSE;
}

static void
constructed (GObject *object)
{
    NmnModelPrivate *priv = GET_PRIVATE (object);
    NMListModel *child_model;
    DBusGConnection *bus;

    if (G_OBJECT_CLASS (nmn_model_parent_class)->constructed)
        G_OBJECT_CLASS (nmn_model_parent_class)->constructed (object);

    child_model = NM_LIST_MODEL (gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (object)));

    priv->client = nm_list_model_get_client (child_model);
    bus = nm_object_get_connection (NM_OBJECT (priv->client));

    priv->user_settings = NM_SETTINGS_INTERFACE (nm_gconf_settings_new (bus));
    priv->system_settings = NM_SETTINGS_INTERFACE (nm_remote_settings_system_new (bus));

    nm_list_model_add_settings (child_model, priv->user_settings);
    nm_list_model_add_settings (child_model, priv->system_settings);

    g_signal_connect (priv->client,
                      "notify::" NM_CLIENT_STATE,
                      G_CALLBACK (nm_client_state_changed),
                      object);

    g_signal_connect (priv->client,
                      "notify::" NM_CLIENT_WIRELESS_ENABLED,
                      G_CALLBACK (nm_wireless_state_changed),
                      object);

    g_signal_connect (priv->client,
                      "notify::" NM_CLIENT_WIRELESS_HARDWARE_ENABLED,
                      G_CALLBACK (nm_wireless_state_changed),
                      object);

    nm_wireless_state_changed (priv->client, NULL, object);
    nm_client_state_changed (priv->client, NULL, object);

    if (request_dbus_name (bus))
        dbus_g_connection_register_g_object (bus, NM_DBUS_PATH_SETTINGS, G_OBJECT (priv->user_settings));
}

static void
dispose (GObject *object)
{
    NmnModelPrivate *priv = GET_PRIVATE (object);

    if (!priv->disposed) {
        g_signal_handlers_disconnect_by_func (priv->client, gtk_tree_model_filter_refilter, object);

        g_object_unref (priv->user_settings);
        g_object_unref (priv->system_settings);

        priv->disposed = TRUE;
    }

    G_OBJECT_CLASS (nmn_model_parent_class)->dispose (object);
}

static void
nmn_model_class_init (NmnModelClass *class)
{
    GObjectClass *object_class = G_OBJECT_CLASS (class);

    g_type_class_add_private (object_class, sizeof (NmnModelPrivate));

    object_class->constructed = constructed;
    object_class->dispose = dispose;

    /* signals */
    signals[ETHERNET_TOGGLED] = g_signal_new 
        ("ethernet-toggled",
         G_OBJECT_CLASS_TYPE (class),
         G_SIGNAL_RUN_LAST,
         G_STRUCT_OFFSET (NmnModelClass, ethernet_toggled),
         NULL, NULL,
         g_cclosure_marshal_VOID__BOOLEAN,
         G_TYPE_NONE, 1,
         G_TYPE_BOOLEAN);

    signals[WIFI_TOGGLED] = g_signal_new 
        ("wifi-toggled",
         G_OBJECT_CLASS_TYPE (class),
         G_SIGNAL_RUN_LAST,
         G_STRUCT_OFFSET (NmnModelClass, wifi_toggled),
         NULL, NULL,
         g_cclosure_marshal_VOID__BOOLEAN,
         G_TYPE_NONE, 1,
         G_TYPE_BOOLEAN);

    signals[MODEMS_TOGGLED] = g_signal_new 
        ("modems-toggled",
         G_OBJECT_CLASS_TYPE (class),
         G_SIGNAL_RUN_LAST,
         G_STRUCT_OFFSET (NmnModelClass, modems_toggled),
         NULL, NULL,
         g_cclosure_marshal_VOID__BOOLEAN,
         G_TYPE_NONE, 1,
         G_TYPE_BOOLEAN);

    signals[BT_TOGGLED] = g_signal_new 
        ("bt-toggled",
         G_OBJECT_CLASS_TYPE (class),
         G_SIGNAL_RUN_LAST,
         G_STRUCT_OFFSET (NmnModelClass, bt_toggled),
         NULL, NULL,
         g_cclosure_marshal_VOID__BOOLEAN,
         G_TYPE_NONE, 1,
         G_TYPE_BOOLEAN);

    signals[OFFLINE_MODE_TOGGLED] = g_signal_new 
        ("offline-mode-toggled",
         G_OBJECT_CLASS_TYPE (class),
         G_SIGNAL_RUN_LAST,
         G_STRUCT_OFFSET (NmnModelClass, offline_mode_toggled),
         NULL, NULL,
         g_cclosure_marshal_VOID__BOOLEAN,
         G_TYPE_NONE, 1,
         G_TYPE_BOOLEAN);
}
