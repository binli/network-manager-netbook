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

#ifndef NM_STATUS_MODEL_H
#define NM_STATUS_MODEL_H

#include <gtk/gtk.h>
#include <nm-list-model.h>
#include <nm-list-item.h>

G_BEGIN_DECLS

#define NM_TYPE_STATUS_MODEL            (nm_status_model_get_type ())
#define NM_STATUS_MODEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), NM_TYPE_STATUS_MODEL, NMStatusModel))
#define NM_STATUS_MODEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), NM_TYPE_STATUS_MODEL, NMStatusModelClass))
#define NM_IS_STATUS_MODEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NM_TYPE_STATUS_MODEL))
#define NM_IS_STATUS_MODEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), NM_TYPE_STATUS_MODEL))
#define NM_STATUS_MODEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), NM_TYPE_STATUS_MODEL, NMStatusModelClass))

typedef struct {
    GtkTreeModelFilter parent;
} NMStatusModel;

typedef struct {
    GtkTreeModelFilterClass parent_class;

    /* Signals */
    void (*changed) (NMStatusModel *self,
                     NMListItem *active_item);
} NMStatusModelClass;

GType nm_status_model_get_type (void);

GtkTreeModel *nm_status_model_new             (GtkTreeModel *list_model);
NMListItem   *nm_status_model_get_active_item (NMStatusModel *self);

G_END_DECLS

#endif /* NM_STATUS_MODEL_H */
