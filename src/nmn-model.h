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

#ifndef NMN_MODEL_H
#define NMN_MODEL_H

#include <gtk/gtk.h>
#include <nm-client.h>
#include <nm-settings-interface.h>

#define NMN_TYPE_MODEL            (nmn_model_get_type ())
#define NMN_MODEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), NMN_TYPE_MODEL, NmnModel))
#define NMN_MODEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), NMN_TYPE_MODEL, NmnModelClass))
#define NMN_IS_MODEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NMN_TYPE_MODEL))
#define NMN_IS_MODEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), NMN_TYPE_MODEL))
#define NMN_MODEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), NMN_TYPE_MODEL, NmnModelClass))

typedef struct {
    GtkTreeModelFilter parent;
} NmnModel;

typedef struct {
    GtkTreeModelFilterClass parent;

    /* Signals */
    void (*ethernet_toggled) (NmnModel *self,
                              gboolean active);

    void (*wifi_toggled) (NmnModel *self,
                          gboolean active);

    void (*modems_toggled) (NmnModel *self,
                            gboolean active);

    void (*bt_toggled) (NmnModel *self,
                        gboolean active);

    void (*offline_mode_toggled) (NmnModel *self,
                                  gboolean active);
} NmnModelClass;

GType nmn_model_get_type (void);

NmnModel *nmn_model_new        (void);
NMClient *nmn_model_get_client (NmnModel *self);

NMSettingsInterface *nmn_model_get_user_settings   (NmnModel *self);
NMSettingsInterface *nmn_model_get_system_settings (NmnModel *self);

gboolean    nmn_model_ethernet_get_active     (NmnModel *self); 
void        nmn_model_ethernet_toggled        (NmnModel *self,
                                               gboolean active);

gboolean    nmn_model_wifi_can_change         (NmnModel *self);
gboolean    nmn_model_wifi_get_active         (NmnModel *self); 
void        nmn_model_wifi_toggled            (NmnModel *self,
                                               gboolean active);

gboolean    nmn_model_modems_get_active       (NmnModel *self); 
void        nmn_model_modems_toggled          (NmnModel *self,
                                               gboolean active);

gboolean    nmn_model_bt_get_active           (NmnModel *self); 
void        nmn_model_bt_toggled              (NmnModel *self,
                                               gboolean active);

gboolean    nmn_model_offline_mode_get_active (NmnModel *self);
void        nmn_model_offline_mode_toggled    (NmnModel *self,
                                               gboolean active);

#endif /* NMN_MODEL_H */
