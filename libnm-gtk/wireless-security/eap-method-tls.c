/* -*- Mode: C; tab-width: 5; indent-tabs-mode: t; c-basic-offset: 5 -*- */

/* NetworkManager Wireless Applet -- Display wireless access points and allow user control
 *
 * Dan Williams <dcbw@redhat.com>
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
 * (C) Copyright 2007 Red Hat, Inc.
 */

#include <glib/gi18n.h>
#include <ctype.h>
#include <string.h>
#include <nm-setting-connection.h>
#include <nm-setting-8021x.h>

#include "gconf-helpers.h"
#include "eap-method.h"
#include "wireless-security.h"
#include "utils.h"
#include "helpers.h"

static void
show_toggled_cb (GtkCheckButton *button, EAPMethod *method)
{
	GtkWidget *widget;
	gboolean visible;

	widget = GTK_WIDGET (gtk_builder_get_object (method->builder, "eap_tls_private_key_password_entry"));
	g_assert (widget);

	visible = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));
	gtk_entry_set_visibility (GTK_ENTRY (widget), visible);
}

static void
destroy (EAPMethod *parent)
{
	EAPMethodTLS *method = (EAPMethodTLS *) parent;

	g_slice_free (EAPMethodTLS, method);
}

static gboolean
validate (EAPMethod *parent)
{
	NMSetting8021xCKType ck_type = NM_SETTING_802_1X_CK_TYPE_UNKNOWN;
	GtkWidget *widget;
	const char *password, *identity;

	widget = GTK_WIDGET (gtk_builder_get_object (parent->builder, "eap_tls_identity_entry"));
	g_assert (widget);
	identity = gtk_entry_get_text (GTK_ENTRY (widget));
	if (!identity || !strlen (identity))
		return FALSE;

	if (!eap_method_validate_filepicker (parent->builder, "eap_tls_ca_cert_button", TYPE_CA_CERT, NULL, NULL))
		return FALSE;

	widget = GTK_WIDGET (gtk_builder_get_object (parent->builder, "eap_tls_private_key_password_entry"));
	g_assert (widget);
	password = gtk_entry_get_text (GTK_ENTRY (widget));
	if (!password || !strlen (password))
		return FALSE;

	if (!eap_method_validate_filepicker (parent->builder,
	                                     "eap_tls_private_key_button",
	                                     TYPE_PRIVATE_KEY,
	                                     password,
	                                     &ck_type))
		return FALSE;

	if (ck_type != NM_SETTING_802_1X_CK_TYPE_PKCS12) {
		if (!eap_method_validate_filepicker (parent->builder, "eap_tls_user_cert_button", TYPE_CLIENT_CERT, NULL, NULL))
			return FALSE;
	}

	return TRUE;
}

static void
add_to_size_group (EAPMethod *parent, GtkSizeGroup *group)
{
	GtkWidget *widget;

	widget = GTK_WIDGET (gtk_builder_get_object (parent->builder, "eap_tls_identity_label"));
	g_assert (widget);
	gtk_size_group_add_widget (group, widget);

	widget = GTK_WIDGET (gtk_builder_get_object (parent->builder, "eap_tls_user_cert_label"));
	g_assert (widget);
	gtk_size_group_add_widget (group, widget);

	widget = GTK_WIDGET (gtk_builder_get_object (parent->builder, "eap_tls_ca_cert_label"));
	g_assert (widget);
	gtk_size_group_add_widget (group, widget);

	widget = GTK_WIDGET (gtk_builder_get_object (parent->builder, "eap_tls_private_key_label"));
	g_assert (widget);
	gtk_size_group_add_widget (group, widget);

	widget = GTK_WIDGET (gtk_builder_get_object (parent->builder, "eap_tls_private_key_password_label"));
	g_assert (widget);
	gtk_size_group_add_widget (group, widget);
}

