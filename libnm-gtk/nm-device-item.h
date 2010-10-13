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

#ifndef NM_DEVICE_ITEM_H
#define NM_DEVICE_ITEM_H

#include <glib-object.h>
#include <nm-device.h>
#include <nm-connection-item.h>

G_BEGIN_DECLS

#define NM_TYPE_DEVICE_ITEM            (nm_device_item_get_type ())
#define NM_DEVICE_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), NM_TYPE_DEVICE_ITEM, NMDeviceItem))
#define NM_DEVICE_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), NM_TYPE_DEVICE_ITEM, NMDeviceItemClass))
#define NM_IS_DEVICE_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NM_TYPE_DEVICE_ITEM))
#define NM_IS_DEVICE_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), NM_TYPE_DEVICE_ITEM))
#define NM_DEVICE_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), NM_TYPE_DEVICE_ITEM, NMDeviceItemClass))

#define NM_DEVICE_ITEM_DEVICE "device"

typedef struct {
    NMConnectionItem parent;
} NMDeviceItem;

typedef struct {
    NMConnectionItemClass parent_class;

    /* Methods */
    char *(*get_specific_object) (NMDeviceItem *self);
    const char *(*get_hw_address) (NMDeviceItem *self);
} NMDeviceItemClass;

GType nm_device_item_get_type (void);

NMDevice   *nm_device_item_get_device     (NMDeviceItem *self);
const char *nm_device_item_get_hw_address (NMDeviceItem *self);

G_END_DECLS

#endif /* NM_DEVICE_ITEM_H */
