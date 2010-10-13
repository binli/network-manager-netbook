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
#include "nm-item-provider.h"

G_DEFINE_ABSTRACT_TYPE (NMItemProvider, nm_item_provider, G_TYPE_OBJECT)

enum {
    PROP_0,
    PROP_CLIENT,

    LAST_PROP
};

enum {
	ITEM_ADDED,
    ITEM_CHANGED,
	ITEM_REMOVED,

	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), NM_TYPE_ITEM_PROVIDER, NMItemProviderPrivate))

typedef struct {
    NMClient *client;
    GSList *settings;

    GSList *items;

    gboolean disposed;
} NMItemProviderPrivate;

static void
item_changed (NMListItem *item,
              GParamSpec *pspec,
              gpointer user_data)
{
    g_signal_emit (user_data, signals[ITEM_CHANGED], 0, item);
}

static void
remove_item (NMListItem *item,
             gpointer user_data)
{
    NMItemProvider *self = NM_ITEM_PROVIDER (user_data);
    NMItemProviderPrivate *priv = GET_PRIVATE (self);
    GSList *link;

    link = g_slist_find (priv->items, item);
    if (link) {
        g_signal_handlers_disconnect_matched (item, G_SIGNAL_MATCH_DATA,
                                              0, 0, NULL, NULL, user_data);

        priv->items = g_slist_delete_link (priv->items, link);
        g_signal_emit (self, signals[ITEM_REMOVED], 0, item);
        g_object_unref (item);
    }
}

void
nm_item_provider_item_added (NMItemProvider *provider,
                             NMListItem *item)
{
    NMItemProviderPrivate *priv;

    g_return_if_fail (NM_IS_ITEM_PROVIDER (provider));
    g_return_if_fail (NM_IS_LIST_ITEM (item));

    priv = GET_PRIVATE (provider);
    priv->items = g_slist_prepend (priv->items, g_object_ref_sink (item));
    g_signal_connect (item, "request-remove", G_CALLBACK (remove_item), provider);
    g_signal_connect (item, "notify", G_CALLBACK (item_changed), provider);

    g_signal_emit (provider, signals[ITEM_ADDED], 0, item);
}

static void
connection_added (NMSettingsInterface *settings,
                  NMSettingsConnectionInterface *connection,
                  gpointer data)
{
    NMItemProvider *self = NM_ITEM_PROVIDER (data);

    if (NM_ITEM_PROVIDER_GET_CLASS (self)->connection_added)
        NM_ITEM_PROVIDER_GET_CLASS (self)->connection_added (self, connection);
}

static void
add_settings_connections (NMSettingsInterface *settings,
                          NMItemProvider *self)
{
    GSList *items;
    GSList *iter;

    items = nm_settings_interface_list_connections (settings);
    for (iter = items; iter; iter = iter->next)
        connection_added (settings, NM_SETTINGS_CONNECTION_INTERFACE (iter->data), self);

    g_slist_free (items);
}

void
nm_item_provider_add_settings (NMItemProvider *self,
                               NMSettingsInterface *settings)
{
    NMItemProviderPrivate *priv;

    g_return_if_fail (NM_IS_ITEM_PROVIDER (self));
    g_return_if_fail (NM_IS_SETTINGS_INTERFACE (settings));

    priv = GET_PRIVATE (self);

    priv->settings = g_slist_prepend (priv->settings, g_object_ref (settings));
    g_signal_connect (settings, "new-connection", G_CALLBACK (connection_added), self);
    add_settings_connections (settings, self);
}

NMClient *
nm_item_provider_get_client (NMItemProvider *self)
{
    g_return_val_if_fail (NM_IS_ITEM_PROVIDER (self), NULL);

    return GET_PRIVATE (self)->client;
}

GSList *
nm_item_provider_get_connections (NMItemProvider *self)
{
    NMItemProviderPrivate *priv;
    GSList *iter;
    GSList *list = NULL;

    g_return_val_if_fail (NM_IS_ITEM_PROVIDER (self), NULL);

    priv = GET_PRIVATE (self);
    for (iter = priv->settings; iter; iter = iter->next)
        list = g_slist_concat (list, nm_settings_interface_list_connections (NM_SETTINGS_INTERFACE (iter->data)));

    return list;
}

