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

#include <string.h>
#include <glib/gi18n.h>
#include <nm-list-model.h>
#include <nm-list-item.h>
#include <nm-gsm-pin-request-item.h>
#include "nmn-list.h"
#include "nmn-model.h"
#include "nmn-network-renderer.h"
#include "nmn-gsm-pin-request-renderer.h"

G_DEFINE_TYPE (NmnList, nmn_list, GTK_TYPE_VBOX)

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), NMN_TYPE_LIST, NmnListPrivate))

typedef struct {
    GtkTreeModel *model;
    GSList *rows;

    gulong row_deleted_id;
    gulong row_inserted_id;
    gulong rows_reordered_id;
    guint layout_idle_id;

    gboolean disposed;
} NmnListPrivate;

GtkWidget *
nmn_list_new (void)
{
    return GTK_WIDGET (g_object_new (NMN_TYPE_LIST, NULL));
}

GtkWidget *
nmn_list_new_with_model (GtkTreeModel *model)
{
    GtkWidget *list;

    list = nmn_list_new ();
    if (list)
        nmn_list_set_model (NMN_LIST (list), model);

    return list;
}

static const char *
get_punctuation (guint items_left)
{
    if (items_left > 1)
        return ", ";
    if (items_left == 1)
        return _(" and ");

    /* items_left == 0 */
    return ".";
}

static char *
get_empty_text (NmnModel *model)
{
    GString *string;

    string = g_string_new (_("Sorry, we can't find any networks."));

    if (nmn_model_offline_mode_get_active (model)) {
        g_string_append (string, _(" You could try disabling Offline mode."));
    } else {
        gboolean wifi_enabled;
        gboolean ethernet_enabled;
        gboolean modem_enabled;
        guint disabled_count = 0;

        wifi_enabled = nmn_model_wifi_get_active (model);
        if (!wifi_enabled)
            disabled_count++;

        ethernet_enabled = nmn_model_ethernet_get_active (model);
        if (!ethernet_enabled)
            disabled_count++;

        modem_enabled = nmn_model_modems_get_active (model);
        if (!modem_enabled)
            disabled_count++;

        if (disabled_count > 0) {
            g_string_append (string, _(" You could try turning on "));

            if (!wifi_enabled) {
                g_string_append (string, _("WiFi"));
                g_string_append (string, get_punctuation (--disabled_count));
            }

            if (!ethernet_enabled) {
                g_string_append (string, _("Wired"));
                g_string_append (string, get_punctuation (--disabled_count));
            }

            if (!modem_enabled) {
                g_string_append (string, _("3G"));
                g_string_append (string, get_punctuation (--disabled_count));
            }
        }
    }

    return g_string_free (string, FALSE);
}

static void
nmn_list_clear (NmnList *self)
{
    GtkContainer *container = GTK_CONTAINER (self);
    GList *list;
    GList *iter;

    list = gtk_container_get_children (container);
    for (iter = list; iter; iter = iter->next)
        gtk_container_remove (container, GTK_WIDGET (iter->data));

    g_list_free (list);
}

static void
switches_flipped (NmnList *self)
{
    NmnListPrivate *priv = GET_PRIVATE (self);
    char *txt, *s;
    GtkWidget *w;

    if (priv->rows || !NMN_IS_MODEL (priv->model))
        return;

    nmn_list_clear (self);
    
    txt = get_empty_text (NMN_MODEL (priv->model));
    s = g_strdup_printf ("<big><b>%s</b></big>", txt);
    g_free (txt);

    w = gtk_label_new (NULL);
    gtk_misc_set_alignment (GTK_MISC (w), 0.0, 0.5);
    gtk_label_set_markup (GTK_LABEL (w), s);
    g_free (s);

    gtk_box_pack_start (GTK_BOX (self), w, FALSE, FALSE, 0);
    gtk_widget_show (w);
}

