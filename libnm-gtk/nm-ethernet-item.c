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
#include <nm-setting-8021x.h>
#include "nm-ethernet-item.h"
#include "nm-icon-cache.h"

G_DEFINE_TYPE (NMEthernetItem, nm_ethernet_item, NM_TYPE_DEVICE_ITEM)

NMListItem *
nm_ethernet_item_new (NMClient *client,
                      NMDeviceEthernet *device,
                      NMSettingsConnectionInterface *connection)
{
    g_return_val_if_fail (NM_IS_CLIENT (client), NULL);
    g_return_val_if_fail (NM_IS_DEVICE_ETHERNET (device), NULL);
    g_return_val_if_fail (NM_IS_SETTINGS_CONNECTION_INTERFACE (connection), NULL);

    return (NMListItem *) g_object_new (NM_TYPE_ETHERNET_ITEM,
                                        NM_LIST_ITEM_TYPE_NAME, _("wired"),
                                        NM_CONNECTION_ITEM_DELETE_ALLOWED, FALSE,
                                        NM_CONNECTION_ITEM_CLIENT, client,
                                        NM_CONNECTION_ITEM_CONNECTION, connection,
                                        NM_DEVICE_ITEM_DEVICE, device,
                                        NULL);
}

static void
update_icon (NMEthernetItem *self)
{
    const char *icon;
    NMListItemStatus status;

    status = nm_list_item_get_status (NM_LIST_ITEM (self));
    if (status == NM_LIST_ITEM_STATUS_CONNECTED)
        icon = "nm-device-wired";
    else
        icon = "nm-no-connection";

    if (icon)
        g_object_set (self, NM_LIST_ITEM_ICON, icon, NULL);
}

static int
priority (NMListItem *item)
{
    return NM_LIST_ITEM_PRIORITY_DEV_ETHERNET + NM_LIST_ITEM_CLASS (nm_ethernet_item_parent_class)->priority (item);
}

static const char *
ethernet_get_hw_address (NMDeviceItem *item)
{
    NMDeviceEthernet *device;

    device = NM_DEVICE_ETHERNET (nm_device_item_get_device (NM_DEVICE_ITEM (item)));

    return nm_device_ethernet_get_hw_address (device);
}

/*****************************************************************************/

static void
nm_ethernet_item_init (NMEthernetItem *self)
{
    update_icon (self);
}

static void
constructed (GObject *object)
{
    NMConnection *connection;
    NMSetting *setting;

    if (G_OBJECT_CLASS (nm_ethernet_item_parent_class)->constructed)
        G_OBJECT_CLASS (nm_ethernet_item_parent_class)->constructed (object);

    connection = NM_CONNECTION (nm_connection_item_get_connection (NM_CONNECTION_ITEM (object)));
    setting = nm_connection_get_setting (connection, NM_TYPE_SETTING_802_1X);

    if (setting)
        g_object_set (object, NM_LIST_ITEM_SECURITY, _("802.1x"), NULL);
}

static void
notify (GObject *object,
        GParamSpec *pspec)
{
    if (!pspec || !pspec->name)
        return;

    if (!strcmp (pspec->name, NM_LIST_ITEM_STATUS))
        update_icon (NM_ETHERNET_ITEM (object));
    else if (!strcmp (pspec->name, NM_CONNECTION_ITEM_CONNECTION) && 
             !nm_connection_item_get_connection (NM_CONNECTION_ITEM (object)))
        /* If the connection is removed from the item, request deletion */
        nm_list_item_request_remove (NM_LIST_ITEM (object));
}

static void
nm_ethernet_item_class_init (NMEthernetItemClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    NMListItemClass *list_class = NM_LIST_ITEM_CLASS (klass);
    NMDeviceItemClass *device_class = NM_DEVICE_ITEM_CLASS (klass);

    object_class->constructed = constructed;
    object_class->notify = notify;

    list_class->priority = priority;
    device_class->get_hw_address = ethernet_get_hw_address;
}
