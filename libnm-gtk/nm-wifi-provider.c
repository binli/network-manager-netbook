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

#include <nm-utils.h>
#include "nm-wifi-provider.h"
#include "nm-wifi-item.h"
#include "utils.h"

G_DEFINE_TYPE (NMWifiProvider, nm_wifi_provider, NM_TYPE_DEVICE_PROVIDER)

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), NM_TYPE_WIFI_PROVIDER, NMWifiProviderPrivate))

typedef struct {
    gulong ap_added_id;
    gulong ap_removed_id;

    gboolean disposed;
} NMWifiProviderPrivate;

NMItemProvider *
nm_wifi_provider_new (NMClient *client,
                      NMDeviceWifi *device)
{
    g_return_val_if_fail (NM_IS_CLIENT (client), NULL);
    g_return_val_if_fail (NM_IS_DEVICE_WIFI (device), NULL);

    return (NMItemProvider *) g_object_new (NM_TYPE_WIFI_PROVIDER,
                                            NM_ITEM_PROVIDER_CLIENT, client,
                                            NM_DEVICE_PROVIDER_DEVICE, device,
                                            NULL);
}

static void
wifi_added (NMItemProvider *provider,
            NMSettingsConnectionInterface *connection)
{
    NMDeviceProvider *device_provider = NM_DEVICE_PROVIDER (provider);
    NMDeviceWifi *device;
    GSList *list;
    GSList *iter;
    const GPtrArray *ap_list;
    int i;

    if (!nm_device_provider_ready (device_provider))
        return;

    device = NM_DEVICE_WIFI (nm_device_provider_get_device (device_provider));

    /* First, see if we have an AP item already which has
       no connection and is compatible with this connection */
    list = nm_item_provider_get_items (provider);
    for (iter = list; iter; iter = iter->next) {
        NMListItem *item = (NMListItem *) iter->data;
        NMSettingsConnectionInterface *item_connection;
        NMAccessPoint *ap;

        item_connection = nm_connection_item_get_connection (NM_CONNECTION_ITEM (item));
        if (item_connection == connection) {
            /* Already have this same connection why do we get called??? */
            g_warning ("Connection already in the list, something is broken");
            return;
        }

        if (item_connection)
            continue;

        ap = nm_wifi_item_get_ap (NM_WIFI_ITEM (item));
        if (utils_connection_valid_for_device (NM_CONNECTION (connection), NM_DEVICE (device), ap)) {
            g_object_set (item, NM_CONNECTION_ITEM_CONNECTION, connection, NULL);
            return;
        }
    }

    /* That didn't work. let's see if we have a compatible AP so we can still
       show this connection. This happens when there's multiple connections which
       match an AP and we want to have an item for each connection */

    ap_list = nm_device_wifi_get_access_points (device);
    for (i = 0; ap_list && ap_list->len > i; i++) {
        NMAccessPoint *ap = (NMAccessPoint *) g_ptr_array_index (ap_list, i);

        if (utils_connection_valid_for_device (NM_CONNECTION (connection), NM_DEVICE (device), ap)) {
            NMListItem *item;

            item = nm_wifi_item_new (nm_item_provider_get_client (provider), device, ap, connection);
            nm_item_provider_item_added (provider, item);
            break;
        }
    }
}

static void
ap_added (NMDeviceWifi *device,
          NMAccessPoint *ap,
          gpointer user_data)
{
    NMItemProvider *provider = NM_ITEM_PROVIDER (user_data);
    const GByteArray *ssid;
    GSList *list;
    GSList *iter;
    NMListItem *item;
    gboolean added = FALSE;

    /* Don't add BSSs that hide their SSID */
    ssid = nm_access_point_get_ssid (ap);
    if (!ssid || nm_utils_is_empty_ssid (ssid->data, ssid->len))
        return;

    /* First, check if any existing item already has a compatible AP */
    list = nm_item_provider_get_items (provider);
    for (iter = list; iter; iter = iter->next) {
        NMWifiItem *wifi_item = (NMWifiItem *) iter->data;

        if (nm_wifi_item_add_ap (wifi_item, ap))
            added = TRUE;
    }

    if (added)
        return;

    /* It was not compatible with any existing item, see if we
     * have a connection for this AP */
    list = nm_item_provider_get_connections (provider);
    for (iter = list; iter; iter = iter->next) {
        NMSettingsConnectionInterface *connection = (NMSettingsConnectionInterface *) iter->data;

        if (!utils_connection_valid_for_device (NM_CONNECTION (connection), NM_DEVICE (device), ap))
            continue;

        item = nm_wifi_item_new (nm_item_provider_get_client (provider), device, ap, connection);
        nm_item_provider_item_added (provider, item);
        added = TRUE;
    }

    g_slist_free (list);

    if (added)
        return;

    /* There's no connection for this AP. Create a connectionless item */
    item = nm_wifi_item_new (nm_item_provider_get_client (provider), device, ap, NULL);
    nm_item_provider_item_added (provider, item);
}

static void
ap_removed (NMDeviceWifi *device,
            NMAccessPoint *ap,
            gpointer user_data)
{
    g_slist_foreach (nm_item_provider_get_items (NM_ITEM_PROVIDER (user_data)),
                     (GFunc) nm_wifi_item_remove_ap, ap);
}

/*****************************************************************************/

static void
nm_wifi_provider_init (NMWifiProvider *self)
{
}

static void
constructed (GObject *object)
{
    NMWifiProviderPrivate *priv = GET_PRIVATE (object);
    NMDeviceWifi *device;
    const GPtrArray *ap_list;
    int i;

    if (G_OBJECT_CLASS (nm_wifi_provider_parent_class)->constructed)
        G_OBJECT_CLASS (nm_wifi_provider_parent_class)->constructed (object);

    device = NM_DEVICE_WIFI (nm_device_provider_get_device (NM_DEVICE_PROVIDER (object)));
    priv->ap_added_id = g_signal_connect (device, "access-point-added", G_CALLBACK (ap_added), object);
    priv->ap_removed_id = g_signal_connect (device, "access-point-removed", G_CALLBACK (ap_removed), object);

    ap_list = nm_device_wifi_get_access_points (device);
    for (i = 0; ap_list && ap_list->len > i; i++)
        ap_added (device, (NMAccessPoint *) g_ptr_array_index (ap_list, i), object);
}

static void
dispose (GObject *object)
{
    NMWifiProviderPrivate *priv = GET_PRIVATE (object);

    if (!priv->disposed) {
        NMDevice *device;

        device = nm_device_provider_get_device (NM_DEVICE_PROVIDER (object));
        g_signal_handler_disconnect (device, priv->ap_added_id);
        g_signal_handler_disconnect (device, priv->ap_removed_id);

        priv->disposed = TRUE;
    }

    G_OBJECT_CLASS (nm_wifi_provider_parent_class)->dispose (object);
}

static void
nm_wifi_provider_class_init (NMWifiProviderClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    NMItemProviderClass *item_class = NM_ITEM_PROVIDER_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (NMWifiProviderPrivate));

    object_class->constructed = constructed;
    object_class->dispose = dispose;

    item_class->connection_added = wifi_added;
}
