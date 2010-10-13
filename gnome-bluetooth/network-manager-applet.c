/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2009  Bastien Nocera <hadess@hadess.net>
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <net/ethernet.h>
#include <netinet/ether.h>
#include <glib/gi18n-lib.h>

#include <gtk/gtk.h>
#include <bluetooth-plugin.h>
#include <nm-setting-connection.h>
#include <nm-setting-bluetooth.h>
#include <nm-setting-ip4-config.h>
#include <nm-utils.h>
#include <nm-settings-interface.h>
#include <nm-remote-settings.h>
#include <nm-remote-settings-system.h>
#include <gconf-helpers.h>

static gboolean
has_config_widget (const char *bdaddr, const char **uuids)
{
	guint i;

	for (i = 0; uuids && uuids[i] != NULL; i++) {
		if (g_str_equal (uuids[i], "NAP"))
			return TRUE;
	}
	return FALSE;
}

static GByteArray *
get_array_from_bdaddr (const char *str)
{
	struct ether_addr *addr;
	GByteArray *array;

	addr = ether_aton (str);
	if (addr) {
		array = g_byte_array_sized_new (ETH_ALEN);
		g_byte_array_append (array, (const guint8 *) addr->ether_addr_octet, ETH_ALEN);
		return array;
	}

	return NULL;
}

typedef struct {
	GMainLoop *loop;
	guint counter;
} GetSettingsSyncInfo;

static void
connections_read (NMSettingsInterface *settings,
				  gpointer user_data)
{
	GetSettingsSyncInfo *info = user_data;

	info->counter--;
	if (info->counter < 1)
		g_main_loop_quit (info->loop);
}

static gboolean
get_settings_timed_out (gpointer user_data)
{
	GetSettingsSyncInfo *info = user_data;

	g_warning ("Getting settings timed out");
	g_main_loop_quit (info->loop);

	return FALSE;
}

static NMSettingsInterface *user_settings = NULL;
static NMSettingsInterface *system_settings = NULL;

static gboolean
init_settings ()
{
	DBusGConnection *bus;
	GetSettingsSyncInfo info;
	GError *err = NULL;
	gboolean running;

	bus = dbus_g_bus_get (DBUS_BUS_SYSTEM, &err);
	if (!bus) {
		g_warning ("Couldn't connect to system bus: %s", err->message);
		g_error_free (err);
		return FALSE;
	}

	info.loop = g_main_loop_new (NULL, FALSE);
	info.counter = 0;

	user_settings = NM_SETTINGS_INTERFACE (nm_remote_settings_new (bus, NM_CONNECTION_SCOPE_USER));
	g_object_get (user_settings, NM_REMOTE_SETTINGS_SERVICE_RUNNING, &running, NULL);
	if (running) {
		g_signal_connect (user_settings, "connections-read", G_CALLBACK (connections_read), &info);
		info.counter++;
	}

	system_settings = NM_SETTINGS_INTERFACE (nm_remote_settings_system_new (bus));
	g_object_get (system_settings, NM_REMOTE_SETTINGS_SERVICE_RUNNING, &running, NULL);
	if (running) {
		g_signal_connect (system_settings, "connections-read", G_CALLBACK (connections_read), &info);
		info.counter++;
	}

	dbus_g_connection_unref (bus);

	if (info.counter > 0) {
		guint timeout_id;

		/* Add a timer so that we don't wait here forever in case we never
		   get signaled that services are read */
		timeout_id = g_timeout_add_seconds (5, get_settings_timed_out, &info);
		g_main_loop_run (info.loop);
		g_source_remove (timeout_id);
	}

	g_main_loop_unref (info.loop);

	return TRUE;
}

static GSList *
get_settings (void)
{
	GSList *list;

	if (!user_settings && !system_settings && !init_settings ())
		return NULL;

	list = nm_settings_interface_list_connections (user_settings);
	list = g_slist_concat (list, nm_settings_interface_list_connections (system_settings));

	return list;
}

