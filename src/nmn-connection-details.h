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

#ifndef NMN_CONNECTION_DETAILS_H
#define NMN_CONNECTION_DETAILS_H

#include <gtk/gtk.h>
#include <nm-setting-ip4-config.h>
#include <nm-ip4-config.h>

#define NMN_TYPE_CONNECTION_DETAILS            (nmn_connection_details_get_type ())
#define NMN_CONNECTION_DETAILS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), NMN_TYPE_CONNECTION_DETAILS, NmnConnectionDetails))
#define NMN_CONNECTION_DETAILS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), NMN_TYPE_CONNECTION_DETAILS, NmnConnectionDetailsClass))
#define NMN_IS_CONNECTION_DETAILS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NMN_TYPE_CONNECTION_DETAILS))
#define NMN_IS_CONNECTION_DETAILS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), NMN_TYPE_CONNECTION_DETAILS))
#define NMN_CONNECTION_DETAILS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), NMN_TYPE_CONNECTION_DETAILS, NmnConnectionDetailsClass))

typedef struct {
    GtkTable parent;
} NmnConnectionDetails;

typedef struct {
    GtkTableClass parent;

    /* Signals */
    void (*changed) (NmnConnectionDetails *self,
                     gboolean complete);
} NmnConnectionDetailsClass;

GType nmn_connection_details_get_type (void);

NmnConnectionDetails *nmn_connection_details_new      (void);
void                  nmn_connection_details_set_data (NmnConnectionDetails *self,
                                                       NMSettingIP4Config *setting,
                                                       NMIP4Config *config,
                                                       const char *hw_address);

NMSettingIP4Config   *nmn_connection_details_get_data (NmnConnectionDetails *self);
gboolean              nmn_connection_details_verify   (NmnConnectionDetails *self);


#endif /* NMN_CONNECTION_DETAILS_H */
