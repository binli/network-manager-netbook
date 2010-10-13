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
#include <nm-setting-connection.h>
#include <nm-setting-wireless.h>
#include <nm-setting-wireless-security.h>
#include <nm-setting-8021x.h>
#include <nm-settings-interface.h>
#include <nm-utils.h>
#include "nm-wifi-item.h"
#include "wireless-dialog.h"
#include "nm-icon-cache.h"
#include "utils.h"

G_DEFINE_TYPE (NMWifiItem, nm_wifi_item, NM_TYPE_DEVICE_ITEM)

enum {
    PROP_0,
    PROP_AP,

    LAST_PROP
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), NM_TYPE_WIFI_ITEM, NMWifiItemPrivate))

typedef struct {
    GSList *ap_list;
    NMAccessPoint *current_ap;

    gboolean disposed;
} NMWifiItemPrivate;

NMListItem *
nm_wifi_item_new (NMClient *client,
                  NMDeviceWifi *device,
                  NMAccessPoint *ap,
                  NMSettingsConnectionInterface *connection)
{
    g_return_val_if_fail (NM_IS_CLIENT (client), NULL);
    g_return_val_if_fail (NM_IS_DEVICE_WIFI (device), NULL);
    g_return_val_if_fail (NM_IS_ACCESS_POINT (ap), NULL);

    return (NMListItem *) g_object_new (NM_TYPE_WIFI_ITEM,
                                        NM_LIST_ITEM_TYPE_NAME, _("WiFi"),
                                        NM_CONNECTION_ITEM_CLIENT, client,
                                        NM_CONNECTION_ITEM_CONNECTION, connection,
                                        NM_DEVICE_ITEM_DEVICE, device,
                                        NM_WIFI_ITEM_AP, ap,
                                        NULL);
}

NMAccessPoint *
nm_wifi_item_get_ap (NMWifiItem *self)
{
    g_return_val_if_fail (NM_IS_WIFI_ITEM (self), NULL);

    return GET_PRIVATE (self)->current_ap;
}

static void
update_icon (NMWifiItem *self)
{
    NMWifiItemPrivate *priv = GET_PRIVATE (self);
    const char *icon;
    guint strength;

    strength = CLAMP (nm_access_point_get_strength (priv->current_ap), 0, 100);
    if (strength > 80)
        icon = "nm-signal-100-active";
    else if (strength > 55)
        icon = "nm-signal-75-active";
    else if (strength > 30)
        icon = "nm-signal-50-active";
    else if (strength > 5)
        icon = "nm-signal-25-active";
    else
        icon = "nm-signal-00-active";

    if (icon)
        g_object_set (self, NM_LIST_ITEM_ICON, icon, NULL);
}

static void
update_current_ap (NMWifiItem *self)
{
    NMWifiItemPrivate *priv = GET_PRIVATE (self);
    GSList *iter;

    /* FIXME: If device is activated, use activation AP only */

    for (iter = priv->ap_list; iter; iter = iter->next) {
        NMAccessPoint *ap = (NMAccessPoint *) iter->data;

        if (!priv->current_ap || nm_access_point_get_strength (priv->current_ap) < nm_access_point_get_strength (ap))
            priv->current_ap = ap;
    }

    update_icon (self);
}

gboolean
nm_wifi_item_add_ap (NMWifiItem *self,
                     NMAccessPoint *ap)
{
    NMWifiItemPrivate *priv;

    g_return_val_if_fail (NM_IS_WIFI_ITEM (self), FALSE);
    g_return_val_if_fail (NM_IS_ACCESS_POINT (ap), FALSE);

    priv = GET_PRIVATE (self);
    if (priv->current_ap && !utils_access_point_is_compatible (priv->current_ap, ap))
        return FALSE;

    priv->ap_list = g_slist_prepend (priv->ap_list, g_object_ref (ap));
    update_current_ap (self);

    g_signal_connect_swapped (ap, "notify::" NM_ACCESS_POINT_STRENGTH, G_CALLBACK (update_current_ap), self);

    return TRUE;
}

