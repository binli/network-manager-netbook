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

#include <glib/gi18n.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <errno.h>
#include <nm-utils.h>
#include "nmn-connection-details.h"

G_DEFINE_TYPE (NmnConnectionDetails, nmn_connection_details, GTK_TYPE_TABLE)

enum {
    CHANGED,

    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), NMN_TYPE_CONNECTION_DETAILS, NmnConnectionDetailsPrivate))

typedef struct {
    GtkWidget *connect_method;

    GtkWidget *address;
    GtkWidget *netmask;
    GtkWidget *gateway;
    GtkWidget *dns;
    GtkWidget *hw_address;

    gboolean editable;
    gboolean last_check_result;

    gboolean disposed;
} NmnConnectionDetailsPrivate;

enum {
    CONNECT_METHOD_DHCP = 0,
    CONNECT_METHOD_MANUAL = 1,
    CONNECT_METHOD_LINK_LOCAL = 2
};

NmnConnectionDetails *
nmn_connection_details_new (void)
{
    return NMN_CONNECTION_DETAILS (g_object_new (NMN_TYPE_CONNECTION_DETAILS, NULL));
}

static void
editable_changed (NmnConnectionDetails *self,
                  gboolean editable)
{
    NmnConnectionDetailsPrivate *priv = GET_PRIVATE (self);
    gboolean entries_enabled;

    priv->editable = editable;

    gtk_widget_set_sensitive (priv->connect_method, editable);

    if (editable)
        entries_enabled = gtk_combo_box_get_active (GTK_COMBO_BOX (priv->connect_method)) == CONNECT_METHOD_MANUAL;
    else
        entries_enabled = FALSE;

    gtk_widget_set_sensitive (priv->address, entries_enabled);
    gtk_widget_set_sensitive (priv->netmask, entries_enabled);
    gtk_widget_set_sensitive (priv->gateway, entries_enabled);

    gtk_text_view_set_editable (GTK_TEXT_VIEW (priv->dns), entries_enabled);
}

static void
connect_method_changed (GtkComboBox *combo,
                        gpointer user_data)
{
    NmnConnectionDetails *self = NMN_CONNECTION_DETAILS (user_data);

    editable_changed (self, GET_PRIVATE (self)->editable);
}

static char *
ip4_address_as_string (guint32 ip)
{
    char *ip_string;
    struct in_addr tmp_addr;

    tmp_addr.s_addr = ip;
    ip_string = g_malloc0 (INET_ADDRSTRLEN + 1);
    if (!inet_ntop (AF_INET, &tmp_addr, ip_string, INET_ADDRSTRLEN))
        strcpy (ip_string, _("(none)"));

    return ip_string;
}

