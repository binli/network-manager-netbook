/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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
 * (C) Copyright 2007 - 2008 Red Hat, Inc.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <netinet/ether.h>

#include <nm-client.h>
#include <nm-utils.h>
#include <nm-device-wifi.h>
#include <nm-setting-connection.h>
#include <nm-setting-wireless.h>
#include <nm-setting-ip4-config.h>

#include "wireless-dialog.h"
#include "wireless-security.h"
#include "utils.h"
#include "gconf-helpers.h"

G_DEFINE_TYPE (NMAWirelessDialog, nma_wireless_dialog, GTK_TYPE_DIALOG)

enum {
    DONE,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

#define NMA_WIRELESS_DIALOG_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), \
                                                                         NMA_TYPE_WIRELESS_DIALOG, \
                                                                         NMAWirelessDialogPrivate))

typedef struct {
    NMClient *client;
    GtkBuilder *builder;

    NMConnection *connection;
    NMDevice *device;
    NMAccessPoint *ap;
    gboolean adhoc_create;

    GtkTreeModel *device_model;
    GtkTreeModel *connection_model;
    GtkSizeGroup *group;
    GtkWidget *sec_combo;

    gboolean nag_ignored;

    gboolean disposed;
} NMAWirelessDialogPrivate;

#define D_NAME_COLUMN       0
#define D_DEV_COLUMN        1

#define S_NAME_COLUMN       0
#define S_SEC_COLUMN        1

#define C_NAME_COLUMN       0
#define C_CON_COLUMN        1
#define C_SEP_COLUMN        2
#define C_NEW_COLUMN        3

static void security_combo_changed (GtkWidget *combo, gpointer user_data);
static gboolean security_combo_init (NMAWirelessDialog *self);

static void
nma_wireless_dialog_set_nag_ignored (NMAWirelessDialog *self, gboolean ignored)
{
    g_return_if_fail (self != NULL);

    NMA_WIRELESS_DIALOG_GET_PRIVATE (self)->nag_ignored = ignored;
}

static void
model_free (GtkTreeModel *model, guint col)
{
    GtkTreeIter iter;

    if (!model)
        return;

    if (gtk_tree_model_get_iter_first (model, &iter)) {
        do {
            char *str;

            gtk_tree_model_get (model, &iter, col, &str, -1);
            g_free (str);
        } while (gtk_tree_model_iter_next (model, &iter));
    }
    g_object_unref (model);
}

static void
size_group_clear (GtkSizeGroup *group)
{
    GSList *children;
    GSList *iter;

    g_return_if_fail (group != NULL);

    children = gtk_size_group_get_widgets (group);
    for (iter = children; iter; iter = g_slist_next (iter))
        gtk_size_group_remove_widget (group, GTK_WIDGET (iter->data));
}

static void
size_group_add_permanent (GtkSizeGroup *group,
                          GtkBuilder *builder)
{
    GtkWidget *widget;

    widget = GTK_WIDGET (gtk_builder_get_object (builder, "network_name_label"));
    gtk_size_group_add_widget (group, widget);

    widget = GTK_WIDGET (gtk_builder_get_object (builder, "security_combo_label"));
    gtk_size_group_add_widget (group, widget);

    widget = GTK_WIDGET (gtk_builder_get_object (builder, "device_label"));
    gtk_size_group_add_widget (group, widget);
}

static void
security_combo_changed (GtkWidget *combo,
                        gpointer user_data)
{
    NMAWirelessDialog *self = NMA_WIRELESS_DIALOG (user_data);
    NMAWirelessDialogPrivate *priv = NMA_WIRELESS_DIALOG_GET_PRIVATE (self);
    GtkWidget *vbox;
    GList *elt, *children;
    GtkTreeIter iter;
    GtkTreeModel *model;
    WirelessSecurity *sec = NULL;
    GtkWidget *sec_widget;

    vbox = GTK_WIDGET (gtk_builder_get_object (priv->builder, "security_vbox"));
    g_assert (vbox);

    size_group_clear (priv->group);

    /* Remove any previous wireless security widgets */
    children = gtk_container_get_children (GTK_CONTAINER (vbox));
    for (elt = children; elt; elt = g_list_next (elt))
        gtk_container_remove (GTK_CONTAINER (vbox), GTK_WIDGET (elt->data));

    model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));
    if (!gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo), &iter)) {
        g_warning ("%s: no active security combo box item.", __func__);
        return;
    }

    gtk_tree_model_get (model, &iter, S_SEC_COLUMN, &sec, -1);
    if (!sec)
        return;

    sec_widget = wireless_security_get_widget (sec);
    g_assert (sec_widget);

    size_group_add_permanent (priv->group, priv->builder);
    wireless_security_add_to_size_group (sec, priv->group);

    if (gtk_widget_get_parent (sec_widget))
        gtk_widget_reparent (sec_widget, vbox);
    else
        gtk_container_add (GTK_CONTAINER (vbox), sec_widget);

    wireless_security_unref (sec);

    /* Re-validate */
    wireless_security_changed_cb (NULL, sec);
}

static GByteArray *
validate_dialog_ssid (NMAWirelessDialog *self)
{
    NMAWirelessDialogPrivate *priv = NMA_WIRELESS_DIALOG_GET_PRIVATE (self);
    GtkWidget *widget;
    const char *ssid;
    guint32 ssid_len;
    GByteArray *ssid_ba;

    widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "network_name_entry"));

    ssid = gtk_entry_get_text (GTK_ENTRY (widget));
    ssid_len = strlen (ssid);
    
    if (!ssid || !ssid_len || (ssid_len > 32))
        return NULL;

    ssid_len = strlen (ssid);
    ssid_ba = g_byte_array_sized_new (ssid_len);
    g_byte_array_append (ssid_ba, (unsigned char *) ssid, ssid_len);
    return ssid_ba;
}

