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

#include <glib/gi18n.h>
#include "nm-device-item.h"

G_DEFINE_ABSTRACT_TYPE (NMDeviceItem, nm_device_item, NM_TYPE_CONNECTION_ITEM)

enum {
    PROP_0,
    PROP_DEVICE,

    LAST_PROP
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), NM_TYPE_DEVICE_ITEM, NMDeviceItemPrivate))

typedef struct {
    NMDevice *device;

    gboolean disposed;
} NMDeviceItemPrivate;

NMDevice *
nm_device_item_get_device (NMDeviceItem *self)
{
    g_return_val_if_fail (NM_IS_DEVICE_ITEM (self), NULL);

    return GET_PRIVATE (self)->device;
}

const char *
nm_device_item_get_hw_address (NMDeviceItem *self)
{
    g_return_val_if_fail (NM_IS_DEVICE_ITEM (self), NULL);

    if (NM_DEVICE_ITEM_GET_CLASS (self)->get_hw_address)
        return NM_DEVICE_ITEM_GET_CLASS (self)->get_hw_address (self);

    return NULL;
}

static void
connect_cb (gpointer user_data, const char *object_path, GError *error)
{
    if (error) {
        char *msg;

        msg = g_strdup_printf (_("Could not activate device: %s"), error->message);
        nm_list_item_warning (NM_LIST_ITEM (user_data), msg);
        g_free (msg);
    }
}

static void
connect (NMListItem *item)
{
    NMDeviceItem *self = NM_DEVICE_ITEM (item);
    NMConnectionItem *connection_item = NM_CONNECTION_ITEM (self);
    NMConnection *connection;
    const char *path;
    const char *service_name;
    char *specific_object;
    NMConnectionScope scope;

    connection = NM_CONNECTION (nm_connection_item_get_connection (connection_item));
    path = nm_connection_get_path (connection);

    scope = nm_connection_get_scope (connection);
    service_name = (scope == NM_CONNECTION_SCOPE_USER) ?
        NM_DBUS_SERVICE_USER_SETTINGS : NM_DBUS_SERVICE_SYSTEM_SETTINGS;

    if (NM_DEVICE_ITEM_GET_CLASS (item)->get_specific_object)
        specific_object = NM_DEVICE_ITEM_GET_CLASS (item)->get_specific_object (self);
    else
        specific_object = g_strdup ("/");

    nm_client_activate_connection (nm_connection_item_get_client (connection_item),
                                   service_name, path,
                                   nm_device_item_get_device (self),
                                   specific_object,
                                   connect_cb,
                                   item);

    g_free (specific_object);
}

static void
disconnect_cb (NMDevice *device, GError *error, gpointer user_data)
{
    if (error) {
        char *msg;

        msg = g_strdup_printf (_("Could not deactivate device: %s"), error->message);
        nm_list_item_warning (NM_LIST_ITEM (user_data), msg);
        g_free (msg);
    }
}

static void
disconnect (NMListItem *item)
{
    nm_device_disconnect (nm_device_item_get_device (NM_DEVICE_ITEM (item)), disconnect_cb, item);
}

/*****************************************************************************/

static void
nm_device_item_init (NMDeviceItem *self)
{
}

static void
set_property (GObject *object, guint prop_id,
              const GValue *value, GParamSpec *pspec)
{
    NMDeviceItemPrivate *priv = GET_PRIVATE (object);

    switch (prop_id) {
    case PROP_DEVICE:
        /* Construct only */
        priv->device = g_value_dup_object (value);
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
    NMDeviceItemPrivate *priv = GET_PRIVATE (object);

    switch (prop_id) {
    case PROP_DEVICE:
        g_value_set_object (value, priv->device);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
dispose (GObject *object)
{
    NMDeviceItemPrivate *priv = GET_PRIVATE (object);

    if (!priv->disposed) {
        g_object_unref (priv->device);
        priv->disposed = TRUE;
    }

    G_OBJECT_CLASS (nm_device_item_parent_class)->dispose (object);
}

static void
nm_device_item_class_init (NMDeviceItemClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    NMListItemClass *list_class = NM_LIST_ITEM_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (NMDeviceItemPrivate));

    object_class->set_property = set_property;
    object_class->get_property = get_property;
    object_class->dispose = dispose;

    list_class->connect = connect;
    list_class->disconnect = disconnect;

    /* Properties */
    g_object_class_install_property
        (object_class, PROP_DEVICE,
         g_param_spec_object (NM_DEVICE_ITEM_DEVICE,
                              "Device",
                              "Device",
                              NM_TYPE_DEVICE,
                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}
