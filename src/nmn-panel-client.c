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
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <libnotify/notify.h>
#include <moblin-panel/mpl-panel-common.h>
#include <nm-device-ethernet.h>
#include <nm-device-wifi.h>
#include <nm-gsm-device.h>
#include <nm-cdma-device.h>
#include <nm-utils.h>
#include <nm-ethernet-item.h>

#include "nmn-panel-client.h"

#define ACTIVATION_STEPS 6

G_DEFINE_TYPE (NmnPanelClient, nmn_panel_client, MPL_TYPE_PANEL_GTK)

enum {
    PROP_0,
    PROP_MODEL,

    LAST_PROP
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), NMN_TYPE_PANEL_CLIENT, NmnPanelClientPrivate))

typedef struct {
    NMStatusModel *model;

    NMListItem *item;
    guint animation_id;
    guint animation_step;

    /* Saved from previous item */
    NMListItemStatus status;
    char *connection_type;
    char *connection_name;
} NmnPanelClientPrivate;

NmnPanelClient *
nmn_panel_client_new (NMStatusModel *model)
{
    g_return_val_if_fail (NM_IS_STATUS_MODEL (model), NULL);

    return (NmnPanelClient *) g_object_new (NMN_TYPE_PANEL_CLIENT,
                                           "name", MPL_PANEL_NETWORK,
                                           "tooltip", _("network"),
                                           "stylesheet", THEME_PATH "/network-manager-netbook.css",
                                           "button-style", "unknown",
                                           "toolbar-service", TRUE,
                                           NMN_PANEL_CLIENT_MODEL, model,
                                           NULL);

}

static void
stop_activation_animation (NmnPanelClient *self)
{
    NmnPanelClientPrivate *priv = GET_PRIVATE (self);

    if (priv->animation_id) {
        g_source_remove (priv->animation_id);
        priv->animation_id = 0;
        priv->animation_step = 0;
    }
}

static gboolean
activation_animation (gpointer data)
{
    NmnPanelClient *self = NMN_PANEL_CLIENT (data);
    NmnPanelClientPrivate *priv = GET_PRIVATE (self);
    char *image;

    if (++priv->animation_step > ACTIVATION_STEPS)
        priv->animation_step = 1;

    image = g_strdup_printf ("progress-%02d", priv->animation_step);
    g_print ("Requesting button style %s", image);
    mpl_panel_client_request_button_style (MPL_PANEL_CLIENT (self), image);
    g_free (image);

    return TRUE;
}

/* FIXME: This is all pretty gross */
static const char *
massage_icon (const char *original)
{
    static char *icon = NULL;

    if (icon) {
        g_free (icon);
        icon = NULL;
    }

    if (!original)
        return NULL;

    if (g_str_has_prefix (original, "nm-"))
        icon = g_strdup (original + 3);
    else
        icon = g_strdup (original);

    if (g_str_has_suffix (icon, "-active") || g_str_has_suffix (icon, "-normal"))
        icon[strlen(icon) - 7] = '\0';

    return icon;
}

static void
update_icon (NmnPanelClient *self)
{
    NmnPanelClientPrivate *priv = GET_PRIVATE (self);
    NMListItemStatus status;

    status = priv->item ? nm_list_item_get_status (priv->item) : NM_LIST_ITEM_STATUS_DISCONNECTED;
    switch (status) {
    case NM_LIST_ITEM_STATUS_CONNECTING:
        if (priv->animation_id == 0) {
            priv->animation_id = g_timeout_add (200, activation_animation, self);
            activation_animation (self);
        }
        break;
    case NM_LIST_ITEM_STATUS_DISCONNECTED:
        stop_activation_animation (self);
        mpl_panel_client_request_button_style (MPL_PANEL_CLIENT (self), "no-network");
        break;
    case NM_LIST_ITEM_STATUS_CONNECTED:
        stop_activation_animation (self);
        mpl_panel_client_request_button_style (MPL_PANEL_CLIENT (self),
                                               massage_icon (nm_list_item_get_icon (priv->item)));
        
        break;
    }
}

static void
update_notification (NmnPanelClient *self)
{
    NmnPanelClientPrivate *priv = GET_PRIVATE (self);
    const char *title;
    char *msg;
    const char *connection_type;
    const char *connection_name;
    const char *icon;
    NMListItemStatus status;

    if (priv->item) {
        status = nm_list_item_get_status (priv->item);
        connection_type = nm_list_item_get_type_name (priv->item);

        /* Don't say "Connected to Wired, a wired network */
        if (!NM_IS_ETHERNET_ITEM (priv->item))
            connection_name = nm_list_item_get_name (priv->item);
        else
            connection_name = NULL;

        icon = nm_list_item_get_icon (priv->item);
    } else {
        status = NM_LIST_ITEM_STATUS_DISCONNECTED;
        connection_type = NULL;
        connection_name = NULL;
        icon = NULL;
    }

    if (priv->status == NM_LIST_ITEM_STATUS_DISCONNECTED && status == NM_LIST_ITEM_STATUS_CONNECTED) {
        title = _("Network connected");

        if (connection_type) {
            if (connection_name)
                msg = g_strdup_printf (_("You're now connected to %s, a %s network"),
                                       connection_name, connection_type);
            else
                msg = g_strdup_printf (_("You're now connected to %s network"), connection_type);
        } else
            msg = g_strdup (_("You're now connected to network"));
    } else if (priv->status == NM_LIST_ITEM_STATUS_CONNECTED && status == NM_LIST_ITEM_STATUS_DISCONNECTED) {
        title = _("Network lost");

        if (priv->connection_type) {
            if (priv->connection_name)
                msg = g_strdup_printf (_("Sorry, we've lost your %s connection to %s"),
                                       priv->connection_type, priv->connection_name);
            else
                msg = g_strdup_printf (_("Sorry, we've lost your %s connection"), priv->connection_type);
        } else
            msg = g_strdup (_("Sorry, we've lost your connection"));
    } else {
        title = NULL;
        msg = NULL;
    }

    if (title && msg) {
        NotifyNotification *note;

        note = notify_notification_new (title, msg, icon, NULL);
        notify_notification_set_timeout (note, 10000);
        notify_notification_show (note, NULL);
        g_object_unref (note);
        g_free (msg);

        priv->status = status;
        g_free (priv->connection_type);
        priv->connection_type = connection_type ? g_strdup (connection_type) : NULL;
        g_free (priv->connection_name);
        priv->connection_name = connection_name ? g_strdup (connection_name) : NULL;
    }
}

