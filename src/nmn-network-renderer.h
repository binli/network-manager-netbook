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

#ifndef NMN_NETWORK_RENDERER_H
#define NMN_NETWORK_RENDERER_H

#include <gtk/gtk.h>
#include "nmn-item-renderer.h"

#define NMN_TYPE_NETWORK_RENDERER            (nmn_network_renderer_get_type ())
#define NMN_NETWORK_RENDERER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), NMN_TYPE_NETWORK_RENDERER, NmnNetworkRenderer))
#define NMN_NETWORK_RENDERER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), NMN_TYPE_NETWORK_RENDERER, NmnNetworkRendererClass))
#define NMN_IS_NETWORK_RENDERER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NMN_TYPE_NETWORK_RENDERER))
#define NMN_IS_NETWORK_RENDERER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), NMN_TYPE_NETWORK_RENDERER))
#define NMN_NETWORK_RENDERER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), NMN_TYPE_NETWORK_RENDERER, NmnNetworkRendererClass))

typedef struct {
    NmnItemRenderer parent;
} NmnNetworkRenderer;

typedef struct {
    NmnItemRendererClass parent;
} NmnNetworkRendererClass;

GType nmn_network_renderer_get_type (void);

GtkWidget  *nmn_network_renderer_new (void);

#endif /* NMN_NETWORK_RENDERER_H */
