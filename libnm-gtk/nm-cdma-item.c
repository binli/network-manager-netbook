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
#include <nm-setting-8021x.h>
#include "nm-cdma-item.h"

G_DEFINE_TYPE (NMCdmaItem, nm_cdma_item, NM_TYPE_DEVICE_ITEM)

NMListItem *
nm_cdma_item_new (NMClient *client,
                  NMCdmaDevice *device,
                  NMSettingsConnectionInterface *connection)
{
    g_return_val_if_fail (NM_IS_CLIENT (client), NULL);
    g_return_val_if_fail (NM_IS_CDMA_DEVICE (device), NULL);
    g_return_val_if_fail (NM_IS_SETTINGS_CONNECTION_INTERFACE (connection), NULL);

    return (NMListItem *) g_object_new (NM_TYPE_CDMA_ITEM,
                                        NM_LIST_ITEM_TYPE_NAME, _("3G"),
                                        NM_CONNECTION_ITEM_CLIENT, client,
                                        NM_CONNECTION_ITEM_CONNECTION, connection,
                                        NM_DEVICE_ITEM_DEVICE, device,
                                        NULL);
}

static int
priority (NMListItem *item)
{
    return NM_LIST_ITEM_PRIORITY_DEV_CDMA + NM_LIST_ITEM_CLASS (nm_cdma_item_parent_class)->priority (item);
}

/*****************************************************************************/

static void
notify (GObject *object, GParamSpec *spec)
{
    /* If the connection is removed from the item, request deletion */
    if (spec && !g_strcmp0 (spec->name, NM_CONNECTION_ITEM_CONNECTION) && 
        !nm_connection_item_get_connection (NM_CONNECTION_ITEM (object)))

        nm_list_item_request_remove (NM_LIST_ITEM (object));
}

static void
nm_cdma_item_init (NMCdmaItem *self)
{
    g_object_set (self, NM_LIST_ITEM_ICON, "nm-device-wwan", NULL);
}

static void
nm_cdma_item_class_init (NMCdmaItemClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    NMListItemClass *list_class = NM_LIST_ITEM_CLASS (klass);

    object_class->notify = notify;

    list_class->priority = priority;
}