static void
update_tooltip (NmnPanelClient *self)
{
    NmnPanelClientPrivate *priv = GET_PRIVATE (self);
    char *tip;
    const char *connection_type;
    const char *connection_name;
    NMListItemStatus status;

    status = priv->item ? nm_list_item_get_status (priv->item) : NM_LIST_ITEM_STATUS_DISCONNECTED;
    switch (status) {
    case NM_LIST_ITEM_STATUS_DISCONNECTED:
        tip = g_strdup_printf (_("networks - not connected"));
        break;
    case NM_LIST_ITEM_STATUS_CONNECTING:
        tip = g_strdup_printf (_("networks - connecting"));
        break;
    case NM_LIST_ITEM_STATUS_CONNECTED:
        connection_type = nm_list_item_get_type_name (priv->item);

        /* Don't produce "networks - Wired - wired" */
        if (!NM_IS_ETHERNET_ITEM (priv->item))
            connection_name = nm_list_item_get_name (priv->item);
        else
            connection_name = NULL;

        if (connection_type) {
            if (connection_name)
                tip = g_strdup_printf (_("networks - %s - %s"), connection_name, connection_type);
            else
                tip = g_strdup_printf (_("networks - %s"), connection_type);
        } else
            tip = g_strdup (_("networks - connected"));

        break;
    }

    mpl_panel_client_request_tooltip (MPL_PANEL_CLIENT (self), tip);
    g_free (tip);
}

static void
item_status_changed (NMListItem *item,
                     GParamSpec *spec,
                     gpointer user_data)
{
    NmnPanelClient *self = NMN_PANEL_CLIENT (user_data);

    update_notification (self);
    update_tooltip (self);
    update_icon (self);
}

static void
model_changed (NMStatusModel *model,
               NMListItem *active_item,
               gpointer user_data)
{
    NmnPanelClient *self = NMN_PANEL_CLIENT (user_data);
    NmnPanelClientPrivate *priv = GET_PRIVATE (self);

    if (active_item == priv->item)
        return;

    if (priv->item)
        g_signal_handlers_disconnect_by_func (priv->item, item_status_changed, self);

    priv->item = active_item;

    if (active_item) {
        g_signal_connect (active_item, "notify::" NM_LIST_ITEM_STATUS, G_CALLBACK (item_status_changed), self);
        g_signal_connect (active_item, "notify::" NM_LIST_ITEM_ICON, G_CALLBACK (item_status_changed), self);
    }
    
    item_status_changed (active_item, NULL, self);
}

/*****************************************************************************/

static void
nmn_panel_client_init (NmnPanelClient *self)
{
    notify_init ("network-manager-netbook");
    mpl_panel_client_set_height_request (MPL_PANEL_CLIENT (self), 499);
}

static void
set_property (GObject *object, guint prop_id,
              const GValue *value, GParamSpec *pspec)
{
    NmnPanelClientPrivate *priv = GET_PRIVATE (object);

    switch (prop_id) {
    case PROP_MODEL:
        /* Construct only */
        priv->model = g_value_dup_object (value);
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
    NmnPanelClientPrivate *priv = GET_PRIVATE (object);

    switch (prop_id) {
    case PROP_MODEL:
        g_value_set_object (value, priv->model);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
constructed (GObject *object)
{
    NmnPanelClientPrivate *priv = GET_PRIVATE (object);

    if (G_OBJECT_CLASS (nmn_panel_client_parent_class)->constructed)
        G_OBJECT_CLASS (nmn_panel_client_parent_class)->constructed (object);

    g_signal_connect (priv->model, "changed", G_CALLBACK (model_changed), object);
    item_status_changed (NULL, NULL, object);
}

static void
finalize (GObject *object)
{
    NmnPanelClientPrivate *priv = GET_PRIVATE (object);

    stop_activation_animation (NMN_PANEL_CLIENT (object));

    g_free (priv->connection_type);
    g_free (priv->connection_name);

    if (priv->model)
        g_object_unref (priv->model);

    notify_uninit ();

    G_OBJECT_CLASS (nmn_panel_client_parent_class)->finalize (object);
}

static void
nmn_panel_client_class_init (NmnPanelClientClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (NmnPanelClientPrivate));

    object_class->set_property = set_property;
    object_class->get_property = get_property;
    object_class->constructed = constructed;
    object_class->finalize = finalize;

    /* Properties */
    g_object_class_install_property
        (object_class, PROP_MODEL,
         g_param_spec_object (NMN_PANEL_CLIENT_MODEL,
                              "NMStatusModel",
                              "NMStatusModel",
                              NM_TYPE_STATUS_MODEL,
                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}