void
nmn_connection_details_set_data (NmnConnectionDetails *self,
                                 NMSettingIP4Config *setting,
                                 NMIP4Config *config,
                                 const char *hw_address)
{
    NmnConnectionDetailsPrivate *priv;
    GtkTextBuffer *buffer;
    NMIP4Address *address = NULL;
    GString *string;
    char *str;
    int i;

    g_return_if_fail (NMN_IS_CONNECTION_DETAILS (self));

    priv = GET_PRIVATE (self);

    if (setting) {
        const char *method;

        method = nm_setting_ip4_config_get_method (setting);
        if (!strcmp (method, NM_SETTING_IP4_CONFIG_METHOD_AUTO))
            gtk_combo_box_set_active (GTK_COMBO_BOX (priv->connect_method), CONNECT_METHOD_DHCP);
        else if (!strcmp (method, NM_SETTING_IP4_CONFIG_METHOD_MANUAL))
            gtk_combo_box_set_active (GTK_COMBO_BOX (priv->connect_method), CONNECT_METHOD_MANUAL);
        else if (!strcmp (method, NM_SETTING_IP4_CONFIG_METHOD_LINK_LOCAL))
            gtk_combo_box_set_active (GTK_COMBO_BOX (priv->connect_method), CONNECT_METHOD_LINK_LOCAL);
        else
            g_assert_not_reached ();
    } else
        gtk_combo_box_set_active (GTK_COMBO_BOX (priv->connect_method), CONNECT_METHOD_DHCP);

    /* We're editable when we're not connected, ie, when
       the active IP4 config is not present */
    editable_changed (self, config == NULL);

    /* We prefer config if it's present */
    if (config) {
        const GSList *list;

        list = nm_ip4_config_get_addresses (config);
        address = list ? list->data : NULL;
    }

    if (!address && setting)
        address = nm_setting_ip4_config_get_address (setting, 0);

    if (address) {
        str = ip4_address_as_string (nm_ip4_address_get_address (address));
        gtk_entry_set_text (GTK_ENTRY (priv->address), str);
        g_free (str);

        str = ip4_address_as_string (nm_utils_ip4_prefix_to_netmask (nm_ip4_address_get_prefix (address)));
        gtk_entry_set_text (GTK_ENTRY (priv->netmask), str);
        g_free (str);

        str = ip4_address_as_string (nm_ip4_address_get_gateway (address));
        gtk_entry_set_text (GTK_ENTRY (priv->gateway), str);
        g_free (str);
    } else {
        gtk_entry_set_text (GTK_ENTRY (priv->address), "");
        gtk_entry_set_text (GTK_ENTRY (priv->gateway), "");
        gtk_entry_set_text (GTK_ENTRY (priv->gateway), "");
    }

    if (config) {
        const GArray *array;

        string = g_string_sized_new (256);
        array = nm_ip4_config_get_nameservers (config);
        for (i = 0; array && i < array->len; i++) {
            if (i > 0)
                g_string_append_c (string, '\n');

            str = ip4_address_as_string (g_array_index (array, guint32, i));
            g_string_append (string, str);
            g_free (str);
        }

        str = g_string_free (string, FALSE);
    } else if (setting) {
        string = g_string_sized_new (256);
        for (i = 0; i < nm_setting_ip4_config_get_num_dns (setting); i++) {
            if (i > 0)
                g_string_append_c (string, '\n');

            str = ip4_address_as_string (nm_setting_ip4_config_get_dns (setting, i));
            g_string_append (string, str);
            g_free (str);
        }

        str = g_string_free (string, FALSE);
    } else
        str = g_strdup ("");

    buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->dns));
    gtk_text_buffer_set_text (buffer, str, -1);
    g_free (str);

    gtk_label_set_text (GTK_LABEL (priv->hw_address), hw_address);
}


static gboolean
parse_netmask (const char *str, guint32 *prefix)
{
	struct in_addr tmp_addr;
	glong tmp_prefix;

	errno = 0;

	/* Is it a prefix? */
	if (!strchr (str, '.')) {
		tmp_prefix = strtol (str, NULL, 10);
		if (!errno && tmp_prefix >= 0 && tmp_prefix <= 32) {
			*prefix = tmp_prefix;
			return TRUE;
		}
	}

	/* Is it a netmask? */
	if (inet_pton (AF_INET, str, &tmp_addr) > 0) {
		*prefix = nm_utils_ip4_netmask_to_prefix (tmp_addr.s_addr);
		return TRUE;
	}

	return FALSE;
}

static gboolean
parse_dns_list (GtkTextView *view, GSList **dns_list)
{
    GtkTextBuffer *buffer;
    char **items;
    char **iter;
    char *str = NULL;
    gboolean success = TRUE;

    buffer = gtk_text_view_get_buffer (view);
    g_object_get (buffer, "text", &str, NULL);

    items = g_strsplit (str, ", ;:\n", 0);
    g_free (str);

    for (iter = items; *iter; iter++) {
        char *stripped = g_strstrip (*iter);
        struct in_addr tmp_addr;

        if (strlen (stripped) < 1)
            continue;

        if (inet_pton (AF_INET, stripped, &tmp_addr))
            *dns_list = g_slist_prepend (*dns_list, GUINT_TO_POINTER (tmp_addr.s_addr));
        else {
            success = FALSE;
            break;
        }
    }

    g_strfreev (items);

    if (success)
        *dns_list = g_slist_reverse (*dns_list);
    else {
        g_slist_free (*dns_list);
        *dns_list = NULL;
    }

    return success;
}

