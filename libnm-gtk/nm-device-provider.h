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

#ifndef NM_DEVICE_PROVIDER_H
#define NM_DEVICE_PROVIDER_H

#include <glib-object.h>
#include <nm-device.h>
#include <nm-item-provider.h>

G_BEGIN_DECLS

#define NM_TYPE_DEVICE_PROVIDER            (nm_device_provider_get_type ())
#define NM_DEVICE_PROVIDER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), NM_TYPE_DEVICE_PROVIDER, NMDeviceProvider))
#define NM_DEVICE_PROVIDER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), NM_TYPE_DEVICE_PROVIDER, NMDeviceProviderClass))
#define NM_IS_DEVICE_PROVIDER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NM_TYPE_DEVICE_PROVIDER))
#define NM_IS_DEVICE_PROVIDER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), NM_TYPE_DEVICE_PROVIDER))
#define NM_DEVICE_PROVIDER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), NM_TYPE_DEVICE_PROVIDER, NMDeviceProviderClass))

#define NM_DEVICE_PROVIDER_DEVICE "device"

typedef struct {
    NMItemProvider parent;
} NMDeviceProvider;

typedef struct {
    NMItemProviderClass parent_class;

    /* Methods */
    void (*state_changed) (NMDeviceProvider *self,
                           NMDeviceState new_state,
                           NMDeviceState old_state,
                           NMDeviceStateReason reason);
} NMDeviceProviderClass;

GType nm_device_provider_get_type (void);

NMDevice *nm_device_provider_get_device (NMDeviceProvider *self);
gboolean  nm_device_provider_ready      (NMDeviceProvider *self);

G_END_DECLS

#endif /* NM_DEVICE_PROVIDER_H */
