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

#ifndef NMN_PANEL_CLIENT_H
#define NMN_PANEL_CLIENT_H

#include <glib-object.h>
#include <moblin-panel/mpl-panel-gtk.h>
#include <nm-status-model.h>

G_BEGIN_DECLS

#define NMN_TYPE_PANEL_CLIENT            (nmn_panel_client_get_type ())
#define NMN_PANEL_CLIENT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), NMN_TYPE_PANEL_CLIENT, NmnPanelClient))
#define NMN_PANEL_CLIENT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), NMN_TYPE_PANEL_CLIENT, NmnPanelClientClass))
#define NMN_IS_PANEL_CLIENT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NMN_TYPE_PANEL_CLIENT))
#define NMN_IS_PANEL_CLIENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), NMN_TYPE_PANEL_CLIENT))
#define NMN_PANEL_CLIENT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), NMN_TYPE_PANEL_CLIENT, NmnPanelClientClass))

#define NMN_PANEL_CLIENT_MODEL "model"

typedef struct {
    MplPanelGtk parent;
} NmnPanelClient;

typedef struct {
    MplPanelGtkClass parent;
} NmnPanelClientClass;

GType nmn_panel_client_get_type (void);

NmnPanelClient *nmn_panel_client_new (NMStatusModel *model);

G_END_DECLS

#endif /* NMN_PANEL_CLIENT_H */