static void
stuff_changed_cb (WirelessSecurity *sec, gpointer user_data)
{
    NMAWirelessDialog *self = NMA_WIRELESS_DIALOG (user_data);
    NMAWirelessDialogPrivate *priv = NMA_WIRELESS_DIALOG_GET_PRIVATE (self);
    GByteArray *ssid = NULL;
    gboolean free_ssid = TRUE;
    gboolean valid = FALSE;
    
    if (priv->connection) {
        NMSettingWireless *s_wireless;
        s_wireless = NM_SETTING_WIRELESS (nm_connection_get_setting (priv->connection, NM_TYPE_SETTING_WIRELESS));
        g_assert (s_wireless);
        ssid = (GByteArray *) nm_setting_wireless_get_ssid (s_wireless);
        free_ssid = FALSE;
    } else {
        ssid = validate_dialog_ssid (self);
    }

    if (ssid) {
        valid = wireless_security_validate (sec, ssid);
        if (free_ssid)
            g_byte_array_free (ssid, TRUE);
    }

    gtk_dialog_set_response_sensitive (GTK_DIALOG (self), GTK_RESPONSE_OK, valid);
}

static void
ssid_entry_changed (GtkWidget *entry, gpointer user_data)
{
    NMAWirelessDialog *self = NMA_WIRELESS_DIALOG (user_data);
    NMAWirelessDialogPrivate *priv = NMA_WIRELESS_DIALOG_GET_PRIVATE (self);
    GtkTreeIter iter;
    WirelessSecurity *sec = NULL;
    GtkTreeModel *model;
    gboolean valid = FALSE;
    GByteArray *ssid;

    ssid = validate_dialog_ssid (self);
    if (!ssid)
        goto out;

    model = gtk_combo_box_get_model (GTK_COMBO_BOX (priv->sec_combo));
    if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (priv->sec_combo), &iter))
        gtk_tree_model_get (model, &iter, S_SEC_COLUMN, &sec, -1);

    if (sec) {
        valid = wireless_security_validate (sec, ssid);
        wireless_security_unref (sec);
    } else {
        valid = TRUE;
    }

 out:
    gtk_dialog_set_response_sensitive (GTK_DIALOG (self), GTK_RESPONSE_OK, valid);
}

static void
connection_combo_changed (GtkWidget *combo,
                          gpointer user_data)
{
    NMAWirelessDialog *self = NMA_WIRELESS_DIALOG (user_data);
    NMAWirelessDialogPrivate *priv = NMA_WIRELESS_DIALOG_GET_PRIVATE (self);
    GtkTreeIter iter;
    GtkTreeModel *model;
    gboolean is_new = FALSE;
    NMSettingWireless *s_wireless;
    char *utf8_ssid;
    GtkWidget *widget;

    gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo), &iter);
    model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));

    if (priv->connection)
        g_object_unref (priv->connection);

    gtk_tree_model_get (model, &iter,
                        C_CON_COLUMN, &priv->connection,
                        C_NEW_COLUMN, &is_new, -1);

    if (!security_combo_init (self)) {
        g_warning ("Couldn't change wireless security combo box.");
        return;
    }
    security_combo_changed (priv->sec_combo, self);

    widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "network_name_entry"));
    if (priv->connection) {
        const GByteArray *ssid;

        s_wireless = NM_SETTING_WIRELESS (nm_connection_get_setting (priv->connection, NM_TYPE_SETTING_WIRELESS));
        ssid = nm_setting_wireless_get_ssid (s_wireless);
        utf8_ssid = nm_utils_ssid_to_utf8 ((const char *) ssid->data, ssid->len);
        gtk_entry_set_text (GTK_ENTRY (widget), utf8_ssid);
        g_free (utf8_ssid);
    } else {
        gtk_entry_set_text (GTK_ENTRY (widget), "");
    }

    gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (priv->builder, "network_name_entry")), is_new);
    gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (priv->builder, "network_name_label")), is_new);
    gtk_widget_set_sensitive (priv->sec_combo, is_new);
    gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (priv->builder, "security_combo_label")), is_new);
    gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (priv->builder, "security_vbox")), is_new);
}

static gboolean
connection_combo_separator_cb (GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
    gboolean is_separator = FALSE;

    gtk_tree_model_get (model, iter, C_SEP_COLUMN, &is_separator, -1);
    return is_separator;
}

