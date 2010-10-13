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

#include <nm-device.h>
#include <nm-device-ethernet.h>
#include <nm-device-wifi.h>
#include <nm-serial-device.h>

#include "nm-device-model.h"
#include "nm-icon-cache.h"
#include "utils.h"

G_DEFINE_TYPE (NMDeviceModel, nm_device_model, GTK_TYPE_LIST_STORE)

enum {
    PROP_0,
    PROP_CLIENT,

    LAST_PROP
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), NM_TYPE_DEVICE_MODEL, NMDeviceModelPrivate))

typedef struct {
    NMClient *client;
    gulong device_added_id;
    gulong device_removed_id;

    gboolean disposed;
} NMDeviceModelPrivate;

static GQuark quark_device_iter = 0;

NMDeviceModel *
nm_device_model_new (NMClient *client)
{
    g_return_val_if_fail (NM_IS_CLIENT (client), NULL);

    return (NMDeviceModel *) g_object_new (NM_TYPE_DEVICE_MODEL,
                                           NM_DEVICE_MODEL_CLIENT, client,
                                           NULL);
}

static void
device_state_changed (NMDevice *device,
                      NMDeviceState new_state,
                      NMDeviceState old_state,
                      NMDeviceStateReason reason,
                      gpointer user_data)
{
    GtkListStore *store = GTK_LIST_STORE (user_data);
    GtkTreeIter *iter;

    iter = (GtkTreeIter *) g_object_get_qdata (G_OBJECT (device), quark_device_iter);
    if (iter)
        gtk_list_store_set (store, iter, NM_DEVICE_MODEL_COL_STATE, new_state, -1);
}

static void
device_added (NMClient *client,
              NMDevice *device,
              gpointer user_data)
{
    GtkListStore *store = GTK_LIST_STORE (user_data);
    GtkTreeIter *iter;
    GdkPixbuf *pixbuf;

    iter = g_slice_new0 (GtkTreeIter);

    if (NM_IS_DEVICE_ETHERNET (device))
        pixbuf = nm_icon_cache_get ("nm-device-wired");
    else if (NM_IS_DEVICE_WIFI (device))
        pixbuf = nm_icon_cache_get ("nm-device-wireless");
    else if (NM_IS_SERIAL_DEVICE (device))
        pixbuf = nm_icon_cache_get ("nm-device-wwan");
    else {
        g_warning ("Unhandled device type (%s)", G_OBJECT_TYPE_NAME (device));
        pixbuf = NULL;
    }

    gtk_list_store_append (store, iter);
    gtk_list_store_set (store, iter,
                        NM_DEVICE_MODEL_COL_DEVICE,        device,
                        NM_DEVICE_MODEL_COL_IFACE,         nm_device_get_iface (device),
                        NM_DEVICE_MODEL_COL_DESCRIPTION,   utils_get_device_description (device),
                        NM_DEVICE_MODEL_COL_ICON,          pixbuf,
                        NM_DEVICE_MODEL_COL_STATE,         nm_device_get_state (device),
                        -1);

    g_object_set_qdata_full (G_OBJECT (device), quark_device_iter, iter, (GDestroyNotify) gtk_tree_iter_free);
    g_signal_connect (device, "state-changed", G_CALLBACK (device_state_changed), user_data);
}

static void
device_removed (NMClient *client,
                NMDevice *device,
                gpointer user_data)
{
    GtkListStore *store = GTK_LIST_STORE (user_data);
    GtkTreeIter *iter;

    iter = (GtkTreeIter *) g_object_get_qdata (G_OBJECT (device), quark_device_iter);
    if (iter) {
        g_signal_handlers_disconnect_by_func (device, device_state_changed, user_data);
        gtk_list_store_remove (store, iter);
        g_object_set_qdata (G_OBJECT (device), quark_device_iter, NULL);
    }
}

