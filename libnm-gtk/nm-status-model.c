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

#include "nm-status-model.h"

G_DEFINE_TYPE (NMStatusModel, nm_status_model, GTK_TYPE_TREE_MODEL_FILTER)

enum {
    CHANGED,

    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

GtkTreeModel *
nm_status_model_new (GtkTreeModel *list_model)
{
    g_return_val_if_fail (GTK_IS_TREE_MODEL (list_model), NULL);

    return (GtkTreeModel *) g_object_new (NM_TYPE_STATUS_MODEL,
                                          "child-model", list_model,
                                          NULL);
}

NMListItem *
nm_status_model_get_active_item (NMStatusModel *self)
{
    NMListItem *item = NULL;
    GtkTreeIter iter;

    g_return_val_if_fail (NM_IS_STATUS_MODEL (self), NULL);

    if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (self), &iter))
        gtk_tree_model_get (GTK_TREE_MODEL (self), &iter, NM_LIST_MODEL_COL_ITEM, &item, -1);

    return item;
}

static gboolean
model_changed_cb (gpointer data)
{
    NMStatusModel *self = NM_STATUS_MODEL (data);
    NMListItem *item;

    item = nm_status_model_get_active_item (self);
    g_signal_emit (self, signals[CHANGED], 0, item);
    g_object_unref (item);

    return FALSE;
}

static void
model_changed (NMStatusModel *self)
{
    GSource *source;

    source = g_idle_source_new ();
    g_source_set_closure (source, g_cclosure_new_object (G_CALLBACK (model_changed_cb), G_OBJECT (self)));
    g_source_attach (source, NULL);
    g_source_unref (source);
}

static gboolean
row_visible_func (GtkTreeModel *model,
                  GtkTreeIter  *iter,
                  gpointer      data)
{
    NMListItem *item = NULL;
    gboolean visible = FALSE;

    gtk_tree_model_get (model, iter, NM_LIST_MODEL_COL_ITEM, &item, -1);
    if (item) {
        NMListItemStatus status;

        status = nm_list_item_get_status (item);
        g_object_unref (item);

        if (status == NM_LIST_ITEM_STATUS_CONNECTED || status == NM_LIST_ITEM_STATUS_CONNECTING)
            visible = TRUE;
    }

    return visible;
}

/*****************************************************************************/

static void
nm_status_model_init (NMStatusModel *self)
{
    gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (self),
                                            row_visible_func, NULL, NULL);

    g_signal_connect_swapped (self, "row-deleted", G_CALLBACK (model_changed), self);
    g_signal_connect_swapped (self, "row-inserted", G_CALLBACK (model_changed), self);
    g_signal_connect_swapped (self, "rows-reordered", G_CALLBACK (model_changed), self);
}

static void
finalize (GObject *object)
{
    g_signal_handlers_disconnect_by_func (object, model_changed, object);

    G_OBJECT_CLASS (nm_status_model_parent_class)->finalize (object);
}

static void
nm_status_model_class_init (NMStatusModelClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = finalize;

    /* Signals */
    signals[CHANGED] =
		g_signal_new ("changed",
					  G_OBJECT_CLASS_TYPE (object_class),
					  G_SIGNAL_RUN_FIRST,
					  G_STRUCT_OFFSET (NMStatusModelClass, changed),
					  NULL, NULL,
					  g_cclosure_marshal_VOID__OBJECT,
					  G_TYPE_NONE, 1,
                      NM_TYPE_LIST_ITEM);
}