static gboolean
connection_combo_init (NMAWirelessDialog *self, NMConnection *connection)
{
    NMAWirelessDialogPrivate *priv = NMA_WIRELESS_DIALOG_GET_PRIVATE (self);
    GtkListStore *store;
    GtkTreeIter tree_iter;
    GtkWidget *widget;
    GtkCellRenderer *renderer;

    g_return_val_if_fail (priv->connection == NULL, FALSE);

    /* Clear any old model */
    model_free (priv->connection_model, C_NAME_COLUMN);

    /* New model */
    store = gtk_list_store_new (4, G_TYPE_STRING, G_TYPE_OBJECT, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN);
    priv->connection_model = GTK_TREE_MODEL (store);

    if (connection) {
        NMSettingConnection *s_con;
        const char *id;

        s_con = NM_SETTING_CONNECTION (nm_connection_get_setting (connection, NM_TYPE_SETTING_CONNECTION));
        g_assert (s_con);

        id = nm_setting_connection_get_id (s_con);
        g_assert (id);

        gtk_list_store_append (store, &tree_iter);
        gtk_list_store_set (store, &tree_iter,
                            C_NAME_COLUMN, id,
                            C_CON_COLUMN, connection, -1);

        priv->connection = g_object_ref (connection);
    }

    widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "connection_combo"));

    gtk_cell_layout_clear (GTK_CELL_LAYOUT (widget));
    renderer = gtk_cell_renderer_text_new ();
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (widget), renderer, TRUE);
    gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (widget), renderer,
                                   "text", C_NAME_COLUMN);
    gtk_combo_box_set_wrap_width (GTK_COMBO_BOX (widget), 1);

    gtk_combo_box_set_model (GTK_COMBO_BOX (widget), priv->connection_model);

    gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (widget),
                                          connection_combo_separator_cb,
                                          NULL,
                                          NULL);

    gtk_combo_box_set_active (GTK_COMBO_BOX (widget), 0);
    g_signal_connect (G_OBJECT (widget), "changed",
                      G_CALLBACK (connection_combo_changed), self);

    gtk_widget_hide (GTK_WIDGET (gtk_builder_get_object (priv->builder, "connection_label")));
    gtk_widget_hide (widget);

    return TRUE;
}

static void
device_combo_changed (GtkWidget *combo,
                      gpointer user_data)
{
    NMAWirelessDialog *self = NMA_WIRELESS_DIALOG (user_data);
    NMAWirelessDialogPrivate *priv = NMA_WIRELESS_DIALOG_GET_PRIVATE (self);
    GtkTreeIter iter;
    GtkTreeModel *model;

    gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo), &iter);
    model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));

    g_object_unref (priv->device);
    gtk_tree_model_get (model, &iter, D_DEV_COLUMN, &priv->device, -1);

    if (!connection_combo_init (self, NULL)) {
        g_warning ("Couldn't change connection combo box.");
        return;
    }

    if (!security_combo_init (self)) {
        g_warning ("Couldn't change wireless security combo box.");
        return;
    }

    security_combo_changed (priv->sec_combo, self);
}

static void
add_device_to_model (GtkListStore *model, NMDevice *device)
{
    GtkTreeIter iter;
    char *desc;

    desc = (char *) utils_get_device_description (device);
    if (!desc)
        desc = (char *) nm_device_get_iface (device);
    g_assert (desc);

    gtk_list_store_append (model, &iter);
    gtk_list_store_set (model, &iter, D_NAME_COLUMN, desc, D_DEV_COLUMN, device, -1);
}

static gboolean
can_use_device (NMDevice *device)
{
    /* Ignore unsupported devices */
    if (!(nm_device_get_capabilities (device) & NM_DEVICE_CAP_NM_SUPPORTED))
        return FALSE;

    if (!NM_IS_DEVICE_WIFI (device))
        return FALSE;

    if (nm_device_get_state (device) < NM_DEVICE_STATE_DISCONNECTED)
        return FALSE;

    return TRUE;
}

static gboolean
device_combo_init (NMAWirelessDialog *self, NMDevice *device)
{
    NMAWirelessDialogPrivate *priv = NMA_WIRELESS_DIALOG_GET_PRIVATE (self);
    const GPtrArray *devices;
    GtkListStore *store;
    int i, num_added = 0;

    g_return_val_if_fail (priv->device == NULL, FALSE);

    store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_OBJECT);
    priv->device_model = GTK_TREE_MODEL (store);

    if (device) {
        if (!can_use_device (device))
            return FALSE;
        add_device_to_model (store, device);
        num_added++;
    } else {
        devices = nm_client_get_devices (priv->client);
        if (devices->len == 0)
            return FALSE;

        for (i = 0; devices && (i < devices->len); i++) {
            device = NM_DEVICE (g_ptr_array_index (devices, i));
            if (can_use_device (device)) {
                add_device_to_model (store, device);
                num_added++;
            }
        }
    }

    if (num_added > 0) {
        GtkWidget *widget;
        GtkTreeIter iter;

        widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "device_combo"));
        gtk_combo_box_set_model (GTK_COMBO_BOX (widget), priv->device_model);
        gtk_combo_box_set_active (GTK_COMBO_BOX (widget), 0);
        g_signal_connect (G_OBJECT (widget), "changed",
                          G_CALLBACK (device_combo_changed), self);
        if (num_added == 1) {
            gtk_widget_hide (GTK_WIDGET (gtk_builder_get_object (priv->builder, "device_label")));
            gtk_widget_hide (widget);
        }
        gtk_tree_model_get_iter_first (priv->device_model, &iter);
        gtk_tree_model_get (priv->device_model, &iter, D_DEV_COLUMN, &priv->device, -1);
    }

    return num_added > 0 ? TRUE : FALSE;
}

static gboolean
find_proto (NMSettingWirelessSecurity *sec, const char *item)
{
    guint32 i;

    for (i = 0; i < nm_setting_wireless_security_get_num_protos (sec); i++) {
        if (!strcmp (item, nm_setting_wireless_security_get_proto (sec, i)))
            return TRUE;
    }
    return FALSE;
}