static gboolean
ui_to_setting (NmnConnectionDetails *self,
               const char **method,
               NMIP4Address **address,
               GSList **dns_list)
{
    NmnConnectionDetailsPrivate *priv = GET_PRIVATE (self);
    int active;
    gboolean success = FALSE;

    active = gtk_combo_box_get_active (GTK_COMBO_BOX (priv->connect_method));

    if (active == CONNECT_METHOD_DHCP) {
        *method = NM_SETTING_IP4_CONFIG_METHOD_AUTO;
        *address = NULL;
        success = TRUE;
    } else if (active == CONNECT_METHOD_LINK_LOCAL) {
        *method = NM_SETTING_IP4_CONFIG_METHOD_LINK_LOCAL;
        *address = NULL;
        success = TRUE;
    } else {
        struct in_addr tmp_addr  = { 0 };
        guint32 prefix;

        *address = nm_ip4_address_new ();
        
        if (inet_aton (gtk_entry_get_text (GTK_ENTRY (priv->address)), &tmp_addr) == 0)
            goto out;
        nm_ip4_address_set_address (*address, tmp_addr.s_addr);

        if (!parse_netmask (gtk_entry_get_text (GTK_ENTRY (priv->netmask)), &prefix))
            goto out;
        nm_ip4_address_set_prefix (*address, prefix);

        if (inet_aton (gtk_entry_get_text (GTK_ENTRY (priv->gateway)), &tmp_addr) == 0)
            goto out;
        nm_ip4_address_set_gateway (*address, tmp_addr.s_addr);

        if (!parse_dns_list (GTK_TEXT_VIEW (priv->dns), dns_list))
            goto out;

        *method = NM_SETTING_IP4_CONFIG_METHOD_MANUAL;

        success = TRUE;
    }

 out:
    if (!success && *address) {
        nm_ip4_address_unref (*address);
        *address = NULL;
    }

    return success;
}

NMSettingIP4Config *
nmn_connection_details_get_data (NmnConnectionDetails *self)
{
    NMSettingIP4Config *setting;
    const char *method;
    NMIP4Address *address;
    GSList *dns_list = NULL;
    GSList *iter;

    g_return_val_if_fail (NMN_IS_CONNECTION_DETAILS (self), NULL);

    if (!ui_to_setting (self, &method, &address, &dns_list))
        return NULL;

    setting = NM_SETTING_IP4_CONFIG (nm_setting_ip4_config_new ());
    g_object_set (setting, NM_SETTING_IP4_CONFIG_METHOD, method, NULL);
    if (address) {
        nm_setting_ip4_config_add_address (setting, address);
        nm_ip4_address_unref (address);
    }

    for (iter = dns_list; iter; iter = iter->next)
        nm_setting_ip4_config_add_dns (setting, GPOINTER_TO_UINT (iter->data));

    g_slist_free (dns_list);

    return setting;
}

gboolean
nmn_connection_details_verify (NmnConnectionDetails *self)
{
    g_return_val_if_fail (NMN_IS_CONNECTION_DETAILS (self), FALSE);

    return GET_PRIVATE (self)->last_check_result;
}

static void
ip_filter_cb (GtkEditable *editable,
              gchar *text,
              gint length,
              gint *position,
              gpointer user_data)
{
    gchar *result;
    int i;
    int count = 0;

    result = g_new (gchar, length);
    for (i = 0; i < length; i++) {
        int c = text[i];

        if ((c >= '0' && c <= '9') || c == '.')
            result[count++] = c;
    }

    if (count > 0) {
        g_signal_handlers_block_by_func (editable, G_CALLBACK (ip_filter_cb), user_data);
        gtk_editable_insert_text (editable, result, count, position);
        g_signal_handlers_unblock_by_func (editable, G_CALLBACK (ip_filter_cb), user_data);
    }

    g_signal_stop_emission_by_name (editable, "insert-text");
    g_free (result);
}

static void
dns_filter_cb (GtkTextBuffer *textbuffer,
               GtkTextIter *location,
               gchar *text,
               gint length,
               gpointer user_data)
{
    gchar *result;
    int i;
    int count = 0;

    result = g_new (gchar, length);
    for (i = 0; i < length; i++) {
        char c = text[i];

        if ((c >= '0' && c <= '9') || c == '.' || c == '\n' || c == ' ' || c == ';' || c == ':')
            result[count++] = c;
    }

    if (count > 0) {
        g_signal_handlers_block_by_func (textbuffer, G_CALLBACK (dns_filter_cb), user_data);
        gtk_text_buffer_insert_at_cursor (textbuffer, result, count);
        g_signal_handlers_unblock_by_func (textbuffer, G_CALLBACK (dns_filter_cb), user_data);
    }

    g_signal_stop_emission_by_name (textbuffer, "insert-text");
    g_free (result);
}

static void
stuff_changed (NmnConnectionDetails *self)
{
    NmnConnectionDetailsPrivate *priv = GET_PRIVATE (self);
    const char *method;
    NMIP4Address *address;
    GSList *dns_list = NULL;
    gboolean verified;

    verified = ui_to_setting (self, &method, &address, &dns_list);
    if (address)
        nm_ip4_address_unref (address);
    g_slist_free (dns_list);

    if (verified != priv->last_check_result) {
        priv->last_check_result = verified;
        g_signal_emit (self, signals[CHANGED], 0, verified);
    }
}

