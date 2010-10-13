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
 * (C) Copyright 2008 Red Hat, Inc.
 */

#ifndef NM_GCONF_CONNECTION_H
#define NM_GCONF_CONNECTION_H

#include <gconf/gconf-client.h>
#include <dbus/dbus-glib.h>
#include <nm-connection.h>
#include <nm-exported-connection.h>
#include <nm-settings-connection-interface.h>

G_BEGIN_DECLS

#define NM_TYPE_GCONF_CONNECTION            (nm_gconf_connection_get_type ())
#define NM_GCONF_CONNECTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), NM_TYPE_GCONF_CONNECTION, NMGConfConnection))
#define NM_GCONF_CONNECTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), NM_TYPE_GCONF_CONNECTION, NMGConfConnectionClass))
#define NM_IS_GCONF_CONNECTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NM_TYPE_GCONF_CONNECTION))
#define NM_IS_GCONF_CONNECTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), NM_TYPE_GCONF_CONNECTION))
#define NM_GCONF_CONNECTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), NM_TYPE_GCONF_CONNECTION, NMGConfConnectionClass))

#define NM_GCONF_CONNECTION_CLIENT "client"
#define NM_GCONF_CONNECTION_DIR    "dir"

typedef struct {
	NMExportedConnection parent;
} NMGConfConnection;

typedef void (*NMNewSecretsRequestedFunc) (NMSettingsConnectionInterface *connection,
										   GHashTable *settings,
										   GError *error,
										   gpointer user_data);

typedef struct {
	NMExportedConnectionClass parent;

	/* Signals */
	void (*new_secrets_requested)  (NMGConfConnection *self,
	                                const char *setting_name,
	                                const char **hints,
	                                gboolean ask_user,
	                                NMNewSecretsRequestedFunc callback,
									gpointer callback_data);
} NMGConfConnectionClass;

GType nm_gconf_connection_get_type (void);

NMGConfConnection *nm_gconf_connection_new  (GConfClient *client,
											 const char *conf_dir);

NMGConfConnection *nm_gconf_connection_new_from_connection (GConfClient *client,
															const char *conf_dir,
															NMConnection *connection);

gboolean nm_gconf_connection_gconf_changed (NMGConfConnection *self);

const char *nm_gconf_connection_get_gconf_path (NMGConfConnection *self);

G_END_DECLS

#endif /* NM_GCONF_CONNECTION_H */
