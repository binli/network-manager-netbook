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

#ifndef NMN_LIST_H
#define NMN_LIST_H

#include <gtk/gtk.h>

#define NMN_TYPE_LIST            (nmn_list_get_type ())
#define NMN_LIST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), NMN_TYPE_LIST, NmnList))
#define NMN_LIST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), NMN_TYPE_LIST, NmnListClass))
#define NMN_IS_LIST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NMN_TYPE_LIST))
#define NMN_IS_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), NMN_TYPE_LIST))
#define NMN_LIST_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), NMN_TYPE_LIST, NmnListClass))

typedef struct {
    GtkVBox parent;
} NmnList;

typedef struct {
    GtkVBoxClass parent;
} NmnListClass;

GType nmn_list_get_type (void);

GtkWidget   *nmn_list_new            (void);
GtkWidget   *nmn_list_new_with_model (GtkTreeModel *model);
void         nmn_list_set_model      (NmnList *list,
                                      GtkTreeModel *model);

GtkTreeModel *nmn_list_get_model      (NmnList *list);

#endif /* NMN_LIST_H */
