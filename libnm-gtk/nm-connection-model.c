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

#include <nm-setting-connection.h>
#include "nm-connection-model.h"

G_DEFINE_TYPE (NMConnectionModel, nm_connection_model, GTK_TYPE_LIST_STORE)

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), NM_TYPE_CONNECTION_MODEL, NMConnectionModelPrivate))

typedef struct {
    GSList *settings;

    gboolean disposed;
} NMConnectionModelPrivate;

static GQuark quark_item_iter = 0;

GtkTreeModel *
nm_connection_model_new (void)
{
    return (GtkTreeModel *) g_object_new (NM_TYPE_CONNECTION_MODEL, NULL);
}

static void
update_iter (GtkListStore *store,
             GtkTreeIter *iter,
             NMSettingsConnectionInterface *connection)
{
    NMSettingConnection *s_con;

    s_con = (NMSettingConnection *) nm_connection_get_setting (NM_CONNECTION (connection),
                                                               NM_TYPE_SETTING_CONNECTION);
    g_return_if_fail (s_con != NULL);

    gtk_list_store_set (store, iter,
                        NM_CONNECTION_MODEL_COL_CONNECTION, connection,
                        NM_CONNECTION_MODEL_COL_NAME,       nm_setting_connection_get_id (s_con),
                        NM_CONNECTION_MODEL_COL_TYPE,       nm_setting_connection_get_connection_type (s_con),
                        -1);
}

static void
connection_updated (NMSettingsConnectionInterface *connection,
                    GHashTable *new_settings,
                    gpointer user_data)
{
    GtkTreeIter *iter;

    iter = (GtkTreeIter *) g_object_get_qdata (G_OBJECT (connection), quark_item_iter);
    if (iter)
        update_iter (GTK_LIST_STORE (user_data), iter, connection);
}

static void
connection_removed (NMSettingsConnectionInterface *connection,
                    gpointer user_data)
{
    GtkListStore *store = GTK_LIST_STORE (user_data);
    GtkTreeIter *iter;

    g_signal_handlers_disconnect_by_func (connection, connection_updated, user_data);
    g_signal_handlers_disconnect_by_func (connection, connection_removed, user_data);

    iter = (GtkTreeIter *) g_object_get_qdata (G_OBJECT (connection), quark_item_iter);
    if (iter) {
        gtk_list_store_remove (store, iter);
        g_object_set_qdata (G_OBJECT (connection), quark_item_iter, NULL);
    }
}

static void
connection_added (NMSettingsInterface *settings,
                  NMSettingsConnectionInterface *connection,
                  gpointer user_data)
{
    GtkListStore *store = GTK_LIST_STORE (user_data);
    GtkTreeIter *iter;

    iter = g_slice_new0 (GtkTreeIter);

    gtk_list_store_append (store, iter);
    update_iter (store, iter, connection);

    g_object_set_qdata_full (G_OBJECT (connection), quark_item_iter, iter, (GDestroyNotify) gtk_tree_iter_free);
    g_signal_connect (connection, "updated", G_CALLBACK (connection_updated), user_data);
    g_signal_connect (connection, "removed", G_CALLBACK (connection_removed), user_data);
}

void
nm_connection_model_add_settings (NMConnectionModel *self,
                                  NMSettingsInterface *settings)
{
    NMConnectionModelPrivate *priv;
    GSList *items;
    GSList *iter;

    g_return_if_fail (NM_IS_CONNECTION_MODEL (self));
    g_return_if_fail (NM_IS_SETTINGS_INTERFACE (settings));

    priv = GET_PRIVATE (self);
    priv->settings = g_slist_append (priv->settings, g_object_ref (settings));
    g_signal_connect (settings, "new-connection", G_CALLBACK (connection_added), self);

    items = nm_settings_interface_list_connections (settings);
    for (iter = items; iter; iter = iter->next)
        connection_added (settings, NM_SETTINGS_CONNECTION_INTERFACE (iter->data), self);

    g_slist_free (items);
}

#if 0
static int
sort_callback (GtkTreeModel *model,
               GtkTreeIter *a,
               GtkTreeIter *b,
               gpointer user_data)
{
    GValue value_a = {0, };
    GValue value_b = {0, };
    int result;

    gtk_tree_model_get_value (model, a, NM_CONNECTION_MODEL_COL_ITEM, &value_a);
    gtk_tree_model_get_value (model, b, NM_CONNECTION_MODEL_COL_ITEM, &value_b);

    result = nm_list_item_compare (NM_LIST_ITEM (g_value_get_object (&value_a)),
                                   NM_LIST_ITEM (g_value_get_object (&value_b)));

    g_value_unset (&value_a);
    g_value_unset (&value_b);

    return result;
}
#endif

/*****************************************************************************/

static void
nm_connection_model_init (NMConnectionModel *self)
{
    GType types[NM_CONNECTION_MODEL_N_COLUMNS];

    types[NM_CONNECTION_MODEL_COL_CONNECTION] = NM_TYPE_SETTINGS_CONNECTION_INTERFACE;
    types[NM_CONNECTION_MODEL_COL_NAME]       = G_TYPE_STRING;
    types[NM_CONNECTION_MODEL_COL_TYPE]       = G_TYPE_STRING;

    gtk_list_store_set_column_types (GTK_LIST_STORE (self), NM_CONNECTION_MODEL_N_COLUMNS, types);

#if 0
    gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (self),
                                     NM_CONNECTION_MODEL_COL_ITEM,
                                     sort_callback,
                                     NULL,
                                     NULL);

    gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (self), NM_CONNECTION_MODEL_COL_ITEM, GTK_SORT_ASCENDING);
#endif
}

static void
dispose (GObject *object)
{
    NMConnectionModelPrivate *priv = GET_PRIVATE (object);

    if (!priv->disposed) {
        if (priv->settings) {
            g_slist_foreach (priv->settings, (GFunc) g_object_unref, NULL);
            g_slist_free (priv->settings);
        }

        priv->disposed = TRUE;
    }

    G_OBJECT_CLASS (nm_connection_model_parent_class)->dispose (object);
}

static void
nm_connection_model_class_init (NMConnectionModelClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    quark_item_iter = g_quark_from_static_string ("NMConnectionModel-item-iter");

    g_type_class_add_private (object_class, sizeof (NMConnectionModelPrivate));

    object_class->dispose = dispose;
}