static NMUtilsSecurityType
get_default_type_for_security (NMSettingWirelessSecurity *sec,
                               gboolean have_ap,
                               guint32 ap_flags,
                               guint32 dev_caps)
{
    const char *key_mgmt, *auth_alg;

    g_return_val_if_fail (sec != NULL, NMU_SEC_NONE);

    key_mgmt = nm_setting_wireless_security_get_key_mgmt (sec);
    auth_alg = nm_setting_wireless_security_get_auth_alg (sec);

    /* No IEEE 802.1x */
    if (!strcmp (key_mgmt, "none"))
        return NMU_SEC_STATIC_WEP;

    if (   !strcmp (key_mgmt, "ieee8021x")
           && (!have_ap || (ap_flags & NM_802_11_AP_FLAGS_PRIVACY))) {
        if (auth_alg && !strcmp (auth_alg, "leap"))
            return NMU_SEC_LEAP;
        return NMU_SEC_DYNAMIC_WEP;
    }

    if (   !strcmp (key_mgmt, "wpa-none")
           || !strcmp (key_mgmt, "wpa-psk")) {
        if (!have_ap || (ap_flags & NM_802_11_AP_FLAGS_PRIVACY)) {
            if (find_proto (sec, "rsn"))
                return NMU_SEC_WPA2_PSK;
            else if (find_proto (sec, "wpa"))
                return NMU_SEC_WPA_PSK;
            else
                return NMU_SEC_WPA_PSK;
        }
    }

    if (   !strcmp (key_mgmt, "wpa-eap")
           && (!have_ap || (ap_flags & NM_802_11_AP_FLAGS_PRIVACY))) {
        if (find_proto (sec, "rsn"))
            return NMU_SEC_WPA2_ENTERPRISE;
        else if (find_proto (sec, "wpa"))
            return NMU_SEC_WPA_ENTERPRISE;
        else
            return NMU_SEC_WPA_ENTERPRISE;
    }

    return NMU_SEC_INVALID;
}

static void
add_security_item (NMAWirelessDialog *self,
                   WirelessSecurity *sec,
                   GtkListStore *model,
                   GtkTreeIter *iter,
                   const char *text)
{
    wireless_security_set_changed_notify (sec, stuff_changed_cb, self);
    gtk_list_store_append (model, iter);
    gtk_list_store_set (model, iter, S_NAME_COLUMN, text, S_SEC_COLUMN, sec, -1);
    wireless_security_unref (sec);
}

