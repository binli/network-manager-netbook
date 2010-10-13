/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* NetworkManager Wireless Applet -- Display wireless access points and allow user control
 *
 * This program is free software; you can redistribute it and/or modify
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
 * (C) Copyright 2008 Novell, Inc.
 */

#ifndef NM_GCONF_SETTINGS_H
#define NM_GCONF_SETTINGS_H

#include <nm-connection.h>
#include <nm-settings-service.h>

#include "nm-gconf-connection.h"

G_BEGIN_DECLS

#define NM_TYPE_GCONF_SETTINGS            (nm_gconf_settings_get_type ())
#define NM_GCONF_SETTINGS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), NM_TYPE_GCONF_SETTINGS, NMGConfSettings))
#define NM_GCONF_SETTINGS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), NM_TYPE_GCONF_SETTINGS, NMGConfSettingsClass))
#define NM_IS_GCONF_SETTINGS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NM_TYPE_GCONF_SETTINGS))
#define NM_IS_GCONF_SETTINGS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), NM_TYPE_GCONF_SETTINGS))
#define NM_GCONF_SETTINGS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), NM_TYPE_GCONF_SETTINGS, NMGConfSettingsClass))

typedef struct {
	NMSettingsService parent;
} NMGConfSettings;

typedef struct {
	NMSettingsServiceClass parent;

	/* Signals */
	void (*new_secrets_requested) (NMGConfSettings *self,
	                               NMGConfConnection *exported,
	                               const char *setting_name,
	                               const char **hints,
	                               gboolean ask_user,
	                               NMNewSecretsRequestedFunc callback,
	                               gpointer callback_data);
} NMGConfSettingsClass;

GType nm_gconf_settings_get_type (void);

NMGConfSettings *nm_gconf_settings_new (DBusGConnection *bus);

NMGConfConnection *nm_gconf_settings_add_connection (NMGConfSettings *self,
													 NMConnection *connection);

G_END_DECLS

#endif /* NM_GCONF_SETTINGS_H */