static inline GtkWidget *
aligned_label_new (const char *text)
{
    GtkWidget *w;

    w = gtk_label_new (text);
    g_object_set (w, "xalign", 0.0, NULL);

    return w;
}

static void
nmn_connection_details_init (NmnConnectionDetails *details)
{
    NmnConnectionDetailsPrivate *priv = GET_PRIVATE (details);
    GtkTable *table;
    GtkWidget *w;
    GtkTextBuffer *buffer;

    g_object_set (details,
                  "n-rows", 6,
                  "n-columns", 2,
                  "homogeneous", FALSE,
                  "row-spacing", 6,
                  "column-spacing", 6,
                  NULL);

    table = GTK_TABLE (details);

    /* Connect by: */
    w = aligned_label_new (_("Connect by:"));
    gtk_table_attach_defaults (table, w, 0, 1, 0, 1);

    w = gtk_combo_box_new_text ();
    gtk_combo_box_append_text (GTK_COMBO_BOX (w), _("DHCP"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (w), _("Manual"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (w), _("Link Local"));
    gtk_combo_box_set_active (GTK_COMBO_BOX (w), 0);
    g_signal_connect (w, "changed", G_CALLBACK (connect_method_changed), details);
    gtk_table_attach_defaults (table, w, 1, 2, 0, 1);
    priv->connect_method = w;

    /* Address */
    w = aligned_label_new (_("IP Address:"));
    gtk_table_attach_defaults (table, w, 0, 1, 1, 2);

    priv->address = gtk_entry_new ();
    gtk_table_attach_defaults (table, priv->address, 1, 2, 1, 2);
    g_signal_connect (priv->address, "insert-text", G_CALLBACK (ip_filter_cb), NULL);
    g_signal_connect_swapped (priv->address, "changed", G_CALLBACK (stuff_changed), details);

    /* Netmask */
    w = aligned_label_new (_("Subnet mask:"));
    gtk_table_attach_defaults (table, w, 0, 1, 2, 3);

    priv->netmask = gtk_entry_new ();
    gtk_table_attach_defaults (table, priv->netmask, 1, 2, 2, 3);
    g_signal_connect (priv->netmask, "insert-text", G_CALLBACK (ip_filter_cb), NULL);
    g_signal_connect_swapped (priv->netmask, "changed", G_CALLBACK (stuff_changed), details);

    /* Gateway */
    w = aligned_label_new (_("Router:"));
    gtk_table_attach_defaults (table, w, 0, 1, 3, 4);

    priv->gateway = gtk_entry_new ();
    gtk_table_attach_defaults (table, priv->gateway, 1, 2, 3, 4);
    g_signal_connect (priv->gateway, "insert-text", G_CALLBACK (ip_filter_cb), NULL);
    g_signal_connect_swapped (priv->gateway, "changed", G_CALLBACK (stuff_changed), details);

    /* DNS */
    w = aligned_label_new (_("DNS:"));
    gtk_table_attach_defaults (table, w, 0, 1, 4, 5);

    priv->dns = gtk_text_view_new ();
    gtk_table_attach_defaults (table, priv->dns, 1, 2, 4, 5);
    buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->dns));
    g_signal_connect (buffer, "insert-text", G_CALLBACK (dns_filter_cb), NULL);
    g_signal_connect_swapped (buffer, "changed", G_CALLBACK (stuff_changed), details);

    /* Hardware address */
    w = aligned_label_new (_("Your MAC address:"));
    gtk_table_attach_defaults (table, w, 0, 1, 5, 6);

    priv->hw_address = aligned_label_new (NULL);
    gtk_table_attach_defaults (table, priv->hw_address, 1, 2, 5, 6);

    editable_changed (details, TRUE);
    stuff_changed (details);
    gtk_widget_show_all (GTK_WIDGET (details));
}

static void
nmn_connection_details_class_init (NmnConnectionDetailsClass *class)
{
    GObjectClass *object_class = G_OBJECT_CLASS (class);

    g_type_class_add_private (object_class, sizeof (NmnConnectionDetailsPrivate));

    /* Signals */
    signals[CHANGED] =
        g_signal_new ("changed",
                      G_OBJECT_CLASS_TYPE (class),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (NmnConnectionDetailsClass, changed),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__BOOLEAN,
                      G_TYPE_NONE, 1,
                      G_TYPE_BOOLEAN);
}
