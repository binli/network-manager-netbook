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

#ifndef NM_LIST_ITEM_H
#define NM_LIST_ITEM_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define NM_TYPE_LIST_ITEM            (nm_list_item_get_type ())
#define NM_LIST_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), NM_TYPE_LIST_ITEM, NMListItem))
#define NM_LIST_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), NM_TYPE_LIST_ITEM, NMListItemClass))
#define NM_IS_LIST_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NM_TYPE_LIST_ITEM))
#define NM_IS_LIST_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), NM_TYPE_LIST_ITEM))
#define NM_LIST_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), NM_TYPE_LIST_ITEM, NMListItemClass))

typedef enum {
    NM_LIST_ITEM_STATUS_DISCONNECTED,
    NM_LIST_ITEM_STATUS_CONNECTING,
    NM_LIST_ITEM_STATUS_CONNECTED
} NMListItemStatus;

#define NM_LIST_ITEM_NAME        "name"
#define NM_LIST_ITEM_TYPE_NAME   "type-name"
#define NM_LIST_ITEM_ICON        "icon"
#define NM_LIST_ITEM_SECURITY    "security"
#define NM_LIST_ITEM_STATUS      "status"
#define NM_LIST_ITEM_SHOW_DELETE "show-delete"

/* Sorting priorities. These are summed and the items are sorted
   in ascending numeric order (higher first) */
#define NM_LIST_ITEM_PRIORITY_DEFAULT_ROUTE 1000
#define NM_LIST_ITEM_PRIORITY_ACTIVATED     200
#define NM_LIST_ITEM_PRIORITY_CONFIGURED    100
#define NM_LIST_ITEM_PRIORITY_DEV_ETHERNET  99
#define NM_LIST_ITEM_PRIORITY_DEV_WIFI      98
#define NM_LIST_ITEM_PRIORITY_DEV_GSM       97
#define NM_LIST_ITEM_PRIORITY_DEV_CDMA      96
#define NM_LIST_ITEM_PRIORITY_DEV_BT        95

typedef struct {
    GInitiallyUnowned parent;
} NMListItem;

typedef struct {
    GInitiallyUnownedClass parent_class;

    /* Methods */
    void (*connect)    (NMListItem *self);
    void (*disconnect) (NMListItem *self);
    void (*delete)     (NMListItem *self);
    int  (*priority)   (NMListItem *self);

    /* Signals */
    void (*request_remove) (NMListItem *self);
    void (*warning)    (NMListItem *self,
                        const char *message);
} NMListItemClass;

GType nm_list_item_get_type (void);

const char       *nm_list_item_get_name        (NMListItem *self);
const char       *nm_list_item_get_type_name   (NMListItem *self);
const char       *nm_list_item_get_icon        (NMListItem *self);
const char       *nm_list_item_get_security    (NMListItem *self);
NMListItemStatus  nm_list_item_get_status      (NMListItem *self);
gboolean          nm_list_item_get_show_delete (NMListItem *self);

void              nm_list_item_request_remove  (NMListItem *self);

void              nm_list_item_connect         (NMListItem *self);
void              nm_list_item_disconnect      (NMListItem *self);
void              nm_list_item_delete          (NMListItem *self);
int               nm_list_item_compare         (NMListItem *self,
                                                NMListItem *other);

void              nm_list_item_warning         (NMListItem *self,
                                                const char *message);

G_END_DECLS

#endif /* NM_LIST_ITEM_H */
