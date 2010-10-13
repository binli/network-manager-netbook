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

#include "nm-cdma-provider.h"
#include "nm-cdma-item.h"
#include "utils.h"

G_DEFINE_TYPE (NMCdmaProvider, nm_cdma_provider, NM_TYPE_DEVICE_PROVIDER)

NMItemProvider *
nm_cdma_provider_new (NMClient *client,
                      NMCdmaDevice *device)
{
    g_return_val_if_fail (NM_IS_CLIENT (client), NULL);
    g_return_val_if_fail (NM_IS_CDMA_DEVICE (device), NULL);

    return (NMItemProvider *) g_object_new (NM_TYPE_CDMA_PROVIDER,
                                            NM_ITEM_PROVIDER_CLIENT, client,
                                            NM_DEVICE_PROVIDER_DEVICE, device,
                                            NULL);
}

static void
cdma_added (NMItemProvider *provider,
            NMSettingsConnectionInterface *connection)
{
    NMDeviceProvider *device_provider = NM_DEVICE_PROVIDER (provider);
    NMDevice *device;

    if (!nm_device_provider_ready (device_provider))
        return;

    device = nm_device_provider_get_device (device_provider);
    if (utils_connection_valid_for_device (NM_CONNECTION (connection), device, NULL)) {
        NMListItem *item;

        item = nm_cdma_item_new (nm_item_provider_get_client (provider), NM_CDMA_DEVICE (device), connection);
        nm_item_provider_item_added (provider, item);
    }
}

/*****************************************************************************/

static void
nm_cdma_provider_init (NMCdmaProvider *self)
{
}

static void
nm_cdma_provider_class_init (NMCdmaProviderClass *klass)
{
    NMItemProviderClass *item_class = NM_ITEM_PROVIDER_CLASS (klass);

    item_class->connection_added = cdma_added;
}