static void
fill_connection (EAPMethod *parent, NMConnection *connection)
{
	EAPMethodTLS *method = (EAPMethodTLS *) parent;
	NMSetting8021xCKType key_type = NM_SETTING_802_1X_CK_TYPE_UNKNOWN;
	NMSetting8021xCKFormat format = NM_SETTING_802_1X_CK_FORMAT_UNKNOWN;
	NMSetting8021x *s_8021x;
	NMSettingConnection *s_con;
	GtkWidget *widget;
	char *filename, *pk_filename, *cc_filename;
	char *password = NULL;
	GError *error = NULL;

	s_con = NM_SETTING_CONNECTION (nm_connection_get_setting (connection, NM_TYPE_SETTING_CONNECTION));
	g_assert (s_con);

	s_8021x = NM_SETTING_802_1X (nm_connection_get_setting (connection, NM_TYPE_SETTING_802_1X));
	g_assert (s_8021x);

	if (method->phase2)
		g_object_set (s_8021x, NM_SETTING_802_1X_PHASE2_AUTH, "tls", NULL);
	else
		nm_setting_802_1x_add_eap_method (s_8021x, "tls");

	widget = GTK_WIDGET (gtk_builder_get_object (parent->builder, "eap_tls_identity_entry"));
	g_assert (widget);
	g_object_set (s_8021x, NM_SETTING_802_1X_IDENTITY, gtk_entry_get_text (GTK_ENTRY (widget)), NULL);


	/* TLS private key */
	widget = GTK_WIDGET (gtk_builder_get_object (parent->builder, "eap_tls_private_key_password_entry"));
	g_assert (widget);
	password = g_strdup (gtk_entry_get_text (GTK_ENTRY (widget)));

	widget = GTK_WIDGET (gtk_builder_get_object (parent->builder, "eap_tls_private_key_button"));
	g_assert (widget);
	pk_filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (widget));
	g_assert (pk_filename);

	if (method->phase2) {
		if (!nm_setting_802_1x_set_phase2_private_key (s_8021x, pk_filename, password, NM_SETTING_802_1X_CK_SCHEME_PATH, &format, &error)) {
			g_warning ("Couldn't read phase2 private key '%s': %s", pk_filename, error ? error->message : "(unknown)");
			g_clear_error (&error);
		}
	} else {
		if (!nm_setting_802_1x_set_private_key (s_8021x, pk_filename, password, NM_SETTING_802_1X_CK_SCHEME_PATH, &format, &error)) {
			g_warning ("Couldn't read private key '%s': %s", pk_filename, error ? error->message : "(unknown)");
			g_clear_error (&error);
		}
	}

	/* TLS client certificate */
	if (key_type == NM_SETTING_802_1X_CK_TYPE_PKCS12) {
		/* if the key is pkcs#12, the cert is filled with the same data */
		cc_filename = g_strdup (pk_filename);
	} else {
		widget = GTK_WIDGET (gtk_builder_get_object (parent->builder, "eap_tls_user_cert_button"));
		g_assert (widget);
		cc_filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (widget));
	}

	g_assert (cc_filename);
	if (method->phase2) {
		if (!nm_setting_802_1x_set_phase2_client_cert (s_8021x, cc_filename, NM_SETTING_802_1X_CK_SCHEME_PATH, &format, &error)) {
			g_warning ("Couldn't read phase2 client certificate '%s': %s", cc_filename, error ? error->message : "(unknown)");
			g_clear_error (&error);
		}
	} else {
		if (!nm_setting_802_1x_set_client_cert (s_8021x, cc_filename, NM_SETTING_802_1X_CK_SCHEME_PATH, &format, &error)) {
			g_warning ("Couldn't read client certificate '%s': %s", cc_filename, error ? error->message : "(unknown)");
			g_clear_error (&error);
		}
	}
	g_free (cc_filename);
	g_free (pk_filename);

	/* TLS CA certificate */
	widget = GTK_WIDGET (gtk_builder_get_object (parent->builder, "eap_tls_ca_cert_button"));
	g_assert (widget);
	filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (widget));

	format = NM_SETTING_802_1X_CK_FORMAT_UNKNOWN;
	if (method->phase2) {
		if (!nm_setting_802_1x_set_phase2_ca_cert (s_8021x, filename, NM_SETTING_802_1X_CK_SCHEME_PATH, &format, &error)) {
			g_warning ("Couldn't read phase2 CA certificate '%s': %s", filename, error ? error->message : "(unknown)");
			g_clear_error (&error);
		}
	} else {
		if (!nm_setting_802_1x_set_ca_cert (s_8021x, filename, NM_SETTING_802_1X_CK_SCHEME_PATH, &format, &error)) {
			g_warning ("Couldn't read CA certificate '%s': %s", filename, error ? error->message : "(unknown)");
			g_clear_error (&error);
		}
	}

	nm_gconf_set_ignore_ca_cert (nm_setting_connection_get_uuid (s_con),
				     method->phase2,
				     eap_method_get_ignore_ca_cert (parent));
}