static gboolean
security_combo_init (NMAWirelessDialog *self)
{
    NMAWirelessDialogPrivate *priv;
    GtkListStore *sec_model;
    GtkTreeIter iter;
    guint32 ap_flags = 0;
    guint32 ap_wpa = 0;
    guint32 ap_rsn = 0;
    guint32 dev_caps;
    NMSettingWirelessSecurity *wsec = NULL;
    NMUtilsSecurityType default_type = NMU_SEC_NONE;
    NMWepKeyType wep_type = NM_WEP_KEY_TYPE_KEY;
    int active = -1;
    int item = 0;
    NMSettingWireless *s_wireless = NULL;
    gboolean is_adhoc;

    g_return_val_if_fail (self != NULL, FALSE);

    priv = NMA_WIRELESS_DIALOG_GET_PRIVATE (self);
    g_return_val_if_fail (priv->device != NULL, FALSE);
    g_return_val_if_fail (priv->sec_combo != NULL, FALSE);

    is_adhoc = priv->adhoc_create;

    /* The security options displayed are filtered based on device
     * capabilities, and if provided, additionally by access point capabilities.
     * If a connection is given, that connection's options should be selected
     * by default.
     */
    dev_caps = nm_device_wifi_get_capabilities (NM_DEVICE_WIFI (priv->device));
    if (priv->ap != NULL) {
        ap_flags = nm_access_point_get_flags (priv->ap);
        ap_wpa = nm_access_point_get_wpa_flags (priv->ap);
        ap_rsn = nm_access_point_get_rsn_flags (priv->ap);
    }

    if (priv->connection) {
        const char *mode;
        const char *security;

        s_wireless = NM_SETTING_WIRELESS (nm_connection_get_setting (priv->connection, NM_TYPE_SETTING_WIRELESS));

        mode = nm_setting_wireless_get_mode (s_wireless);
        if (mode && !strcmp (mode, "adhoc"))
            is_adhoc = TRUE;

        wsec = NM_SETTING_WIRELESS_SECURITY (nm_connection_get_setting (priv->connection, 
                                                                        NM_TYPE_SETTING_WIRELESS_SECURITY));

        security = nm_setting_wireless_get_security (s_wireless);
        if (!security || strcmp (security, NM_SETTING_WIRELESS_SECURITY_SETTING_NAME))
            wsec = NULL;
        if (wsec) {
            default_type = get_default_type_for_security (wsec, !!priv->ap, ap_flags, dev_caps);
            if (default_type == NMU_SEC_STATIC_WEP)
                wep_type = nm_setting_wireless_security_get_wep_key_type (wsec);
            if (wep_type == NM_WEP_KEY_TYPE_UNKNOWN)
                wep_type = NM_WEP_KEY_TYPE_KEY;
        }
    } else if (is_adhoc) {
        default_type = NMU_SEC_STATIC_WEP;
        wep_type = NM_WEP_KEY_TYPE_PASSPHRASE;
    }

    sec_model = gtk_list_store_new (2, G_TYPE_STRING, wireless_security_get_g_type ());

    if (nm_utils_security_valid (NMU_SEC_NONE, dev_caps, !!priv->ap, is_adhoc, ap_flags, ap_wpa, ap_rsn)) {
        gtk_list_store_append (sec_model, &iter);
        gtk_list_store_set (sec_model, &iter,
                            S_NAME_COLUMN, _("None"),
                            -1);
        if (default_type == NMU_SEC_NONE)
            active = item;
        item++;
    }

    /* Don't show Static WEP if both the AP and the device are capable of WPA,
     * even though technically it's possible to have this configuration.
     */
    if (   nm_utils_security_valid (NMU_SEC_STATIC_WEP, dev_caps, !!priv->ap, is_adhoc, ap_flags, ap_wpa, ap_rsn)
           && ((!ap_wpa && !ap_rsn) || !(dev_caps & (NM_WIFI_DEVICE_CAP_WPA | NM_WIFI_DEVICE_CAP_RSN)))) {
        WirelessSecurityWEPKey *ws_wep;

        ws_wep = ws_wep_key_new (priv->connection, NM_WEP_KEY_TYPE_KEY, priv->adhoc_create);
        if (ws_wep) {
            add_security_item (self, WIRELESS_SECURITY (ws_wep), sec_model,
                               &iter, _("WEP 40/128-bit Key"));
            if ((active < 0) && (default_type == NMU_SEC_STATIC_WEP) && (wep_type == NM_WEP_KEY_TYPE_KEY))
                active = item;
            item++;
        }

        ws_wep = ws_wep_key_new (priv->connection, NM_WEP_KEY_TYPE_PASSPHRASE, priv->adhoc_create);
        if (ws_wep) {
            add_security_item (self, WIRELESS_SECURITY (ws_wep), sec_model,
                               &iter, _("WEP 128-bit Passphrase"));
            if ((active < 0) && (default_type == NMU_SEC_STATIC_WEP) && (wep_type == NM_WEP_KEY_TYPE_PASSPHRASE))
                active = item;
            item++;
        }
    }

    /* Don't show LEAP if both the AP and the device are capable of WPA,
     * even though technically it's possible to have this configuration.
     */
    if (   nm_utils_security_valid (NMU_SEC_LEAP, dev_caps, !!priv->ap, is_adhoc, ap_flags, ap_wpa, ap_rsn)
           && ((!ap_wpa && !ap_rsn) || !(dev_caps & (NM_WIFI_DEVICE_CAP_WPA | NM_WIFI_DEVICE_CAP_RSN)))) {
        WirelessSecurityLEAP *ws_leap;

        ws_leap = ws_leap_new (priv->connection);
        if (ws_leap) {
            add_security_item (self, WIRELESS_SECURITY (ws_leap), sec_model,
                               &iter, _("LEAP"));
            if ((active < 0) && (default_type == NMU_SEC_LEAP))
                active = item;
            item++;
        }
    }

    if (nm_utils_security_valid (NMU_SEC_DYNAMIC_WEP, dev_caps, !!priv->ap, is_adhoc, ap_flags, ap_wpa, ap_rsn)) {
        WirelessSecurityDynamicWEP *ws_dynamic_wep;

        ws_dynamic_wep = ws_dynamic_wep_new (priv->connection);
        if (ws_dynamic_wep) {
            add_security_item (self, WIRELESS_SECURITY (ws_dynamic_wep), sec_model,
                               &iter, _("Dynamic WEP (802.1x)"));
            if ((active < 0) && (default_type == NMU_SEC_DYNAMIC_WEP))
                active = item;
            item++;
        }
    }

    if (   nm_utils_security_valid (NMU_SEC_WPA_PSK, dev_caps, !!priv->ap, is_adhoc, ap_flags, ap_wpa, ap_rsn)
           || nm_utils_security_valid (NMU_SEC_WPA2_PSK, dev_caps, !!priv->ap, is_adhoc, ap_flags, ap_wpa, ap_rsn)) {
        WirelessSecurityWPAPSK *ws_wpa_psk;

        ws_wpa_psk = ws_wpa_psk_new (priv->connection);
        if (ws_wpa_psk) {
            add_security_item (self, WIRELESS_SECURITY (ws_wpa_psk), sec_model,
                               &iter, _("WPA & WPA2 Personal"));
            if ((active < 0) && ((default_type == NMU_SEC_WPA_PSK) || (default_type == NMU_SEC_WPA2_PSK)))
                active = item;
            item++;
        }
    }

    if (   nm_utils_security_valid (NMU_SEC_WPA_ENTERPRISE, dev_caps, !!priv->ap, is_adhoc, ap_flags, ap_wpa, ap_rsn)
           || nm_utils_security_valid (NMU_SEC_WPA2_ENTERPRISE, dev_caps, !!priv->ap, is_adhoc, ap_flags, ap_wpa, ap_rsn)) {
        WirelessSecurityWPAEAP *ws_wpa_eap;

        ws_wpa_eap = ws_wpa_eap_new (priv->connection);
        if (ws_wpa_eap) {
            add_security_item (self, WIRELESS_SECURITY (ws_wpa_eap), sec_model,
                               &iter, _("WPA & WPA2 Enterprise"));
            if ((active < 0) && ((default_type == NMU_SEC_WPA_ENTERPRISE) || (default_type == NMU_SEC_WPA2_ENTERPRISE)))
                active = item;
            item++;
        }
    }

    gtk_combo_box_set_model (GTK_COMBO_BOX (priv->sec_combo), GTK_TREE_MODEL (sec_model));
    gtk_combo_box_set_active (GTK_COMBO_BOX (priv->sec_combo), active < 0 ? 0 : (guint32) active);
    g_object_unref (G_OBJECT (sec_model));
    return TRUE;
}

static gboolean
revalidate (gpointer user_data)
{
    NMAWirelessDialog *self = NMA_WIRELESS_DIALOG (user_data);
    NMAWirelessDialogPrivate *priv = NMA_WIRELESS_DIALOG_GET_PRIVATE (self);

    security_combo_changed (priv->sec_combo, self);
    return FALSE;
}

