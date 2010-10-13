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

#include <arpa/inet.h>
#include <nm-connection.h>
#include <nm-settings-interface.h>
#include <nm-setting-connection.h>
#include <nm-setting-serial.h>
#include <nm-setting-ppp.h>
#include <nm-setting-gsm.h>
#include <nm-setting-ip4-config.h>
#include <nm-utils.h>

#include "nm-gsm-provider.h"
#include "nm-gsm-item.h"
#include "libnm-gtk-gsm-device.h"
#include "nm-gsm-pin-request-item.h"
#include "nm-mobile-providers.h"
#include "gconf-helpers.h"
#include "utils.h"

#define MM_MODEM_ERROR "org.freedesktop.ModemManager.Modem.Gsm"
#define MM_MODEM_ERROR_SIM_PIN MM_MODEM_ERROR ".SimPinRequired"

G_DEFINE_TYPE (NMGsmProvider, nm_gsm_provider, NM_TYPE_DEVICE_PROVIDER)

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), NM_TYPE_GSM_PROVIDER, NMGsmProviderPrivate))

typedef struct {
    gboolean usable;
    gboolean pin_needed;
} NMGsmProviderPrivate;

NMItemProvider *
nm_gsm_provider_new (NMClient *client,
                     NMGsmDevice *device)
{
    g_return_val_if_fail (NM_IS_CLIENT (client), NULL);
    g_return_val_if_fail (NM_IS_GSM_DEVICE (device), NULL);

    return (NMItemProvider *) g_object_new (NM_TYPE_GSM_PROVIDER,
                                            NM_ITEM_PROVIDER_CLIENT, client,
                                            NM_DEVICE_PROVIDER_DEVICE, device,
                                            NULL);
}

static void
gsm_added (NMItemProvider *provider,
           NMSettingsConnectionInterface *connection)
{
    NMDeviceProvider *device_provider = NM_DEVICE_PROVIDER (provider);
    NMGsmProviderPrivate *priv;
    NMDevice *device;

    if (!nm_device_provider_ready (device_provider))
        return;

    priv = GET_PRIVATE (provider);
    if (!priv->usable)
        return;

    device = nm_device_provider_get_device (device_provider);
    if (utils_connection_valid_for_device (NM_CONNECTION (connection), device, NULL)) {
        NMListItem *item;

        item = nm_gsm_item_new (nm_item_provider_get_client (provider), NM_GSM_DEVICE (device), connection);
        nm_item_provider_item_added (provider, item);
    }
}

static NMConnection *
mobile_new_connection (NMMobileProvider *provider,
                       NMMobileAccessMethod *method)
{
    NMConnection *connection;
    NMSetting *type_setting;
    NMSetting *setting;
    char *name;
    char *uuid;

    connection = nm_connection_new ();

    if (method->type == NM_MOBILE_ACCESS_METHOD_TYPE_GSM) {
        type_setting = nm_setting_gsm_new ();
        g_object_set (type_setting,
                      NM_SETTING_GSM_NUMBER,   "*99#",
                      NM_SETTING_GSM_USERNAME, method->username,
                      NM_SETTING_GSM_PASSWORD, method->password,
                      NM_SETTING_GSM_APN,      method->gsm_apn,
                      NULL);

        /* FIXME: Choose the network_id more intelligently */
        if (g_slist_length (method->gsm_mcc_mnc) == 1) {
            NMGsmMccMnc *mcc_mnc = (NMGsmMccMnc *) method->gsm_mcc_mnc->data;
            char *network_id;

            network_id = g_strconcat (mcc_mnc->mcc, mcc_mnc->mnc, NULL);
            g_object_set (type_setting,
                          NM_SETTING_GSM_NETWORK_ID, network_id,
                          NULL);

            g_free (network_id);
        }
    } else
        g_assert_not_reached ();

    nm_connection_add_setting (connection, type_setting);

    if (method->gateway || method->dns) {
        GSList *iter;

        setting = nm_setting_ip4_config_new ();
        g_object_set (setting, NM_SETTING_IP4_CONFIG_METHOD, NM_SETTING_IP4_CONFIG_METHOD_AUTO, NULL);

        for (iter = method->dns; iter; iter = iter->next) {
            struct in_addr addr;

            if (inet_pton (AF_INET, (char *) iter->data, &addr) > 0)
                nm_setting_ip4_config_add_dns (NM_SETTING_IP4_CONFIG (setting), addr.s_addr);
        }

        /* I'd rather not use the gateway from providers database */
        /* if (method->gateway) */

        nm_connection_add_setting (connection, setting);
    }

    /* Serial setting */
    setting = nm_setting_serial_new ();
    g_object_set (setting,
                  NM_SETTING_SERIAL_BAUD, 115200,
                  NM_SETTING_SERIAL_BITS, 8,
                  NM_SETTING_SERIAL_PARITY, 'n',
                  NM_SETTING_SERIAL_STOPBITS, 1,
                  NULL);

    nm_connection_add_setting (connection, setting);

    setting = nm_setting_ppp_new ();
    nm_connection_add_setting (connection, setting);

    setting = nm_setting_connection_new ();
    uuid = nm_utils_uuid_generate ();
    name = g_strdup_printf ("Mobile %s", provider->name);

    g_object_set (setting,
                  NM_SETTING_CONNECTION_ID, name,
                  NM_SETTING_CONNECTION_TYPE, nm_setting_get_name (type_setting),
                  NM_SETTING_CONNECTION_AUTOCONNECT, FALSE,
                  NM_SETTING_CONNECTION_UUID, uuid,
                  NULL);
    g_free (uuid);
    g_free (name);
    nm_connection_add_setting (connection, setting);

    return connection;
}

