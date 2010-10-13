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

#ifndef NM_DEVICE_HANDLER_H
#define NM_DEVICE_HANDLER_H

#include <glib-object.h>
#include <nm-client.h>
#include <nm-item-provider.h>

G_BEGIN_DECLS

#define NM_TYPE_DEVICE_HANDLER            (nm_device_handler_get_type ())
#define NM_DEVICE_HANDLER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), NM_TYPE_DEVICE_HANDLER, NMDeviceHandler))
#define NM_DEVICE_HANDLER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), NM_TYPE_DEVICE_HANDLER, NMDeviceHandlerClass))
#define NM_IS_DEVICE_HANDLER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NM_TYPE_DEVICE_HANDLER))
#define NM_IS_DEVICE_HANDLER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), NM_TYPE_DEVICE_HANDLER))
#define NM_DEVICE_HANDLER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), NM_TYPE_DEVICE_HANDLER, NMDeviceHandlerClass))

#define NM_DEVICE_HANDLER_CLIENT "client"

typedef struct {
    GObject parent;
} NMDeviceHandler;

typedef struct {
    GObjectClass parent_class;

    /* Signals */
    void (*provider_added) (NMDeviceHandler *self,
                            NMItemProvider *provider);

    void (*provider_removed) (NMDeviceHandler *self,
                              NMItemProvider *provider);
} NMDeviceHandlerClass;

GType nm_device_handler_get_type (void);

NMDeviceHandler *nm_device_handler_new           (NMClient *client);
GSList          *nm_device_handler_get_providers (NMDeviceHandler *self);

G_END_DECLS

#endif /* NM_DEVICE_HANDLER_H */