GSList *
nm_item_provider_get_items (NMItemProvider *self)
{
    g_return_val_if_fail (NM_IS_ITEM_PROVIDER (self), NULL);

    return GET_PRIVATE (self)->items;
}

void
nm_item_provider_clear (NMItemProvider *self)
{
    NMItemProviderPrivate *priv;

    g_return_if_fail (NM_IS_ITEM_PROVIDER (self));

    priv = GET_PRIVATE (self);
    g_slist_foreach (priv->items, (GFunc) remove_item, self);
}

void
nm_item_provider_reset (NMItemProvider *self)
{
    NMItemProviderPrivate *priv;

    g_return_if_fail (NM_IS_ITEM_PROVIDER (self));

    priv = GET_PRIVATE (self);
    g_slist_foreach (priv->settings, (GFunc) add_settings_connections, self);
}

/*****************************************************************************/

static void
nm_item_provider_init (NMItemProvider *self)
{
}

static void
set_property (GObject *object, guint prop_id,
              const GValue *value, GParamSpec *pspec)
{
    NMItemProviderPrivate *priv = GET_PRIVATE (object);

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
    NMItemProviderPrivate *priv = GET_PRIVATE (object);

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
dispose (GObject *object)
{
    NMItemProviderPrivate *priv = GET_PRIVATE (object);

    if (!priv->disposed) {
        GSList *iter;

        nm_item_provider_clear (NM_ITEM_PROVIDER (object));

        for (iter = priv->settings; iter; iter = iter->next) {
            GObject *settings = iter->data;

            g_signal_handlers_disconnect_by_func (settings, connection_added, object);
            g_object_unref (settings);
        }
        g_slist_free (priv->settings);

        if (priv->client)
            g_object_unref (priv->client);

        priv->disposed = TRUE;
    }

    G_OBJECT_CLASS (nm_item_provider_parent_class)->dispose (object);
}

static void
nm_item_provider_class_init (NMItemProviderClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (NMItemProviderPrivate));

    object_class->set_property = set_property;
    object_class->get_property = get_property;
    object_class->dispose = dispose;

    /* Properties */
    g_object_class_install_property
        (object_class, PROP_CLIENT,
         g_param_spec_object (NM_ITEM_PROVIDER_CLIENT,
                              "NMClient",
                              "NMClient",
                              NM_TYPE_CLIENT,
                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    signals[ITEM_ADDED] =
		g_signal_new ("item-added",
					  G_OBJECT_CLASS_TYPE (object_class),
					  G_SIGNAL_RUN_FIRST,
					  G_STRUCT_OFFSET (NMItemProviderClass, item_added),
					  NULL, NULL,
					  g_cclosure_marshal_VOID__OBJECT,
					  G_TYPE_NONE, 1,
					  NM_TYPE_LIST_ITEM);

    /* Signals */
    signals[ITEM_CHANGED] =
		g_signal_new ("item-changed",
					  G_OBJECT_CLASS_TYPE (object_class),
					  G_SIGNAL_RUN_FIRST,
					  G_STRUCT_OFFSET (NMItemProviderClass, item_changed),
					  NULL, NULL,
					  g_cclosure_marshal_VOID__OBJECT,
					  G_TYPE_NONE, 1,
					  NM_TYPE_LIST_ITEM);

    signals[ITEM_REMOVED] =
		g_signal_new ("item-removed",
					  G_OBJECT_CLASS_TYPE (object_class),
					  G_SIGNAL_RUN_FIRST,
					  G_STRUCT_OFFSET (NMItemProviderClass, item_removed),
					  NULL, NULL,
					  g_cclosure_marshal_VOID__OBJECT,
					  G_TYPE_NONE, 1,
					  NM_TYPE_LIST_ITEM);
}
