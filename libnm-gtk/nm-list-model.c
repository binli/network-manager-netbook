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

#include "nm-list-model.h"
#include "nm-list-item.h"
#include "nm-device-handler.h"

G_DEFINE_TYPE (NMListModel, nm_list_model, GTK_TYPE_LIST_STORE)

enum {
    PROP_0,
    PROP_CLIENT,

    LAST_PROP
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), NM_TYPE_LIST_MODEL, NMListModelPrivate))

typedef struct {
    NMClient *client;
    GSList *settings;
    NMDeviceHandler *device_handler;

    gboolean disposed;
} NMListModelPrivate;

static GQuark quark_item_iter = 0;

NMListModel *
nm_list_model_new (NMClient *client)
{
    g_return_val_if_fail (NM_IS_CLIENT (client), NULL);

    return (NMListModel *) g_object_new (NM_TYPE_LIST_MODEL,
                                         NM_LIST_MODEL_CLIENT, client,
                                         NULL);
}

void
nm_list_model_add_settings (NMListModel *self,
                            NMSettingsInterface *settings)
{
    NMListModelPrivate *priv;

    g_return_if_fail (NM_IS_LIST_MODEL (self));
    g_return_if_fail (NM_IS_SETTINGS_INTERFACE (settings));

    priv = GET_PRIVATE (self);
    priv->settings = g_slist_append (priv->settings, g_object_ref (settings));

    g_slist_foreach (nm_device_handler_get_providers (priv->device_handler),
                     (GFunc) nm_item_provider_add_settings, settings);
}

NMClient *
nm_list_model_get_client (NMListModel *self)
{
    g_return_val_if_fail (NM_IS_LIST_MODEL (self), NULL);

    return GET_PRIVATE (self)->client;
}

static void
item_added (NMItemProvider *provider,
            NMListItem *item,
            gpointer user_data)
{
    GtkListStore *store = GTK_LIST_STORE (user_data);
    GtkTreeIter *iter;

    iter = g_slice_new0 (GtkTreeIter);
    gtk_list_store_insert_with_values (store, iter, G_MAXINT,
                                       NM_LIST_MODEL_COL_ITEM,        item,
                                       NM_LIST_MODEL_COL_NAME,        nm_list_item_get_name (item),
                                       NM_LIST_MODEL_COL_ICON,        nm_list_item_get_icon (item),
                                       NM_LIST_MODEL_COL_SECURITY,    nm_list_item_get_security (item),
                                       NM_LIST_MODEL_COL_STATUS,      nm_list_item_get_status (item),
                                       NM_LIST_MODEL_COL_SHOW_DELETE, nm_list_item_get_show_delete (item),
                                       -1);

    g_object_set_qdata_full (G_OBJECT (item), quark_item_iter, iter, (GDestroyNotify) gtk_tree_iter_free);
}

static void
item_changed (NMItemProvider *provider,
              NMListItem *item,
              gpointer user_data)
{
    GtkListStore *store = GTK_LIST_STORE (user_data);
    GtkTreeIter *iter;

    iter = (GtkTreeIter *) g_object_get_qdata (G_OBJECT (item), quark_item_iter);
    if (iter)
        gtk_list_store_set (store, iter,
                            NM_LIST_MODEL_COL_ITEM,        item,
                            NM_LIST_MODEL_COL_NAME,        nm_list_item_get_name (item),
                            NM_LIST_MODEL_COL_ICON,        nm_list_item_get_icon (item),
                            NM_LIST_MODEL_COL_SECURITY,    nm_list_item_get_security (item),
                            NM_LIST_MODEL_COL_STATUS,      nm_list_item_get_status (item),
                            NM_LIST_MODEL_COL_SHOW_DELETE, nm_list_item_get_show_delete (item),
                            -1);
}

static void
item_removed (NMItemProvider *provider,
              NMListItem *item,
              gpointer user_data)
{
    GtkListStore *store = GTK_LIST_STORE (user_data);
    GtkTreeIter *iter;

    iter = (GtkTreeIter *) g_object_get_qdata (G_OBJECT (item), quark_item_iter);
    if (iter) {
        gtk_list_store_remove (store, iter);
        g_object_set_qdata (G_OBJECT (item), quark_item_iter, NULL);
    }
}

static void
device_provider_added (NMDeviceHandler *handler,
                       NMItemProvider *provider,
                       gpointer user_data)
{
    NMListModelPrivate *priv = GET_PRIVATE (user_data);
    GSList *list;
    GSList *iter;

    g_signal_connect (provider, "item-added", G_CALLBACK (item_added), user_data);
    g_signal_connect (provider, "item-changed", G_CALLBACK (item_changed), user_data);
    g_signal_connect (provider, "item-removed", G_CALLBACK (item_removed), user_data);

    list = nm_item_provider_get_items (provider);
    for (iter = list; iter; iter = iter->next)
        item_added (provider, (NMListItem *) iter->data, user_data);

    for (iter = priv->settings; iter; iter = iter->next)
        nm_item_provider_add_settings (provider, (NMSettingsInterface *) iter->data);
}

