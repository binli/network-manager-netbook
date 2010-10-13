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

#ifndef NMN_NEW_CONNECTION_H
#define NMN_NEW_CONNECTION_H

#include <gtk/gtk.h>
#include "nmn-model.h"

#define NMN_TYPE_NEW_CONNECTION            (nmn_new_connection_get_type ())
#define NMN_NEW_CONNECTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), NMN_TYPE_NEW_CONNECTION, NmnNewConnection))
#define NMN_NEW_CONNECTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), NMN_TYPE_NEW_CONNECTION, NmnNewConnectionClass))
#define NMN_IS_NEW_CONNECTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NMN_TYPE_NEW_CONNECTION))
#define NMN_IS_NEW_CONNECTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), NMN_TYPE_NEW_CONNECTION))
#define NMN_NEW_CONNECTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), NMN_TYPE_NEW_CONNECTION, NmnNewConnectionClass))

#define NMN_NEW_CONNECTION_MODEL "model"

typedef struct {
    GtkVBox parent;
} NmnNewConnection;

typedef struct {
    GtkVBoxClass parent;

    /* Signals */
    void (*close) (NmnNewConnection *self);
} NmnNewConnectionClass;

GType nmn_new_connection_get_type (void);

GtkWidget *nmn_new_connection_create (NmnModel *model);
void       nmn_new_connection_update (NmnNewConnection *self);

#endif /* NMN_NEW_CONNECTION_H */