static void
nmn_list_layout (NmnList *self)
{
    NmnListPrivate *priv = GET_PRIVATE (self);
    GSList *iter;

    if (priv->layout_idle_id) {
        g_source_remove (priv->layout_idle_id);
        priv->layout_idle_id = 0;
    }

    /* FIXME: This could probably be much more efficient, especially for cases where
       a single item is added to the end of the list or a single item changes it's location */

    nmn_list_clear (self);
    switches_flipped (self);

    for (iter = priv->rows; iter; iter = iter->next) {
        NmnItemRenderer *renderer = iter->data;
        NMListItem *item;

        item = nmn_item_renderer_get_item (renderer);
        if (!item) {
            GtkTreeIter iter;

            if (gtk_tree_model_get_flags (priv->model) & GTK_TREE_MODEL_ITERS_PERSIST)
                iter = renderer->iter;
            else {
                GtkTreePath *path;

                path = gtk_tree_path_new_from_indices (renderer->index, -1);
                gtk_tree_model_get_iter (priv->model, &iter, path);
                gtk_tree_path_free (path);
            }

            gtk_tree_model_get (priv->model, &iter, NM_LIST_MODEL_COL_ITEM, &item, -1);
            if (item) {
                nmn_item_renderer_set_item (renderer, item);
                g_object_unref (item);
            }
        }

        if (item) {
            gtk_box_pack_start (GTK_BOX (self), GTK_WIDGET (renderer), FALSE, FALSE, 0);
            gtk_widget_show (GTK_WIDGET (renderer));
        }
    }
}

static gboolean
queue_layout_cb (gpointer data)
{
    NmnListPrivate *priv = GET_PRIVATE (data);

    priv->layout_idle_id = 0;
    nmn_list_layout (NMN_LIST (data));

    return FALSE;
}

static void
queue_layout (NmnList *self)
{
    NmnListPrivate *priv = GET_PRIVATE (self);

    if (!priv->layout_idle_id)
        priv->layout_idle_id = g_idle_add (queue_layout_cb, self);
}

static void
model_row_deleted (GtkTreeModel *model,
                   GtkTreePath *path,
                   gpointer user_data)
{
    NmnList *self = NMN_LIST (user_data);
    NmnListPrivate *priv = GET_PRIVATE (self);
    GSList *list;
    GSList *next;
    NmnItemRenderer *renderer;
    int index;

    index = gtk_tree_path_get_indices(path)[0];
    list = g_slist_nth (priv->rows, index);
    renderer = list->data;
    g_object_unref (renderer);

    for (next = list->next; next; next = next->next) {
        renderer = next->data;
        renderer->index--;
    }
  
    priv->rows = g_slist_delete_link (priv->rows, list);
    queue_layout (self);
}

static void
model_row_inserted (GtkTreeModel *model,
                    GtkTreePath *path,
                    GtkTreeIter *iter,
                    gpointer user_data)
{
    NmnList *self = NMN_LIST (user_data);
    NmnListPrivate *priv = GET_PRIVATE (self);
    NmnItemRenderer *renderer;
    GSList *list;
    NMListItem *item;
    int index;

    index = gtk_tree_path_get_indices (path)[0];

    gtk_tree_model_get (model, iter, NM_LIST_MODEL_COL_ITEM, &item, -1);

    if (item && NM_IS_GSM_PIN_REQUEST_ITEM (item))
        renderer = (NmnItemRenderer *) nmn_gsm_pin_request_renderer_new ();
    else
        renderer = (NmnItemRenderer *) nmn_network_renderer_new ();

    renderer->index = index;

    if (gtk_tree_model_get_flags (model) & GTK_TREE_MODEL_ITERS_PERSIST)
        renderer->iter = *iter;

    if (item) {
        nmn_item_renderer_set_item (renderer, item);
        g_object_unref (item);
    }

    priv->rows = g_slist_insert (priv->rows, g_object_ref_sink (renderer), index);
  
    list = g_slist_nth (priv->rows, index + 1);
    for (; list; list = list->next) {
        renderer = list->data;
        renderer->index++;
    }

    queue_layout (self);
}