#if 0
static int
sort_callback (GtkTreeModel *model,
               GtkTreeIter *a,
               GtkTreeIter *b,
               gpointer user_data)
{
    GValue value_a = {0, };
    GValue value_b = {0, };
    int result;

    gtk_tree_model_get_value (model, a, NM_DEVICE_MODEL_COL_ITEM, &value_a);
    gtk_tree_model_get_value (model, b, NM_DEVICE_MODEL_COL_ITEM, &value_b);

    result = nm_device_item_compare (NM_DEVICE_ITEM (g_value_get_object (&value_a)),
                                   NM_DEVICE_ITEM (g_value_get_object (&value_b)));

    g_value_unset (&value_a);
    g_value_unset (&value_b);

    return result;
}
#endif

/*****************************************************************************/

static void
nm_device_model_init (NMDeviceModel *self)
{
    GType types[NM_DEVICE_MODEL_N_COLUMNS];

    types[NM_DEVICE_MODEL_COL_DEVICE]      = NM_TYPE_DEVICE;
    types[NM_DEVICE_MODEL_COL_IFACE]       = G_TYPE_STRING;
    types[NM_DEVICE_MODEL_COL_DESCRIPTION] = G_TYPE_STRING;
    types[NM_DEVICE_MODEL_COL_ICON]        = GDK_TYPE_PIXBUF;
    types[NM_DEVICE_MODEL_COL_STATE]       = G_TYPE_INT;

    gtk_list_store_set_column_types (GTK_LIST_STORE (self), NM_DEVICE_MODEL_N_COLUMNS, types);
#if 0
    gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (self),
                                     NM_DEVICE_MODEL_COL_ITEM,
                                     sort_callback,
                                     NULL,
                                     NULL);

    gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (self), NM_DEVICE_MODEL_COL_DEVICE, GTK_SORT_ASCENDING);
#endif
}

static void
set_property (GObject *object, guint prop_id,
              const GValue *value, GParamSpec *pspec)
{
    NMDeviceModelPrivate *priv = GET_PRIVATE (object);

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
    NMDeviceModelPrivate *priv = GET_PRIVATE (object);

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
constructed (GObject *object)
{
    NMDeviceModelPrivate *priv = GET_PRIVATE (object);
    const GPtrArray *devices;
    int i;

    if (G_OBJECT_CLASS (nm_device_model_parent_class)->constructed)
        G_OBJECT_CLASS (nm_device_model_parent_class)->constructed (object);

    priv->device_added_id = g_signal_connect (priv->client, "device-added", G_CALLBACK (device_added), object);
    priv->device_removed_id = g_signal_connect (priv->client, "device-removed", G_CALLBACK (device_removed), object);

    devices = nm_client_get_devices (priv->client);
    for (i = 0; devices && devices->len > i; i++)
        device_added (priv->client, NM_DEVICE (g_ptr_array_index (devices, i)), object);
}

static void
dispose (GObject *object)
{
    NMDeviceModelPrivate *priv = GET_PRIVATE (object);

    if (!priv->disposed) {
        g_signal_handler_disconnect (priv->client, priv->device_added_id);
        g_signal_handler_disconnect (priv->client, priv->device_removed_id);

        if (priv->client)
            g_object_unref (priv->client);

        priv->disposed = TRUE;
    }

    G_OBJECT_CLASS (nm_device_model_parent_class)->dispose (object);
}

static void
nm_device_model_class_init (NMDeviceModelClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    quark_device_iter = g_quark_from_static_string ("NMDeviceModel-device-iter");

    g_type_class_add_private (object_class, sizeof (NMDeviceModelPrivate));

    object_class->set_property = set_property;
    object_class->get_property = get_property;
    object_class->constructed = constructed;
    object_class->dispose = dispose;

    /* Properties */
    g_object_class_install_property
        (object_class, PROP_CLIENT,
         g_param_spec_object (NM_DEVICE_MODEL_CLIENT,
                              "NMClient",
                              "NMClient",
                              NM_TYPE_CLIENT,
                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}
