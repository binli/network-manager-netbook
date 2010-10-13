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

#ifndef NM_ITEM_PROVIDER_H
#define NM_ITEM_PROVIDER_H

#include <glib-object.h>
#include <nm-client.h>
#include <nm-settings-interface.h>
#include <nm-list-item.h>

G_BEGIN_DECLS

#define NM_TYPE_ITEM_PROVIDER            (nm_item_provider_get_type ())
#define NM_ITEM_PROVIDER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), NM_TYPE_ITEM_PROVIDER, NMItemProvider))
#define NM_ITEM_PROVIDER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), NM_TYPE_ITEM_PROVIDER, NMItemProviderClass))
#define NM_IS_ITEM_PROVIDER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NM_TYPE_ITEM_PROVIDER))
#define NM_IS_ITEM_PROVIDER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), NM_TYPE_ITEM_PROVIDER))
#define NM_ITEM_PROVIDER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), NM_TYPE_ITEM_PROVIDER, NMItemProviderClass))

#define NM_ITEM_PROVIDER_CLIENT "client"

typedef struct {
    GObject parent;
} NMItemProvider;

typedef struct {
    GObjectClass parent_class;

    /* Methods */
    void (*connection_added) (NMItemProvider *self,
                              NMSettingsConnectionInterface *connection);

    /* Signals */
    void (*item_added) (NMItemProvider *self,
                        NMListItem *item);

    void (*item_changed) (NMItemProvider *self,
                          NMListItem *item);

    void (*item_removed) (NMItemProvider *self,
                          NMListItem *item);
} NMItemProviderClass;

GType nm_item_provider_get_type (void);

void      nm_item_provider_add_settings (NMItemProvider *self,
                                         NMSettingsInterface *settings);

NMClient *nm_item_provider_get_client   (NMItemProvider *self);
GSList   *nm_item_provider_get_connections (NMItemProvider *self);

GSList   *nm_item_provider_get_items    (NMItemProvider *self);

void      nm_item_provider_item_added   (NMItemProvider *provider,
                                         NMListItem *item);

void      nm_item_provider_clear        (NMItemProvider *self);
void      nm_item_provider_reset        (NMItemProvider *self);

G_END_DECLS

#endif /* NM_ITEM_PROVIDER_H */