void
nm_wifi_item_remove_ap (NMWifiItem *self,
                        NMAccessPoint *ap)
{
    NMWifiItemPrivate *priv;
    GSList *iter;

    g_return_if_fail (NM_IS_WIFI_ITEM (self));
    g_return_if_fail (NM_IS_ACCESS_POINT (ap));

    priv = GET_PRIVATE (self);
    for (iter = priv->ap_list; iter; iter = iter->next) {
        if (ap != iter->data)
            continue;

        g_signal_handlers_disconnect_matched (ap,
                                              G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA,
                                              0, 0, NULL, update_current_ap, self);

        priv->ap_list = g_slist_delete_link (priv->ap_list, iter);
        g_object_unref (ap);

        if (priv->ap_list)
            update_current_ap (self);
        else
            /* No APs left, die */
            nm_list_item_request_remove (NM_LIST_ITEM (self));

        break;
    }
}

static char *
wifi_get_specific_object (NMDeviceItem *item)
{
    NMAccessPoint *ap;

    ap = GET_PRIVATE (item)->current_ap;

    return g_strdup (nm_object_get_path (NM_OBJECT (ap)));
}

static const char *
wifi_get_hw_address (NMDeviceItem *item)
{
    NMDeviceWifi *device;

    device = NM_DEVICE_WIFI (nm_device_item_get_device (NM_DEVICE_ITEM (item)));

    return nm_device_wifi_get_hw_address (device);
}

static int
priority (NMListItem *item)
{
    return NM_LIST_ITEM_PRIORITY_DEV_WIFI + NM_LIST_ITEM_CLASS (nm_wifi_item_parent_class)->priority (item);
}

static const char * default_ssid_list[] = {
    "linksys",
    "linksys-a",
    "linksys-g",
    "default",
    "belkin54g",
    "NETGEAR",
    NULL
};

static gboolean
is_manufacturer_default_ssid (const GByteArray *ssid)
{
    const char **default_ssid = default_ssid_list;

    while (*default_ssid) {
        if (ssid->len == strlen (*default_ssid)) {
            if (!memcmp (*default_ssid, ssid->data, ssid->len))
                return TRUE;
        }
        default_ssid++;
    }
    return FALSE;
}

static void
add_ciphers_from_flags (NMSettingWirelessSecurity *sec,
                        guint32 flags,
                        gboolean pairwise)
{
    if (pairwise) {
        if (flags & NM_802_11_AP_SEC_PAIR_TKIP)
            nm_setting_wireless_security_add_pairwise (sec, "tkip");
        if (flags & NM_802_11_AP_SEC_PAIR_CCMP)
            nm_setting_wireless_security_add_pairwise (sec, "ccmp");
    } else {
        if (flags & NM_802_11_AP_SEC_GROUP_WEP40)
            nm_setting_wireless_security_add_group (sec, "wep40");
        if (flags & NM_802_11_AP_SEC_GROUP_WEP104)
            nm_setting_wireless_security_add_group (sec, "wep104");
        if (flags & NM_802_11_AP_SEC_GROUP_TKIP)
            nm_setting_wireless_security_add_group (sec, "tkip");
        if (flags & NM_802_11_AP_SEC_GROUP_CCMP)
            nm_setting_wireless_security_add_group (sec, "ccmp");
    }
}

