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

#ifndef NM_CONNECTION_ITEM_H
#define NM_CONNECTION_ITEM_H

#include <glib-object.h>
#include <gio/gio.h>
#include <nm-client.h>
#include <nm-settings-connection-interface.h>
#include <nm-list-item.h>
#include <nm-gconf-connection.h>

G_BEGIN_DECLS

#define NM_TYPE_CONNECTION_ITEM            (nm_connection_item_get_type ())
#define NM_CONNECTION_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), NM_TYPE_CONNECTION_ITEM, NMConnectionItem))
#define NM_CONNECTION_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), NM_TYPE_CONNECTION_ITEM, NMConnectionItemClass))
#define NM_IS_CONNECTION_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NM_TYPE_CONNECTION_ITEM))
#define NM_IS_CONNECTION_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), NM_TYPE_CONNECTION_ITEM))
#define NM_CONNECTION_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), NM_TYPE_CONNECTION_ITEM, NMConnectionItemClass))

#define NM_CONNECTION_ITEM_CLIENT     "client"
#define NM_CONNECTION_ITEM_CONNECTION "connection"
#define NM_CONNECTION_ITEM_DELETE_ALLOWED "delete-allowed"

typedef struct {
    NMListItem parent;
} NMConnectionItem;

typedef struct {
    NMListItemClass parent_class;

    void (*secrets_requested) (NMConnectionItem *self,
                               NMConnection *connection,
                               const char *setting_name,
                               const char **hints,
                               gboolean ask_user,
                               NMNewSecretsRequestedFunc callback,
                               gpointer callback_data);

    NMConnection *(*create_connection) (NMConnectionItem *self);
} NMConnectionItemClass;

GType nm_connection_item_get_type (void);

NMClient                      *nm_connection_item_get_client     (NMConnectionItem *self);
NMSettingsConnectionInterface *nm_connection_item_get_connection (NMConnectionItem *self);
void                           nm_connection_item_set_connection (NMConnectionItem *self,
                                                                  NMSettingsConnectionInterface *connection);

void                           nm_connection_item_new_connection (NMConnectionItem *self,
                                                                  NMConnection *connection,
                                                                  gboolean connect);

void                           nm_connection_item_create_connection (NMConnectionItem *self,
                                                                     GAsyncReadyCallback callback,
                                                                     gpointer user_data);

NMConnection                  *nm_connection_item_create_connection_finish (NMConnectionItem *self,
                                                                            GAsyncResult *result,
                                                                            GError **error);

G_END_DECLS

#endif /* NM_CONNECTION_ITEM_H */