static gboolean
internal_init (NMAWirelessDialog *self,
               NMConnection *specific_connection,
               NMDevice *specific_device,
               gboolean auth_only,
               gboolean create)
{
    NMAWirelessDialogPrivate *priv = NMA_WIRELESS_DIALOG_GET_PRIVATE (self);
    GtkWidget *widget;
    char *label, *icon_name = "network-wireless";
    gboolean security_combo_focus = FALSE;

    gtk_window_set_position (GTK_WINDOW (self), GTK_WIN_POS_CENTER_ALWAYS);
    gtk_container_set_border_width (GTK_CONTAINER (self), 6);
    gtk_window_set_default_size (GTK_WINDOW (self), 488, -1);
    gtk_window_set_resizable (GTK_WINDOW (self), FALSE);
    gtk_dialog_set_has_separator (GTK_DIALOG (self), FALSE);

    if (auth_only)
        icon_name = "dialog-password";
    else
        icon_name = "network-wireless";

    gtk_window_set_icon_name (GTK_WINDOW (self), icon_name);
    widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "image1"));
    gtk_image_set_from_icon_name (GTK_IMAGE (widget), icon_name, GTK_ICON_SIZE_DIALOG);

    gtk_box_set_spacing (GTK_BOX (gtk_bin_get_child (GTK_BIN (self))), 2);

    widget = gtk_dialog_add_button (GTK_DIALOG (self), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
    gtk_box_set_child_packing (GTK_BOX (GTK_DIALOG (self)->action_area), widget,
                               FALSE, TRUE, 0, GTK_PACK_END);

    /* Connect/Create button */
    if (create) {
        GtkWidget *image;

        widget = gtk_button_new_with_mnemonic (_("C_reate"));
        image = gtk_image_new_from_stock (GTK_STOCK_CONNECT, GTK_ICON_SIZE_BUTTON);
        gtk_button_set_image (GTK_BUTTON (widget), image);

        gtk_widget_show (widget);
        gtk_dialog_add_action_widget (GTK_DIALOG (self), widget, GTK_RESPONSE_OK);
    } else
        widget = gtk_dialog_add_button (GTK_DIALOG (self), GTK_STOCK_CONNECT, GTK_RESPONSE_OK);

    gtk_box_set_child_packing (GTK_BOX (GTK_DIALOG (self)->action_area), widget,
                               FALSE, TRUE, 0, GTK_PACK_END);
    g_object_set (G_OBJECT (widget), "can-default", TRUE, NULL);
    gtk_widget_grab_default (widget);

    widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "hbox1"));
    if (!widget) {
        nm_warning ("Couldn't find UI wireless_dialog widget.");
        return FALSE;
    }

    gtk_widget_reparent (widget, gtk_bin_get_child (GTK_BIN (self)));

    /* If given a valid connection, hide the SSID bits and connection combo */
    if (specific_connection) {
        widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "network_name_label"));
        gtk_widget_hide (widget);

        widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "network_name_entry"));
        gtk_widget_hide (widget);

        security_combo_focus = TRUE;
    } else {
        widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "network_name_entry"));
        g_signal_connect (G_OBJECT (widget), "changed", (GCallback) ssid_entry_changed, self);
        gtk_widget_grab_focus (widget);
    }

    gtk_dialog_set_response_sensitive (GTK_DIALOG (self), GTK_RESPONSE_OK, FALSE);

    if (!device_combo_init (self, specific_device)) {
        g_warning ("No wireless devices available.");
        return FALSE;
    }

    if (!connection_combo_init (self, specific_connection)) {
        g_warning ("Couldn't set up connection combo box.");
        return FALSE;
    }

    if (!security_combo_init (self)) {
        g_warning ("Couldn't set up wireless security combo box.");
        return FALSE;
    }

    if (security_combo_focus)
        gtk_widget_grab_focus (priv->sec_combo);

    security_combo_changed (priv->sec_combo, self);
    g_signal_connect (G_OBJECT (priv->sec_combo), "changed", G_CALLBACK (security_combo_changed), self);

    if (priv->connection) {
        char *tmp;
        char *esc_ssid = NULL;
        NMSettingWireless *s_wireless;
        const GByteArray *ssid;

        s_wireless = NM_SETTING_WIRELESS (nm_connection_get_setting (priv->connection, NM_TYPE_SETTING_WIRELESS));
        ssid = s_wireless ? nm_setting_wireless_get_ssid (s_wireless) : NULL;
        if (ssid)
            esc_ssid = nm_utils_ssid_to_utf8 ((const char *) ssid->data, ssid->len);

        tmp = g_strdup_printf (_("Passwords or encryption keys are required to access the wireless network '%s'."),
                               esc_ssid ? esc_ssid : "<unknown>");
        gtk_window_set_title (GTK_WINDOW (self), _("Wireless Network Authentication Required"));
        label = g_strdup_printf ("<span size=\"larger\" weight=\"bold\">%s</span>\n\n%s",
                                 _("Authentication required by wireless network"),
                                 tmp);
        g_free (esc_ssid);
        g_free (tmp);
    } else if (priv->adhoc_create) {
        gtk_window_set_title (GTK_WINDOW (self), _("Create New Wireless Network"));
        label = g_strdup_printf ("<span size=\"larger\" weight=\"bold\">%s</span>\n\n%s",
                                 _("New wireless network"),
                                 _("Enter a name for the wireless network you wish to create."));
    } else {
        gtk_window_set_title (GTK_WINDOW (self), _("Connect to Hidden Wireless Network"));
        label = g_strdup_printf ("<span size=\"larger\" weight=\"bold\">%s</span>\n\n%s",
                                 _("Hidden wireless network"),
                                 _("Enter the name and security details of the hidden wireless network you wish to connect to."));
    }

    widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "caption_label"));
    gtk_label_set_markup (GTK_LABEL (widget), label);
    g_free (label);

    /* Re-validate from an idle handler so that widgets like file choosers
     * have had time to find their files.
     */
    g_idle_add (revalidate, self);

    return TRUE;
}

