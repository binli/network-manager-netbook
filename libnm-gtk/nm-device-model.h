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

#ifndef NM_DEVICE_MODEL_H
#define NM_DEVICE_MODEL_H

#include <gtk/gtk.h>
#include <nm-client.h>

G_BEGIN_DECLS

#define NM_TYPE_DEVICE_MODEL            (nm_device_model_get_type ())
#define NM_DEVICE_MODEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), NM_TYPE_DEVICE_MODEL, NMDeviceModel))
#define NM_DEVICE_MODEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), NM_TYPE_DEVICE_MODEL, NMDeviceModelClass))
#define NM_IS_DEVICE_MODEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NM_TYPE_DEVICE_MODEL))
#define NM_IS_DEVICE_MODEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), NM_TYPE_DEVICE_MODEL))
#define NM_DEVICE_MODEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), NM_TYPE_DEVICE_MODEL, NMDeviceModelClass))

#define NM_DEVICE_MODEL_CLIENT "client"

enum {
    NM_DEVICE_MODEL_COL_DEVICE,
    NM_DEVICE_MODEL_COL_IFACE,
    NM_DEVICE_MODEL_COL_DESCRIPTION,
    NM_DEVICE_MODEL_COL_ICON,
    NM_DEVICE_MODEL_COL_STATE,

    NM_DEVICE_MODEL_N_COLUMNS
};

typedef struct {
    GtkListStore parent;
} NMDeviceModel;

typedef struct {
    GtkListStoreClass parent_class;
} NMDeviceModelClass;

GType nm_device_model_get_type (void);

NMDeviceModel *nm_device_model_new (NMClient *client);

G_END_DECLS

#endif /* NM_DEVICE_MODEL_H */
