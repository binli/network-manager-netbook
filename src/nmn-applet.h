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

#ifndef NMN_APPLET_H
#define NMN_APPLET_H

#include <gtk/gtk.h>
#include "nmn-model.h"

G_BEGIN_DECLS

#define NMN_TYPE_APPLET            (nmn_applet_get_type ())
#define NMN_APPLET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), NMN_TYPE_APPLET, NmnApplet))
#define NMN_APPLET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), NMN_TYPE_APPLET, NmnAppletClass))
#define NMN_IS_APPLET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NMN_TYPE_APPLET))
#define NMN_IS_APPLET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), NMN_TYPE_APPLET))
#define NMN_APPLET_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), NMN_TYPE_APPLET, NmnAppletClass))

#define NMN_APPLET_MODEL "model"

typedef struct {
    GtkNotebook parent;
} NmnApplet;

typedef struct {
    GtkNotebookClass parent;
} NmnAppletClass;

GType nmn_applet_get_type (void);

NmnApplet *nmn_applet_new  (NmnModel *model);
void       nmn_applet_show (NmnApplet *applet);
void       nmn_applet_hide (NmnApplet *applet);

G_END_DECLS

#endif /* NMN_APPLET_H */