static NMSettingWirelessSecurity *
get_security_for_ap (NMAccessPoint *ap,
                     guint32 dev_caps,
                     gboolean *supported,
                     NMSetting8021x **s_8021x)
{
    NMSettingWirelessSecurity *sec;
    NM80211Mode mode;
    guint32 flags;
    guint32 wpa_flags;
    guint32 rsn_flags;

    g_return_val_if_fail (NM_IS_ACCESS_POINT (ap), NULL);
    g_return_val_if_fail (supported != NULL, NULL);
    g_return_val_if_fail (*supported == TRUE, NULL);
    g_return_val_if_fail (s_8021x != NULL, NULL);
    g_return_val_if_fail (*s_8021x == NULL, NULL);

    sec = (NMSettingWirelessSecurity *) nm_setting_wireless_security_new ();

    mode = nm_access_point_get_mode (ap);
    flags = nm_access_point_get_flags (ap);
    wpa_flags = nm_access_point_get_wpa_flags (ap);
    rsn_flags = nm_access_point_get_rsn_flags (ap);

    /* No security */
    if (   !(flags & NM_802_11_AP_FLAGS_PRIVACY)
           && (wpa_flags == NM_802_11_AP_SEC_NONE)
           && (rsn_flags == NM_802_11_AP_SEC_NONE))
        goto none;

    /* Static WEP, Dynamic WEP, or LEAP */
    if (flags & NM_802_11_AP_FLAGS_PRIVACY) {
        if ((dev_caps & NM_WIFI_DEVICE_CAP_RSN) || (dev_caps & NM_WIFI_DEVICE_CAP_WPA)) {
            /* If the device can do WPA/RSN but the AP has no WPA/RSN informatoin
             * elements, it must be LEAP or static/dynamic WEP.
             */
            if ((wpa_flags == NM_802_11_AP_SEC_NONE) && (rsn_flags == NM_802_11_AP_SEC_NONE)) {
                g_object_set (sec,
                              NM_SETTING_WIRELESS_SECURITY_KEY_MGMT, "none",
                              NM_SETTING_WIRELESS_SECURITY_WEP_TX_KEYIDX, 0,
                              NULL);
                return sec;
            }
            /* Otherwise, the AP supports WPA or RSN, which is preferred */
        } else {
            /* Device can't do WPA/RSN, but can at least pass through the
             * WPA/RSN information elements from a scan.  Since Privacy was
             * advertised, LEAP or static/dynamic WEP must be in use.
             */
            g_object_set (sec,
                          NM_SETTING_WIRELESS_SECURITY_KEY_MGMT, "none",
                          NM_SETTING_WIRELESS_SECURITY_WEP_TX_KEYIDX, 0,
                          NULL);
            return sec;
        }
    }

    /* Stuff after this point requires infrastructure */
    if (mode != NM_802_11_MODE_INFRA) {
        *supported = FALSE;
        goto none;
    }

    /* WPA2 PSK first */
    if (   (rsn_flags & NM_802_11_AP_SEC_KEY_MGMT_PSK)
           && (dev_caps & NM_WIFI_DEVICE_CAP_RSN)) {
        g_object_set (sec, NM_SETTING_WIRELESS_SECURITY_KEY_MGMT, "wpa-psk", NULL);
        nm_setting_wireless_security_add_proto (sec, "rsn");
        add_ciphers_from_flags (sec, rsn_flags, TRUE);
        add_ciphers_from_flags (sec, rsn_flags, FALSE);
        return sec;
    }

    /* WPA PSK */
    if (   (wpa_flags & NM_802_11_AP_SEC_KEY_MGMT_PSK)
           && (dev_caps & NM_WIFI_DEVICE_CAP_WPA)) {
        g_object_set (sec, NM_SETTING_WIRELESS_SECURITY_KEY_MGMT, "wpa-psk", NULL);
        nm_setting_wireless_security_add_proto (sec, "wpa");
        add_ciphers_from_flags (sec, wpa_flags, TRUE);
        add_ciphers_from_flags (sec, wpa_flags, FALSE);
        return sec;
    }

    /* WPA2 Enterprise */
    if (   (rsn_flags & NM_802_11_AP_SEC_KEY_MGMT_802_1X)
           && (dev_caps & NM_WIFI_DEVICE_CAP_RSN)) {
        g_object_set (sec, NM_SETTING_WIRELESS_SECURITY_KEY_MGMT, "wpa-eap", NULL);
        nm_setting_wireless_security_add_proto (sec, "rsn");
        add_ciphers_from_flags (sec, rsn_flags, TRUE);
        add_ciphers_from_flags (sec, rsn_flags, FALSE);

        *s_8021x = NM_SETTING_802_1X (nm_setting_802_1x_new ());
        nm_setting_802_1x_add_eap_method (*s_8021x, "ttls");
        g_object_set (*s_8021x, NM_SETTING_802_1X_PHASE2_AUTH, "mschapv2", NULL);
        return sec;
    }

    /* WPA Enterprise */
    if (   (wpa_flags & NM_802_11_AP_SEC_KEY_MGMT_802_1X)
           && (dev_caps & NM_WIFI_DEVICE_CAP_WPA)) {
        g_object_set (sec, NM_SETTING_WIRELESS_SECURITY_KEY_MGMT, "wpa-eap", NULL);
        nm_setting_wireless_security_add_proto (sec, "wpa");
        add_ciphers_from_flags (sec, wpa_flags, TRUE);
        add_ciphers_from_flags (sec, wpa_flags, FALSE);

        *s_8021x = NM_SETTING_802_1X (nm_setting_802_1x_new ());
        nm_setting_802_1x_add_eap_method (*s_8021x, "ttls");
        g_object_set (*s_8021x, NM_SETTING_802_1X_PHASE2_AUTH, "mschapv2", NULL);
        return sec;
    }

    *supported = FALSE;

 none:
    g_object_unref (sec);
    return NULL;
}