static void
model_rows_reordered (GtkTreeModel *model,
                      GtkTreePath *path,
                      GtkTreeIter *iter,
                      gint *new_order,
                      gpointer user_data)
{
    NmnList *self = NMN_LIST (user_data);
    NmnListPrivate *priv = GET_PRIVATE (self);
    GSList *list;
    NmnItemRenderer **renderer_array;
    GSList *rows = NULL;
    gint *order;
    int length;
    int i;

    length = gtk_tree_model_iter_n_children (model, NULL);

    order = g_new (int, length);
    for (i = 0; i < length; i++)
        order[new_order[i]] = i;

    renderer_array = g_new (NmnItemRenderer *, length);
    for (i = 0, list = priv->rows; list != NULL; list = list->next, i++)
        renderer_array[order[i]] = list->data;
    g_free (order);

    for (i = length - 1; i >= 0; i--) {
        renderer_array[i]->index = i;
        rows = g_slist_prepend (rows, renderer_array[i]);
    }
  
    g_free (renderer_array);
    g_slist_free (priv->rows);
    priv->rows = rows;

    queue_layout (self);
}

static gboolean
model_foreach_cb (GtkTreeModel *model,
                  GtkTreePath *path,
                  GtkTreeIter *iter,
                  gpointer user_data)
{
    model_row_inserted (model, path, iter, user_data);

    return FALSE;
}

void
nmn_list_set_model (NmnList *list,
                    GtkTreeModel *model)
{
    NmnListPrivate *priv;

    g_return_if_fail (NMN_IS_LIST (list));

    priv = GET_PRIVATE (list);

    if (model == priv->model)
        return;

    if (priv->model) {
        if (NMN_IS_MODEL (priv->model))
            g_signal_handlers_disconnect_by_func (priv->model, switches_flipped, list);

        g_signal_handler_disconnect (priv->model, priv->row_deleted_id);
        g_signal_handler_disconnect (priv->model, priv->row_inserted_id);
        g_signal_handler_disconnect (priv->model, priv->rows_reordered_id);

        g_object_unref (priv->model);
    }

    priv->model = model;

    if (priv->model) {
        g_object_ref (priv->model);

        priv->row_deleted_id = g_signal_connect (model, "row-deleted", G_CALLBACK (model_row_deleted), list);
        priv->row_inserted_id = g_signal_connect (model, "row-inserted", G_CALLBACK (model_row_inserted), list);
        priv->rows_reordered_id = g_signal_connect (model, "rows-reordered", G_CALLBACK (model_rows_reordered), list);

        gtk_tree_model_foreach (model, model_foreach_cb, list);

        if (NMN_IS_MODEL (model)) {
            g_signal_connect_swapped (model, "ethernet-toggled",     G_CALLBACK (switches_flipped), list);
            g_signal_connect_swapped (model, "wifi-toggled",         G_CALLBACK (switches_flipped), list);
            g_signal_connect_swapped (model, "modems-toggled",       G_CALLBACK (switches_flipped), list);
            g_signal_connect_swapped (model, "bt-toggled",           G_CALLBACK (switches_flipped), list);
            g_signal_connect_swapped (model, "offline-mode-toggled", G_CALLBACK (switches_flipped), list);
        }
    }
}

GtkTreeModel *
nmn_list_get_model (NmnList *list)
{
    g_return_val_if_fail (NMN_IS_LIST (list), NULL);

    return GET_PRIVATE (list)->model;
}

/*****************************************************************************/

static void
nmn_list_init (NmnList *list)
{
    gtk_box_set_homogeneous (GTK_BOX (list), FALSE);
    gtk_box_set_spacing (GTK_BOX (list), 6);
    gtk_container_set_border_width (GTK_CONTAINER (list), 6);
}

static void
dispose (GObject *object)
{
    NmnListPrivate *priv = GET_PRIVATE (object);

    if (!priv->disposed) {
        if (priv->layout_idle_id)
            g_source_remove (priv->layout_idle_id);

        g_slist_foreach (priv->rows, (GFunc) g_object_unref, NULL);
        g_slist_free (priv->rows);

        nmn_list_set_model (NMN_LIST (object), NULL);

        priv->disposed = TRUE;
    }

    G_OBJECT_CLASS (nmn_list_parent_class)->dispose (object);
}

static void
nmn_list_class_init (NmnListClass *class)
{
    GObjectClass *object_class = G_OBJECT_CLASS (class);

    g_type_class_add_private (object_class, sizeof (NmnListPrivate));

    object_class->dispose = dispose;
}