NMConnection *
nma_wireless_dialog_get_connection (NMAWirelessDialog *self)
{
    g_return_val_if_fail (NMA_IS_WIRELESS_DIALOG (self), NULL);

    return NMA_WIRELESS_DIALOG_GET_PRIVATE (self)->connection;
}

NMDevice *
nma_wireless_dialog_get_device (NMAWirelessDialog *self)
{
    NMAWirelessDialogPrivate *priv;
    GObject *combo;
    NMDevice *device = NULL;
    GtkTreeIter iter;

    g_return_val_if_fail (NMA_IS_WIRELESS_DIALOG (self), NULL);

    priv = NMA_WIRELESS_DIALOG_GET_PRIVATE (self);
    combo = gtk_builder_get_object (priv->builder, "device_combo");
    gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo), &iter);
    gtk_tree_model_get (priv->device_model, &iter, D_DEV_COLUMN, &device, -1);

    if (device)
        g_object_unref (device);

    return device;
}

NMAccessPoint *
nma_wireless_dialog_get_ap (NMAWirelessDialog *self)
{
    g_return_val_if_fail (NMA_IS_WIRELESS_DIALOG (self), NULL);

    return NMA_WIRELESS_DIALOG_GET_PRIVATE (self)->ap;
}

GtkWidget *
nma_wireless_dialog_new (NMClient *client,
                         NMConnection *connection,
                         NMDevice *device,
                         NMAccessPoint *ap)
{
    NMAWirelessDialog *self;
    NMAWirelessDialogPrivate *priv;
    guint32 dev_caps;

    g_return_val_if_fail (NM_IS_CLIENT (client), NULL);
    g_return_val_if_fail (NM_IS_CONNECTION (connection), NULL);
    g_return_val_if_fail (NM_IS_DEVICE (device), NULL);
    g_return_val_if_fail (NM_IS_ACCESS_POINT (ap), NULL);

    /* Ensure device validity */
    dev_caps = nm_device_get_capabilities (device);
    g_return_val_if_fail (dev_caps & NM_DEVICE_CAP_NM_SUPPORTED, NULL);
    g_return_val_if_fail (NM_IS_DEVICE_WIFI (device), NULL);

    self = NMA_WIRELESS_DIALOG (g_object_new (NMA_TYPE_WIRELESS_DIALOG, NULL));
    if (!self)
        return NULL;

    priv = NMA_WIRELESS_DIALOG_GET_PRIVATE (self);

    priv->client = g_object_ref (client);
    priv->ap = g_object_ref (ap);

    if (!internal_init (self, connection, device, TRUE, FALSE)) {
        nm_warning ("Couldn't create wireless security dialog.");
        g_object_unref (self);
        return NULL;
    }

    return GTK_WIDGET (self);
}

GtkWidget *
nma_wireless_dialog_hidden_new (NMClient *client)
{
    NMAWirelessDialog *self;
    NMAWirelessDialogPrivate *priv;

    g_return_val_if_fail (NM_IS_CLIENT (client), NULL);

    self = NMA_WIRELESS_DIALOG (g_object_new (NMA_TYPE_WIRELESS_DIALOG, NULL));
    if (!self)
        return NULL;

    priv = NMA_WIRELESS_DIALOG_GET_PRIVATE (self);

    priv->client = g_object_ref (client);

    if (!internal_init (self, NULL, NULL, FALSE, FALSE)) {
        nm_warning ("Couldn't create wireless security dialog.");
        g_object_unref (self);
        return NULL;
    }

    return GTK_WIDGET (self);
}

void
nma_wireless_dialog_show (NMAWirelessDialog *dialog)
{
    GtkWidget *widget;

    g_return_if_fail (NMA_IS_WIRELESS_DIALOG (dialog));

    widget = GTK_WIDGET (dialog);

    /* Prevent focus stealing */
    gtk_widget_realize (widget);
    gtk_widget_show (widget);
    gtk_window_present_with_time (GTK_WINDOW (widget), gdk_x11_get_server_time (widget->window));
}

static gboolean
wireless_dialog_close (gpointer user_data)
{
    GtkWidget *dialog = GTK_WIDGET (user_data);

    gtk_dialog_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
    return FALSE;
}

static void
nag_dialog_response_cb (GtkDialog *nag_dialog,
                        gint response,
                        gpointer user_data)
{
    NMAWirelessDialog *wireless_dialog = NMA_WIRELESS_DIALOG (user_data);

    if (response == GTK_RESPONSE_NO) {  /* user opted not to correct the warning */
        nma_wireless_dialog_set_nag_ignored (wireless_dialog, TRUE);
        g_idle_add (wireless_dialog_close, wireless_dialog);
    }
}