static void
got_imsi (GObject *obj, GAsyncResult *result, gpointer user_data)
{
    NMGsmDevice *device = NM_GSM_DEVICE (obj);
    char *imsi;
    GHashTable *providers;
    NMMobileProvider *provider = NULL;
    NMMobileAccessMethod *access_method = NULL;
    GError *error = NULL;

    imsi = nm_gsm_device_get_imsi_finish (device, result, &error);
    if (error) {
        g_warning ("Reading IMSI failed: %s", error->message);
        g_clear_error (&error);
        return;
    }

    g_message ("Got IMSI: %s", imsi);
    providers = nm_mobile_providers_parse (NULL);
    if (nm_mobile_provider_lookup (providers, NM_MOBILE_ACCESS_METHOD_TYPE_GSM, imsi, &provider, &access_method)) {
        NMConnection *connection;

        g_message ("Provider %s", provider->name);
        g_message ("method %s, apn %s", access_method->name, access_method->gsm_apn);
        connection = mobile_new_connection (provider, access_method);
        if (connection) {
            g_message ("Successfully created new connection");
            /* Just add it to GConf, we'll get notified when that succeeds */
            nm_gconf_write_connection (connection, NULL, NULL);
            g_object_unref (connection);
        }
    }

    g_hash_table_destroy (providers);
    g_free (imsi);
    GET_PRIVATE (user_data)->usable = TRUE;
}

static void
modem_became_usable (NMGsmProvider *self)
{
    NMGsmDevice *device;
    GSList *list;
    GSList *iter;
    gboolean have_connections = FALSE;

    device = NM_GSM_DEVICE (nm_device_provider_get_device (NM_DEVICE_PROVIDER (self)));

    /* Do we have a configuration yet? */
    list = nm_item_provider_get_connections (NM_ITEM_PROVIDER (self));
    for (iter = list; iter; iter = iter->next) {
        NMSettingsConnectionInterface *connection = (NMSettingsConnectionInterface *) iter->data;
        NMListItem *item;

        if (!utils_connection_valid_for_device (NM_CONNECTION (connection), NM_DEVICE (device), NULL))
            continue;

        item = nm_gsm_item_new (nm_item_provider_get_client (NM_ITEM_PROVIDER (self)), device, connection);
        nm_item_provider_item_added (NM_ITEM_PROVIDER (self), item);
        have_connections = TRUE;
    }

    if (have_connections)
        GET_PRIVATE (self)->usable = TRUE;
    else
        /* No? let's try to create one automagically */
        nm_gsm_device_get_imsi (device, got_imsi, self);
}

static void
pin_unlocked (NMGsmPinRequestItem *req_item,
              gpointer user_data)
{
    NMGsmProvider *self = NM_GSM_PROVIDER (user_data);
    NMGsmProviderPrivate *priv = GET_PRIVATE (self);

    nm_list_item_request_remove (NM_LIST_ITEM (req_item));
    priv->pin_needed = FALSE;
    modem_became_usable (self);
}

static void
modem_enabled (GObject *obj, GAsyncResult *result, gpointer user_data)
{
    NMGsmDevice *device = NM_GSM_DEVICE (obj);
    NMGsmProvider *self = NM_GSM_PROVIDER (user_data);
    NMGsmProviderPrivate *priv = GET_PRIVATE (self);
    GError *error = NULL;
    gboolean success;

    success = nm_gsm_device_enable_finish (device, result, &error);
    if (error) {
        /* FIXME: The first condition only should work, but for some reason, doesn't */
        if (dbus_g_error_has_name (error, MM_MODEM_ERROR_SIM_PIN) ||
            !g_strcmp0 (error->message, "SIM PIN required")) {

            g_debug ("PIN needed");
            priv->pin_needed = TRUE;
        } else
            g_warning ("Modem enable failed: %s", error->message);

        g_clear_error (&error);
    } else
        g_debug ("Modem enabled successfully");

    if (success)
        modem_became_usable (self);
    else if (priv->pin_needed) {
        NMListItem *item;

        /* Add a PIN request item */
        item = nm_gsm_pin_request_item_new (device);
        g_signal_connect (item, "unlocked", G_CALLBACK (pin_unlocked), self);
        nm_item_provider_item_added (NM_ITEM_PROVIDER (self), item);
    }
}

static gboolean
initial_pin_check (gpointer data)
{
    NMGsmDevice *device;

    device = NM_GSM_DEVICE (nm_device_provider_get_device (NM_DEVICE_PROVIDER (data)));
    nm_gsm_device_enable (device, TRUE, modem_enabled, data);

    return FALSE;
}

/*****************************************************************************/

static void
nm_gsm_provider_init (NMGsmProvider *self)
{
}

static void
constructed (GObject *object)
{
    GSource *source;

    if (G_OBJECT_CLASS (nm_gsm_provider_parent_class)->constructed)
        G_OBJECT_CLASS (nm_gsm_provider_parent_class)->constructed (object);

    source = g_idle_source_new ();
    g_source_set_closure (source, g_cclosure_new_object (G_CALLBACK (initial_pin_check), object));
    g_source_attach (source, NULL);
    g_source_unref (source);
}

static void
nm_gsm_provider_class_init (NMGsmProviderClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    NMItemProviderClass *item_class = NM_ITEM_PROVIDER_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (NMGsmProviderPrivate));

    object_class->constructed = constructed;

    item_class->connection_added = gsm_added;
}