static NMSettingsConnectionInterface *
get_connection_for_bdaddr (const char *bdaddr)
{
	NMSettingsConnectionInterface *found = NULL;
	GSList *list, *l;
	GByteArray *array;

	array = get_array_from_bdaddr (bdaddr);
	if (array == NULL)
		return NULL;

	list = get_settings ();
	for (l = list; l != NULL; l = l->next) {
		NMSettingsConnectionInterface *candidate = l->data;
		NMSetting *setting;
		const char *type;
		const GByteArray *addr;

		setting = nm_connection_get_setting_by_name (NM_CONNECTION (candidate), NM_SETTING_BLUETOOTH_SETTING_NAME);
		if (setting == NULL)
			continue;
		type = nm_setting_bluetooth_get_connection_type (NM_SETTING_BLUETOOTH (setting));
		if (g_strcmp0 (type, NM_SETTING_BLUETOOTH_TYPE_PANU) != 0)
			continue;
		addr = nm_setting_bluetooth_get_bdaddr (NM_SETTING_BLUETOOTH (setting));
		if (addr == NULL || memcmp (addr->data, array->data, addr->len) != 0)
			continue;
		found = candidate;
		break;
	}

	g_byte_array_free (array, TRUE);
	g_slist_free (list);

	return found;
}

static void
create_connection (const char *bdaddr)
{
	NMConnection *connection;
	NMSetting *setting;
	GByteArray *mac;
	char *id, *uuid;

	mac = get_array_from_bdaddr (bdaddr);
	if (mac == NULL)
		return;

	/* The connection */
	connection = nm_connection_new ();

	/* The connection settings */
	setting = nm_setting_connection_new ();
	id = g_strdup_printf ("%s %s", bdaddr, "PANU");
	uuid = nm_utils_uuid_generate ();
	g_object_set (G_OBJECT (setting),
	              NM_SETTING_CONNECTION_ID, id,
	              NM_SETTING_CONNECTION_UUID, uuid,
	              NM_SETTING_CONNECTION_TYPE, NM_SETTING_BLUETOOTH_SETTING_NAME,
	              NM_SETTING_CONNECTION_AUTOCONNECT, FALSE,
	              NULL);
	g_free (id);
	g_free (uuid);
	nm_connection_add_setting (connection, setting);

	/* The Bluetooth settings */
	setting = nm_setting_bluetooth_new ();
	g_object_set (G_OBJECT (setting),
	              NM_SETTING_BLUETOOTH_BDADDR, mac,
	              NM_SETTING_BLUETOOTH_TYPE, NM_SETTING_BLUETOOTH_TYPE_PANU,
	              NULL);
	g_byte_array_free (mac, TRUE);
	nm_connection_add_setting (connection, setting);

	/* The IPv4 settings */
	setting = nm_setting_ip4_config_new ();
	g_object_set (G_OBJECT (setting),
	              NM_SETTING_IP4_CONFIG_METHOD, NM_SETTING_IP4_CONFIG_METHOD_AUTO,
	              NULL);
	nm_connection_add_setting (connection, setting);

	nm_gconf_write_connection (connection, NULL, NULL);
	g_object_unref (connection);
}

static void
delete_cb (NMSettingsConnectionInterface *connection,
           GError *error,
           gpointer user_data)
{
	if (error)
		g_warning ("Error deleting connection: (%d) %s", error->code, error->message);
}

static void
delete_connection (const char *bdaddr)
{
	NMSettingsConnectionInterface *connection;

	// FIXME: don't just delete any random PAN conenction for this
	// bdaddr, actually delete the one this plugin created
	connection = get_connection_for_bdaddr (bdaddr);
	if (connection)
		nm_settings_connection_interface_delete (connection, delete_cb, NULL);
}

static void
button_toggled (GtkToggleButton *button, gpointer user_data)
{
	const char *bdaddr;

	bdaddr = g_object_get_data (G_OBJECT (button), "bdaddr");
	g_assert (bdaddr);

	if (gtk_toggle_button_get_active (button) == FALSE)
		delete_connection (bdaddr);
	else
		create_connection (bdaddr);
}

static GtkWidget *
get_config_widgets (const char *bdaddr, const char **uuids)
{
	GtkWidget *button;
	NMSettingsConnectionInterface *connection;

	button = gtk_check_button_new_with_label (_("Access the Internet using your mobile phone"));
	g_object_set_data_full (G_OBJECT (button),
	                        "bdaddr", g_strdup (bdaddr),
	                        (GDestroyNotify) g_free);

	connection = get_connection_for_bdaddr (bdaddr);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), connection != NULL);
	g_signal_connect (button, "toggled", G_CALLBACK (button_toggled), NULL);

	return button;
}

static void
device_removed (const char *bdaddr)
{
	g_message ("Device '%s' got removed", bdaddr);
	delete_connection (bdaddr);
}

static GbtPluginInfo plugin_info = {
	"network-manager-applet",
	has_config_widget,
	get_config_widgets,
	device_removed
};

GBT_INIT_PLUGIN(plugin_info)