static void
dialog_response (GtkDialog *dialog,
                 gint response,
                 gpointer user_data)
{
    NMAWirelessDialog *self = NMA_WIRELESS_DIALOG (user_data);
    NMAWirelessDialogPrivate *priv = NMA_WIRELESS_DIALOG_GET_PRIVATE (self);
    NMSetting *setting;
    GtkTreeModel *model;
    GtkTreeIter iter;
    WirelessSecurity *sec = NULL;

    if (response != GTK_RESPONSE_OK)
        goto out;

    if (!priv->connection) {
        /* Create new connection */
        char *id;
        char *uuid;
        GByteArray *ssid;

        priv->connection = nm_connection_new ();

        /* Wireless setting */
        setting = nm_setting_wireless_new ();
        ssid = validate_dialog_ssid (self);
        g_object_set (setting, NM_SETTING_WIRELESS_SSID, ssid, NULL);
        nm_connection_add_setting (priv->connection, setting);

        if (ssid) {
            id = nm_utils_ssid_to_utf8 ((char *) ssid->data, ssid->len);
            g_byte_array_free (ssid, TRUE);
        } else
            id = NULL;

        /* Connection setting */
        setting = nm_setting_connection_new ();
        uuid = nm_utils_uuid_generate ();

        /* FIXME: don't autoconnect until the connection is successful at least once */
        /* Don't autoconnect adhoc networks by default for now */
        g_object_set (setting,
                      NM_SETTING_CONNECTION_TYPE, NM_SETTING_WIRELESS_SETTING_NAME,
                      NM_SETTING_CONNECTION_UUID, uuid,
                      NM_SETTING_CONNECTION_ID, id,
                      NM_SETTING_CONNECTION_AUTOCONNECT, !priv->adhoc_create,
                      NULL);

        g_free (uuid);
        g_free (id);
        nm_connection_add_setting (priv->connection, setting);

        /* IPv4 setting */
        if (priv->adhoc_create) {
            g_object_set (setting, NM_SETTING_WIRELESS_MODE, "adhoc", NULL);

            setting = nm_setting_ip4_config_new ();
            g_object_set (setting,
                          NM_SETTING_IP4_CONFIG_METHOD, NM_SETTING_IP4_CONFIG_METHOD_SHARED,
                          NULL);

            nm_connection_add_setting (priv->connection, setting);
        }
    }

    model = gtk_combo_box_get_model (GTK_COMBO_BOX (priv->sec_combo));
    gtk_combo_box_get_active_iter (GTK_COMBO_BOX (priv->sec_combo), &iter);
    gtk_tree_model_get (model, &iter, S_SEC_COLUMN, &sec, -1);

    if (sec && !priv->nag_ignored) {
        GtkWidget *nag_dialog;

        /* Nag the user about certificates or whatever.  Only destroy the dialog
         * if no nagging was done.
         */
        nag_dialog = wireless_security_nag_user (sec);
        if (nag_dialog) {
            gtk_window_set_transient_for (GTK_WINDOW (nag_dialog), GTK_WINDOW (dialog));
            g_signal_connect (nag_dialog, "response",
                              G_CALLBACK (nag_dialog_response_cb),
                              dialog);
            return;
        }
    }

    /* Fill security */
    if (sec) {
        wireless_security_fill_connection (sec, priv->connection);
        wireless_security_unref (sec);
    } else {
        /* Unencrypted */
        setting = nm_connection_get_setting (priv->connection, NM_TYPE_SETTING_WIRELESS);
        g_object_set (setting, NM_SETTING_WIRELESS_SEC, NULL, NULL);
    }

 out:
    g_signal_emit (self, signals[DONE], 0, response);
    nm_utils_dialog_done ();

    /* FIXME: clear security? */
}

static void
nma_wireless_dialog_init (NMAWirelessDialog *self)
{
    NMAWirelessDialogPrivate *priv = NMA_WIRELESS_DIALOG_GET_PRIVATE (self);
    GError *error = NULL;

    priv->builder = gtk_builder_new ();
    gtk_builder_add_from_file (priv->builder, UIDIR "/wireless-security.ui", &error);
    if (error) {
        g_error ("Could not load wireless security UI file: %s", error->message);
        g_error_free (error);
    }

    g_signal_connect (self, "response", G_CALLBACK (dialog_response), self);

    priv->sec_combo = GTK_WIDGET (gtk_builder_get_object (priv->builder, "security_combo"));
    priv->group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
}

static void
dispose (GObject *object)
{
    NMAWirelessDialogPrivate *priv = NMA_WIRELESS_DIALOG_GET_PRIVATE (object);

    if (!priv->disposed)
        return;

    priv->disposed = TRUE;

    model_free (priv->device_model, D_NAME_COLUMN);
    model_free (priv->connection_model, C_NAME_COLUMN);
    gtk_combo_box_set_model (GTK_COMBO_BOX (priv->sec_combo), NULL);

    if (priv->group)
        g_object_unref (priv->group);

    if (priv->connection)
        g_object_unref (priv->connection);

    if (priv->device)
        g_object_unref (priv->device);

    if (priv->ap)
        g_object_unref (priv->ap);

    g_object_unref (priv->builder);

    if (priv->client)
        g_object_unref (priv->client);

    G_OBJECT_CLASS (nma_wireless_dialog_parent_class)->dispose (object);
}

static void
nma_wireless_dialog_class_init (NMAWirelessDialogClass *class)
{
    GObjectClass *object_class = G_OBJECT_CLASS (class);

    g_type_class_add_private (object_class, sizeof (NMAWirelessDialogPrivate));

    /* virtual methods */
    object_class->dispose = dispose;

    /* Signals */
    signals[DONE] = g_signal_new ("done",
                                  G_OBJECT_CLASS_TYPE (class),
                                  G_SIGNAL_RUN_LAST,
                                  G_STRUCT_OFFSET (NMAWirelessDialogClass, done),
                                  NULL, NULL,
                                  g_cclosure_marshal_VOID__INT,
                                  G_TYPE_NONE, 1,
                                  G_TYPE_INT);
}