static NMConnection *
create_connection (NMConnectionItem *item)
{
    NMConnection *connection;
    NMDeviceWifi *device;
    NMAccessPoint *ap;
    NMSetting *s_con;
    NMSetting *s_wireless;
    NMSettingWirelessSecurity *s_wireless_sec;
    NMSetting8021x *s_8021x = NULL;
    const GByteArray *ap_ssid;
    char *id;
    char buf[33];
    int buf_len;
    NM80211Mode mode;
    guint32 dev_caps;
    gboolean supported = TRUE;

    device = NM_DEVICE_WIFI (nm_device_item_get_device (NM_DEVICE_ITEM (item)));
    ap = nm_wifi_item_get_ap (NM_WIFI_ITEM (item));

    dev_caps = nm_device_wifi_get_capabilities (device);
    s_wireless_sec = get_security_for_ap (ap, dev_caps, &supported, &s_8021x);
    if (!supported)
        return NULL;

    if (NM_CONNECTION_ITEM_CLASS (nm_wifi_item_parent_class)->create_connection)
        connection = NM_CONNECTION_ITEM_CLASS (nm_wifi_item_parent_class)->create_connection (item);

    if (!connection)
        return NULL;

    s_wireless = nm_setting_wireless_new ();
    ap_ssid = nm_access_point_get_ssid (ap);
    g_object_set (s_wireless, NM_SETTING_WIRELESS_SSID, ap_ssid, NULL);

    mode = nm_access_point_get_mode (ap);
    if (mode == NM_802_11_MODE_ADHOC)
        g_object_set (s_wireless, NM_SETTING_WIRELESS_MODE, "adhoc", NULL);
    else if (mode == NM_802_11_MODE_INFRA)
        g_object_set (s_wireless, NM_SETTING_WIRELESS_MODE, "infrastructure", NULL);
    else
        g_assert_not_reached ();

    nm_connection_add_setting (connection, s_wireless);

    if (s_wireless_sec) {
        g_object_set (s_wireless, NM_SETTING_WIRELESS_SEC, NM_SETTING_WIRELESS_SECURITY_SETTING_NAME, NULL);
        nm_connection_add_setting (connection, NM_SETTING (s_wireless_sec));
    }
    if (s_8021x)
        nm_connection_add_setting (connection, NM_SETTING (s_8021x));

    s_con = nm_connection_get_setting (connection, NM_TYPE_SETTING_CONNECTION);
    g_object_set (s_con,
                  NM_SETTING_CONNECTION_TYPE, nm_setting_get_name (s_wireless),
                  NM_SETTING_CONNECTION_AUTOCONNECT, !is_manufacturer_default_ssid (ap_ssid),
                  NULL);

    memset (buf, 0, sizeof (buf));
    buf_len = MIN (ap_ssid->len, sizeof (buf) - 1);
    memcpy (buf, ap_ssid->data, buf_len);
    id = nm_utils_ssid_to_utf8 (buf, buf_len);
    g_object_set (s_con, NM_SETTING_CONNECTION_ID, id, NULL);
    g_free (id);

    return connection;
}

static void
connection_created_cb (GObject *source_object,
                       GAsyncResult *result,
                       gpointer user_data)
{
    NMConnectionItem *connection_item = NM_CONNECTION_ITEM (source_object);
    NMConnection *connection;

    connection = nm_connection_item_create_connection_finish (connection_item, result, NULL);
    if (connection) {
        nm_connection_item_new_connection (connection_item, connection, TRUE);
        g_object_unref (connection);
    }
}

static void
connect (NMListItem *item)
{
    NMConnectionItem *connection_item = NM_CONNECTION_ITEM (item);
    NMConnection *connection;

    connection = (NMConnection *) nm_connection_item_get_connection (connection_item);
    if (connection) {
        NM_LIST_ITEM_CLASS (nm_wifi_item_parent_class)->connect (item);
        return;
    }

    /* We don't have a connection yet, so create one */
    nm_connection_item_create_connection (connection_item, connection_created_cb, NULL);
}

static void
update_cb (NMSettingsConnectionInterface *connection,
           GError *error,
           gpointer user_data)
{
    if (error) {
        g_warning ("%s: failed to update connection: (%d) %s",
                   __func__, error->code, error->message);
    }
}

typedef struct {
    NMNewSecretsRequestedFunc callback;
    gpointer callback_data;
} SecretsRequestInfo;

