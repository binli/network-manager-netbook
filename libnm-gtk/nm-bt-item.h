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

#ifndef NM_BT_ITEM_H
#define NM_BT_ITEM_H

#include <glib-object.h>
#include <nm-device-bt.h>
#include <nm-device-item.h>

G_BEGIN_DECLS

#define NM_TYPE_BT_ITEM            (nm_bt_item_get_type ())
#define NM_BT_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), NM_TYPE_BT_ITEM, NMBtItem))
#define NM_BT_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), NM_TYPE_BT_ITEM, NMBtItemClass))
#define NM_IS_BT_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NM_TYPE_BT_ITEM))
#define NM_IS_BT_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), NM_TYPE_BT_ITEM))
#define NM_BT_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), NM_TYPE_BT_ITEM, NMBtItemClass))

typedef struct {
    NMDeviceItem parent;
} NMBtItem;

typedef struct {
    NMDeviceItemClass parent_class;
} NMBtItemClass;

GType nm_bt_item_get_type (void);

NMListItem *nm_bt_item_new (NMClient *client,
                            NMDeviceBt *device,
                            NMSettingsConnectionInterface *connection);

G_END_DECLS

#endif /* NM_BT_ITEM_H */