static void
device_provider_removed (NMDeviceHandler *handler,
                         NMItemProvider *provider,
                         gpointer user_data)
{
    g_signal_handlers_disconnect_matched (provider, G_SIGNAL_MATCH_DATA,
                                          0, 0, NULL, NULL, user_data);
}

static int
sort_callback (GtkTreeModel *model,
               GtkTreeIter *a,
               GtkTreeIter *b,
               gpointer user_data)
{
    NMListItem *item_a;
    NMListItem *item_b;
    int result;

    gtk_tree_model_get (model, a, NM_LIST_MODEL_COL_ITEM, &item_a, -1);
    gtk_tree_model_get (model, b, NM_LIST_MODEL_COL_ITEM, &item_b, -1);

    result = nm_list_item_compare (item_a, item_b);

    if (item_a)
        g_object_unref (item_a);

    if (item_b)
        g_object_unref (item_b);

    return result;
}

/*****************************************************************************/

static void
nm_list_model_init (NMListModel *self)
{
    GType types[NM_LIST_MODEL_N_COLUMNS];

    types[NM_LIST_MODEL_COL_ITEM]        = NM_TYPE_LIST_ITEM;
    types[NM_LIST_MODEL_COL_NAME]        = G_TYPE_STRING;
    types[NM_LIST_MODEL_COL_ICON]        = G_TYPE_STRING;
    types[NM_LIST_MODEL_COL_SECURITY]    = G_TYPE_STRING;
    types[NM_LIST_MODEL_COL_STATUS]      = G_TYPE_INT;
    types[NM_LIST_MODEL_COL_SHOW_DELETE] = G_TYPE_BOOLEAN;

    gtk_list_store_set_column_types (GTK_LIST_STORE (self), NM_LIST_MODEL_N_COLUMNS, types);

    gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (self),
                                     NM_LIST_MODEL_COL_ITEM,
                                     sort_callback,
                                     NULL,
                                     NULL);

    gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (self), NM_LIST_MODEL_COL_ITEM, GTK_SORT_ASCENDING);
}

static void
set_property (GObject *object, guint prop_id,
              const GValue *value, GParamSpec *pspec)
{
    NMListModelPrivate *priv = GET_PRIVATE (object);

    switch (prop_id) {
    case PROP_CLIENT:
        /* Construct only */
        priv->client = g_value_dup_object (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
get_property (GObject *object, guint prop_id,
              GValue *value, GParamSpec *pspec)
{
    NMListModelPrivate *priv = GET_PRIVATE (object);

    switch (prop_id) {
    case PROP_CLIENT:
        g_value_set_object (value, priv->client);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
constructed (GObject *object)
{
    NMListModelPrivate *priv = GET_PRIVATE (object);
    GSList *list;
    GSList *iter;

    if (G_OBJECT_CLASS (nm_list_model_parent_class)->constructed)
        G_OBJECT_CLASS (nm_list_model_parent_class)->constructed (object);

    priv->device_handler = nm_device_handler_new (priv->client);
    g_signal_connect (priv->device_handler, "provider-added", G_CALLBACK (device_provider_added), object);
    g_signal_connect (priv->device_handler, "provider-removed", G_CALLBACK (device_provider_removed), object);

    list = nm_device_handler_get_providers (priv->device_handler);
    for (iter = list; iter; iter = iter->next)
        device_provider_added (priv->device_handler, NM_ITEM_PROVIDER (iter->data), object);

    /* FIXME: create VPNProvider */
}

static void
dispose (GObject *object)
{
    NMListModelPrivate *priv = GET_PRIVATE (object);

    if (!priv->disposed) {
        if (priv->device_handler)
            g_object_unref (priv->device_handler);

        if (priv->settings) {
            g_slist_foreach (priv->settings, (GFunc) g_object_unref, NULL);
            g_slist_free (priv->settings);
            priv->settings = NULL;
        }

        if (priv->client)
            g_object_unref (priv->client);

        priv->disposed = TRUE;
    }

    G_OBJECT_CLASS (nm_list_model_parent_class)->dispose (object);
}

static void
nm_list_model_class_init (NMListModelClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    quark_item_iter = g_quark_from_static_string ("NMListModel-item-iter");

    g_type_class_add_private (object_class, sizeof (NMListModelPrivate));

    object_class->set_property = set_property;
    object_class->get_property = get_property;
    object_class->constructed = constructed;
    object_class->dispose = dispose;

    /* Properties */
    g_object_class_install_property
        (object_class, PROP_CLIENT,
         g_param_spec_object (NM_LIST_MODEL_CLIENT,
                              "NMClient",
                              "NMClient",
                              NM_TYPE_CLIENT,
                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}