static void
connection_secrets_response_cb (NMAWirelessDialog *dialog,
                                gint response,
                                gpointer user_data)
{
    SecretsRequestInfo *info = user_data;
    NMConnection *connection;
    GHashTable *settings = NULL;
    NMSetting *s_wireless_sec;
    const char *key_mgmt;
    GError *error = NULL;

    gtk_widget_hide (GTK_WIDGET (dialog));

	connection = nma_wireless_dialog_get_connection (dialog);

    if (response != GTK_RESPONSE_OK) {
        error = g_error_new (NM_SETTINGS_INTERFACE_ERROR,
                             NM_SETTINGS_INTERFACE_ERROR_SECRETS_REQUEST_CANCELED,
                             "%s.%d (%s): canceled",
                             __FILE__, __LINE__, __func__);

        goto done;
    }

	/* Returned secrets are a{sa{sv}}; this is the outer a{s...} hash that
	 * will contain all the individual settings hashes.
	 */
	settings = g_hash_table_new_full (g_str_hash, g_str_equal,
	                                  g_free, (GDestroyNotify) g_hash_table_destroy);

	/* If the user chose an 802.1x-based auth method, return 802.1x secrets,
	 * not wireless secrets.  Can happen with Dynamic WEP, because NM doesn't
	 * know the capabilities of the AP (since Dynamic WEP APs don't broadcast
	 * beacons), and therefore defaults to requesting WEP secrets from the
	 * wireless-security setting, not the 802.1x setting.
	 */

    s_wireless_sec = nm_connection_get_setting (connection, NM_TYPE_SETTING_WIRELESS_SECURITY);
	key_mgmt = nm_setting_wireless_security_get_key_mgmt (NM_SETTING_WIRELESS_SECURITY (s_wireless_sec));
	if (!strcmp (key_mgmt, "ieee8021x") || !strcmp (key_mgmt, "wpa-eap")) {
        const char *auth_alg;

		/* LEAP secrets aren't in the 802.1x setting */
		auth_alg = nm_setting_wireless_security_get_auth_alg (NM_SETTING_WIRELESS_SECURITY (s_wireless_sec));
		if (!auth_alg || strcmp (auth_alg, "leap")) {
			NMSetting *s_8021x;

			s_8021x = nm_connection_get_setting (connection, NM_TYPE_SETTING_802_1X);
			if (!s_8021x) {
				error = g_error_new (NM_SETTINGS_INTERFACE_ERROR,
                                     NM_SETTINGS_INTERFACE_ERROR_INVALID_CONNECTION,
                                     "%s.%d (%s): requested setting '802-1x' didn't"
                                     " exist in the connection.",
                                     __FILE__, __LINE__, __func__);
				goto done;
			}

			/* Add the 802.1x setting */
            g_hash_table_insert (settings,
                                 g_strdup (nm_setting_get_name (s_8021x)),
                                 nm_setting_to_hash (s_8021x));
		}
	}

	/* Add the 802-11-wireless-security setting no matter what */
    g_hash_table_insert (settings,
                         g_strdup (nm_setting_get_name (s_wireless_sec)),
                         nm_setting_to_hash (s_wireless_sec));

	info->callback ((NMSettingsConnectionInterface *) connection, settings, NULL, info->callback_data);

	/* Save the connection back to GConf _after_ hashing it, because
	 * saving to GConf might trigger the GConf change notifiers, resulting
	 * in the connection being read back in from GConf which clears secrets.
	 */
	if (NM_IS_GCONF_CONNECTION (connection)) {
		nm_settings_connection_interface_update (NM_SETTINGS_CONNECTION_INTERFACE (connection),
		                                         update_cb,
		                                         NULL);
	}

done:
	if (settings)
		g_hash_table_destroy (settings);

	if (error) {
		g_warning ("%s", error->message);
		info->callback (NM_SETTINGS_CONNECTION_INTERFACE (connection), NULL, error, info->callback_data);
		g_error_free (error);
	}

    g_free (info);

	if (connection)
		nm_connection_clear_secrets (connection);

    gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
secrets_requested (NMConnectionItem *item,
                   NMConnection *connection,
                   const char *setting_name,
                   const char **hints,
                   gboolean ask_user,
                   NMNewSecretsRequestedFunc callback,
                   gpointer callback_data)
{
    GtkWidget *dialog;
    SecretsRequestInfo *info;

    dialog = nma_wireless_dialog_new (nm_connection_item_get_client (item),
                                      connection,
                                      nm_device_item_get_device (NM_DEVICE_ITEM (item)),
                                      nm_wifi_item_get_ap (NM_WIFI_ITEM (item)));

    info = g_new (SecretsRequestInfo, 1);
    info->callback = callback;
    info->callback_data = callback_data;

    g_signal_connect (dialog, "done", G_CALLBACK (connection_secrets_response_cb), info);
    nma_wireless_dialog_show (NMA_WIRELESS_DIALOG (dialog));
}

/*****************************************************************************/

static void
nm_wifi_item_init (NMWifiItem *self)
{
}

static void
set_property (GObject *object, guint prop_id,
              const GValue *value, GParamSpec *pspec)
{
    NMWifiItem *self = NM_WIFI_ITEM (object);

    switch (prop_id) {
    case PROP_AP:
        /* Construct only */
        nm_wifi_item_add_ap (self, NM_ACCESS_POINT (g_value_dup_object (value)));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
get_property (GObject *object, guint prop_id,
              GValue *value, GParamSpec *pspec)
{
    NMWifiItemPrivate *priv = GET_PRIVATE (object);

    switch (prop_id) {
    case PROP_AP:
        g_value_set_object (value, priv->current_ap);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
constructed (GObject *object)
{
    NMWifiItemPrivate *priv = GET_PRIVATE (object);
    const GByteArray *ssid;
    char *str;
    guint32 flags;
	guint32 wpa_flags;
	guint32 rsn_flags;

    if (G_OBJECT_CLASS (nm_wifi_item_parent_class)->constructed)
        G_OBJECT_CLASS (nm_wifi_item_parent_class)->constructed (object);

    ssid = nm_access_point_get_ssid (priv->current_ap);
    str = nm_utils_ssid_to_utf8 ((char *) ssid->data, ssid->len);
    g_object_set (object, NM_LIST_ITEM_NAME, str, NULL);
    g_free (str);

	flags = nm_access_point_get_flags (priv->current_ap);
	wpa_flags = nm_access_point_get_wpa_flags (priv->current_ap);
	rsn_flags = nm_access_point_get_rsn_flags (priv->current_ap);

    if (rsn_flags & NM_802_11_AP_SEC_KEY_MGMT_PSK ||
        wpa_flags & NM_802_11_AP_SEC_KEY_MGMT_PSK ||
        rsn_flags & NM_802_11_AP_SEC_KEY_MGMT_802_1X ||
        wpa_flags & NM_802_11_AP_SEC_KEY_MGMT_802_1X)
        str = "WPA encrypted";
    else if (flags & NM_802_11_AP_FLAGS_PRIVACY)
        str = "WEP encrypted";
    else
        str = NULL;

    if (str)
        g_object_set (object, NM_LIST_ITEM_SECURITY, str, NULL);
}

static void
dispose (GObject *object)
{
    NMWifiItemPrivate *priv = GET_PRIVATE (object);

    if (!priv->disposed) {
        while (priv->ap_list) {
            NMAccessPoint *ap = (NMAccessPoint *) priv->ap_list->data;

            g_signal_handlers_disconnect_matched (ap,
                                                  G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA,
                                                  0, 0, NULL, update_current_ap, object);
            g_object_unref (ap);
            priv->ap_list = g_slist_delete_link (priv->ap_list, priv->ap_list);
        }

        priv->disposed = TRUE;
    }

    G_OBJECT_CLASS (nm_wifi_item_parent_class)->dispose (object);
}


static void
nm_wifi_item_class_init (NMWifiItemClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    NMListItemClass *list_class = NM_LIST_ITEM_CLASS (klass);
    NMConnectionItemClass *connection_class = NM_CONNECTION_ITEM_CLASS (klass);
    NMDeviceItemClass *device_class = NM_DEVICE_ITEM_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (NMWifiItemPrivate));

    object_class->set_property = set_property;
    object_class->get_property = get_property;
    object_class->constructed = constructed;
    object_class->dispose = dispose;

    list_class->priority = priority;
    list_class->connect = connect;

    connection_class->secrets_requested = secrets_requested;
    connection_class->create_connection = create_connection;

    device_class->get_specific_object = wifi_get_specific_object;
    device_class->get_hw_address = wifi_get_hw_address;

    /* Properties */
    g_object_class_install_property
        (object_class, PROP_AP,
         g_param_spec_object (NM_WIFI_ITEM_AP,
                              "NMAccessPoint",
                              "NMAccessPoint",
                              NM_TYPE_ACCESS_POINT,
                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}
