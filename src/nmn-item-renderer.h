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

#ifndef NMN_ITEM_RENDERER_H
#define NMN_ITEM_RENDERER_H

#include <gtk/gtk.h>
#include <nm-list-item.h>

#define NMN_TYPE_ITEM_RENDERER            (nmn_item_renderer_get_type ())
#define NMN_ITEM_RENDERER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), NMN_TYPE_ITEM_RENDERER, NmnItemRenderer))
#define NMN_ITEM_RENDERER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), NMN_TYPE_ITEM_RENDERER, NmnItemRendererClass))
#define NMN_IS_ITEM_RENDERER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NMN_TYPE_ITEM_RENDERER))
#define NMN_IS_ITEM_RENDERER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), NMN_TYPE_ITEM_RENDERER))
#define NMN_ITEM_RENDERER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), NMN_TYPE_ITEM_RENDERER, NmnItemRendererClass))

typedef struct {
    GtkEventBox parent;

    GtkTreeIter iter;
    int index;
} NmnItemRenderer;

typedef struct {
    GtkEventBoxClass parent;

    /* Signals */
    void (*background_updated) (NmnItemRenderer *self,
                                gboolean prelight);

    void (*item_changed)       (NmnItemRenderer *self);
} NmnItemRendererClass;

GType nmn_item_renderer_get_type (void);

NMListItem *nmn_item_renderer_get_item (NmnItemRenderer *self);
void        nmn_item_renderer_set_item (NmnItemRenderer *self,
                                        NMListItem *item);

gboolean    nmn_item_renderer_is_prelight (NmnItemRenderer *self);

GtkWidget  *nmn_item_renderer_get_content_area (NmnItemRenderer *self);

void nmn_item_renderer_show_error (NmnItemRenderer *self,
                                   const char *message);

void nmn_item_renderer_hide_error (NmnItemRenderer *self);

#endif /* NMN_ITEM_RENDERER_H */