static void
private_key_picker_helper (EAPMethod *parent, const char *filename, gboolean changed)
{
	NMSetting8021x *setting;
	NMSetting8021xCKType cert_type = NM_SETTING_802_1X_CK_TYPE_UNKNOWN;
	const char *password;
	GtkWidget *widget;

	widget = GTK_WIDGET (gtk_builder_get_object (parent->builder, "eap_tls_private_key_password_entry"));
	g_assert (widget);
	password = gtk_entry_get_text (GTK_ENTRY (widget));

	setting = (NMSetting8021x *) nm_setting_802_1x_new ();
	nm_setting_802_1x_set_private_key_from_file (setting, filename, password, &cert_type, NULL);
	g_object_unref (setting);

	/* With PKCS#12, the client cert must be the same as the private key */
	widget = GTK_WIDGET (gtk_builder_get_object (parent->builder, "eap_tls_user_cert_button"));
	if (cert_type == NM_SETTING_802_1X_CK_TYPE_PKCS12) {
		gtk_file_chooser_unselect_all (GTK_FILE_CHOOSER (widget));
		gtk_widget_set_sensitive (widget, FALSE);
	} else if (changed)
		gtk_widget_set_sensitive (widget, TRUE);
}

static void
private_key_picker_file_set_cb (GtkWidget *chooser, gpointer user_data)
{
	EAPMethod *parent = (EAPMethod *) user_data;
	char *filename;

	filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));
	if (filename)
		private_key_picker_helper (parent, filename, TRUE);
	g_free (filename);
}

static void reset_filter (GtkWidget *widget, GParamSpec *spec, gpointer user_data)
{
	if (!gtk_file_chooser_get_filter (GTK_FILE_CHOOSER (widget))) {
		g_signal_handlers_block_by_func (widget, reset_filter, user_data);
		gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (widget), GTK_FILE_FILTER (user_data));
		g_signal_handlers_unblock_by_func (widget, reset_filter, user_data);
	}
}

typedef const char * (*PathFunc) (NMSetting8021x *setting);
typedef NMSetting8021xCKScheme (*SchemeFunc)  (NMSetting8021x *setting);

static void
setup_filepicker (GtkBuilder *builder,
                  const char *name,
                  const char *title,
                  WirelessSecurity *parent,
                  EAPMethodTLS *method,
                  NMSetting8021x *s_8021x,
                  SchemeFunc scheme_func,
                  PathFunc path_func,
                  gboolean privkey,
                  gboolean client_cert)
{
	GtkWidget *widget;
	GtkFileFilter *filter;
	const char *filename = NULL;

	widget = GTK_WIDGET (gtk_builder_get_object (builder, name));
	g_assert (widget);
	gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (widget), TRUE);
	gtk_file_chooser_button_set_title (GTK_FILE_CHOOSER_BUTTON (widget), title);

	if (s_8021x && path_func && scheme_func) {
		if (scheme_func (s_8021x) == NM_SETTING_802_1X_CK_SCHEME_PATH) {
			filename = path_func (s_8021x);
			if (filename)
				gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (widget), filename);
		}
	}

	/* Connect a special handler for private keys to intercept PKCS#12 key types
	 * and desensitize the user cert button.
	 */
	if (privkey) {
		g_signal_connect (G_OBJECT (widget), "selection-changed",
		                  (GCallback) private_key_picker_file_set_cb,
		                  method);
		if (filename)
			private_key_picker_helper ((EAPMethod *) method, filename, FALSE);
	}

	g_signal_connect (G_OBJECT (widget), "selection-changed",
	                  (GCallback) wireless_security_changed_cb,
	                  parent);

	filter = eap_method_default_file_chooser_filter_new (privkey);
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (widget), filter);

	/* For some reason, GTK+ calls set_current_filter (..., NULL) from 
	 * gtkfilechooserdefault.c::show_and_select_files_finished_loading() on our
	 * dialog; so force-reset the filter to what we want it to be whenever
	 * it gets cleared.
	 */
	if (client_cert)
		g_signal_connect (G_OBJECT (widget), "notify::filter", (GCallback) reset_filter, filter);
}

