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

#ifndef NM_BT_PROVIDER_H
#define NM_BT_PROVIDER_H

#include <glib-object.h>
#include <nm-device-bt.h>
#include <nm-device-provider.h>

G_BEGIN_DECLS

#define NM_TYPE_BT_PROVIDER            (nm_bt_provider_get_type ())
#define NM_BT_PROVIDER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), NM_TYPE_BT_PROVIDER, NMBtProvider))
#define NM_BT_PROVIDER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), NM_TYPE_BT_PROVIDER, NMBtProviderClass))
#define NM_IS_BT_PROVIDER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NM_TYPE_BT_PROVIDER))
#define NM_IS_BT_PROVIDER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), NM_TYPE_BT_PROVIDER))
#define NM_BT_PROVIDER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), NM_TYPE_BT_PROVIDER, NMBtProviderClass))

typedef struct {
    NMDeviceProvider parent;
} NMBtProvider;

typedef struct {
    NMDeviceProviderClass parent_class;
} NMBtProviderClass;

GType nm_bt_provider_get_type (void);

NMItemProvider *nm_bt_provider_new (NMClient *client,
                                    NMDeviceBt *device);

G_END_DECLS

#endif /* NM_BT_PROVIDER_H */