EAPMethodTLS *
eap_method_tls_new (WirelessSecurity *parent,
                    NMConnection *connection,
                    gboolean phase2)
{
	EAPMethodTLS *method;
	GtkBuilder *builder;
	GtkWidget *widget;
	NMSetting8021x *s_8021x = NULL;

	method = g_slice_new0 (EAPMethodTLS);
	if (!eap_method_init (EAP_METHOD (method), validate, add_to_size_group,
					  fill_connection, destroy, "eap-tls.ui", "eap_tls_notebook")) {
		g_slice_free (EAPMethodTLS, method);
		return NULL;
	}

	builder = EAP_METHOD (method)->builder;

	eap_method_nag_init (EAP_METHOD (method), "ca-nag-dialog.ui", "eap_tls_ca_cert_button", connection, phase2);

	method->phase2 = phase2;

	if (connection)
		s_8021x = NM_SETTING_802_1X (nm_connection_get_setting (connection, NM_TYPE_SETTING_802_1X));

	widget = GTK_WIDGET (gtk_builder_get_object (builder, "eap_tls_identity_entry"));
	g_assert (widget);
	g_signal_connect (G_OBJECT (widget), "changed",
	                  (GCallback) wireless_security_changed_cb,
	                  parent);
	if (s_8021x && nm_setting_802_1x_get_identity (s_8021x))
		gtk_entry_set_text (GTK_ENTRY (widget), nm_setting_802_1x_get_identity (s_8021x));

	widget = GTK_WIDGET (gtk_builder_get_object (builder, "eap_tls_private_key_password_entry"));
	g_assert (widget);

	/* Fill secrets, if any */
	if (connection) {
		helper_fill_secret_entry (connection,
		                          GTK_ENTRY (widget),
		                          NM_TYPE_SETTING_802_1X,
		                          phase2 ? (HelperSecretFunc) nm_setting_802_1x_get_phase2_private_key_password :
		                                   (HelperSecretFunc) nm_setting_802_1x_get_private_key_password,
		                          NM_SETTING_802_1X_SETTING_NAME,
		                          phase2 ? NM_SETTING_802_1X_PHASE2_PRIVATE_KEY_PASSWORD :
		                                   NM_SETTING_802_1X_PRIVATE_KEY_PASSWORD);
	}

	g_signal_connect (G_OBJECT (widget), "changed",
	                  (GCallback) wireless_security_changed_cb,
	                  parent);
	setup_filepicker (builder, "eap_tls_user_cert_button",
	                  _("Choose your personal certificate..."),
	                  parent, method, s_8021x,
	                  phase2 ? nm_setting_802_1x_get_phase2_client_cert_scheme : nm_setting_802_1x_get_client_cert_scheme,
	                  phase2 ? nm_setting_802_1x_get_phase2_client_cert_path : nm_setting_802_1x_get_client_cert_path,
	                  FALSE, TRUE);
	setup_filepicker (builder, "eap_tls_ca_cert_button",
	                  _("Choose a Certificate Authority certificate..."),
	                  parent, method, s_8021x,
	                  phase2 ? nm_setting_802_1x_get_phase2_ca_cert_scheme : nm_setting_802_1x_get_ca_cert_scheme,
	                  phase2 ? nm_setting_802_1x_get_phase2_ca_cert_path : nm_setting_802_1x_get_ca_cert_path,
	                  FALSE, FALSE);
	setup_filepicker (builder,
	                  "eap_tls_private_key_button",
	                  _("Choose your private key..."),
	                  parent, method, s_8021x,
	                  phase2 ? nm_setting_802_1x_get_phase2_private_key_scheme : nm_setting_802_1x_get_private_key_scheme,
	                  phase2 ? nm_setting_802_1x_get_phase2_private_key_path : nm_setting_802_1x_get_private_key_path,
	                  TRUE, FALSE);

	widget = GTK_WIDGET (gtk_builder_get_object (builder, "eap_tls_show_checkbutton"));
	g_assert (widget);
	g_signal_connect (G_OBJECT (widget), "toggled",
	                  (GCallback) show_toggled_cb,
	                  method);

	return method;
}

