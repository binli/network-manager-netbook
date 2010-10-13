/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "libnm-gtk-gsm-device.h"

#define MM_DBUS_SERVICE              "org.freedesktop.ModemManager"
#define MM_DBUS_INTERFACE_MODEM      "org.freedesktop.ModemManager.Modem"
#define MM_DBUS_INTERFACE_MODEM_GSM_CARD    "org.freedesktop.ModemManager.Modem.Gsm.Card"

typedef struct {
    DBusGProxy *modem_proxy;
    DBusGProxy *card_proxy;
} NMGsmDevicePrivate;

static void
destroy_priv (gpointer data)
{
    NMGsmDevicePrivate *priv = data;

    g_object_unref (priv->modem_proxy);
    g_object_unref (priv->card_proxy);
    g_slice_free (NMGsmDevicePrivate, data);
}

static NMGsmDevicePrivate *
get_private (NMGsmDevice *device)
{
    NMGsmDevicePrivate *priv;

    priv = g_object_get_data (G_OBJECT (device), "libnm-gtk-gsm-device-private");
    if (!priv) {
        priv = g_slice_new (NMGsmDevicePrivate);

        priv->modem_proxy = dbus_g_proxy_new_for_name (nm_object_get_connection (NM_OBJECT (device)),
                                                       MM_DBUS_SERVICE,
                                                       nm_device_get_udi (NM_DEVICE (device)),
                                                       MM_DBUS_INTERFACE_MODEM);

        priv->card_proxy = dbus_g_proxy_new_for_name (nm_object_get_connection (NM_OBJECT (device)),
                                                      MM_DBUS_SERVICE,
                                                      nm_device_get_udi (NM_DEVICE (device)),
                                                      MM_DBUS_INTERFACE_MODEM_GSM_CARD);

        g_object_set_data_full (G_OBJECT (device), "libnm-gtk-gsm-device-private", priv, destroy_priv);
    }

    return priv;
}

#define GET_PRIVATE(o) get_private(o);

static void
modem_enable_cb (DBusGProxy *proxy,
                 DBusGProxyCall *call_id,
                 gpointer user_data)
{
    GSimpleAsyncResult *result = user_data;
    GError *error = NULL;

    if (!dbus_g_proxy_end_call (proxy, call_id, &error, G_TYPE_INVALID)) {
        g_simple_async_result_set_from_error (result, error);
        g_error_free (error);
    }

    g_simple_async_result_complete (result);
}

void
nm_gsm_device_enable (NMGsmDevice *device,
                      gboolean enabled,
                      GAsyncReadyCallback callback,
                      gpointer user_data)
{
    NMGsmDevicePrivate *priv;
    GSimpleAsyncResult *result;

    g_return_if_fail (NM_IS_GSM_DEVICE (device));

    result = g_simple_async_result_new (G_OBJECT (device), callback, user_data, nm_gsm_device_enable_finish);

    priv = GET_PRIVATE (device);
    dbus_g_proxy_begin_call (priv->modem_proxy,
                             "Enable", modem_enable_cb,
                             result, g_object_unref,
                             G_TYPE_BOOLEAN, enabled,
                             G_TYPE_INVALID);
}

gboolean
nm_gsm_device_enable_finish (NMGsmDevice *device,
                             GAsyncResult *result,
                             GError **error)
{
    g_return_if_fail (NM_IS_GSM_DEVICE (device));

    if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result), error) ||
        !g_simple_async_result_is_valid (result, G_OBJECT (device), nm_gsm_device_enable_finish))
        return FALSE;

    return TRUE;
}

static void
modem_send_pin_cb (DBusGProxy *proxy,
                   DBusGProxyCall *call_id,
                   gpointer user_data)
{
    GSimpleAsyncResult *result = user_data;
    GError *error = NULL;

    if (!dbus_g_proxy_end_call (proxy, call_id, &error, G_TYPE_INVALID)) {
        g_simple_async_result_set_from_error (result, error);
        g_error_free (error);
    }

    g_simple_async_result_complete (result);
}

void
nm_gsm_device_send_pin (NMGsmDevice *device,
                        const char *pin,
                        GAsyncReadyCallback callback,
                        gpointer user_data)
{
    NMGsmDevicePrivate *priv;
    GSimpleAsyncResult *result;

    g_return_if_fail (NM_IS_GSM_DEVICE (device));
    g_return_if_fail (pin != NULL);

    result = g_simple_async_result_new (G_OBJECT (device), callback, user_data, nm_gsm_device_send_pin_finish);

    priv = GET_PRIVATE (device);
    dbus_g_proxy_begin_call (priv->card_proxy,
                             "SendPin", modem_send_pin_cb,
                             result, g_object_unref,
                             G_TYPE_STRING, pin,
                             G_TYPE_INVALID);
}

gboolean
nm_gsm_device_send_pin_finish (NMGsmDevice *device,
                               GAsyncResult *result,
                               GError **error)
{
    g_return_if_fail (NM_IS_GSM_DEVICE (device));

    if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result), error) ||
        !g_simple_async_result_is_valid (result, G_OBJECT (device), nm_gsm_device_send_pin_finish))
        return FALSE;

    return TRUE;
}

static void
modem_enable_pin_cb (DBusGProxy *proxy,
                     DBusGProxyCall *call_id,
                     gpointer user_data)
{
    GSimpleAsyncResult *result = user_data;
    GError *error = NULL;

    if (!dbus_g_proxy_end_call (proxy, call_id, &error, G_TYPE_INVALID)) {
        g_simple_async_result_set_from_error (result, error);
        g_error_free (error);
    }

    g_simple_async_result_complete (result);
}

void
nm_gsm_device_enable_pin (NMGsmDevice *device,
                          const char *pin,
                          gboolean enabled,
                          GAsyncReadyCallback callback,
                          gpointer user_data)
{
    NMGsmDevicePrivate *priv;
    GSimpleAsyncResult *result;

    g_return_if_fail (NM_IS_GSM_DEVICE (device));
    g_return_if_fail (pin != NULL);

    result = g_simple_async_result_new (G_OBJECT (device), callback, user_data, nm_gsm_device_enable_pin_finish);

    priv = GET_PRIVATE (device);
    dbus_g_proxy_begin_call (priv->card_proxy,
                             "EnablePin", modem_enable_pin_cb,
                             result, g_object_unref,
                             G_TYPE_STRING, pin,
                             G_TYPE_BOOLEAN, enabled,
                             G_TYPE_INVALID);
}

gboolean
nm_gsm_device_enable_pin_finish (NMGsmDevice *device,
                                 GAsyncResult *result,
                                 GError **error)
{
    g_return_if_fail (NM_IS_GSM_DEVICE (device));

    if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result), error) ||
        !g_simple_async_result_is_valid (result, G_OBJECT (device), nm_gsm_device_enable_pin_finish))
        return FALSE;

    return TRUE;
}

static void
modem_get_imei_cb (DBusGProxy *proxy,
                   DBusGProxyCall *call_id,
                   gpointer user_data)
{
    GSimpleAsyncResult *result = user_data;
    char *str = NULL;
    GError *error = NULL;

    if (!dbus_g_proxy_end_call (proxy, call_id, &error, G_TYPE_STRING, &str, G_TYPE_INVALID)) {
        g_simple_async_result_set_from_error (result, error);
        g_error_free (error);
    } else
        g_simple_async_result_set_op_res_gpointer (result, str, g_free);

    g_simple_async_result_complete (result);
}

void
nm_gsm_device_get_imei (NMGsmDevice *device,
                        GAsyncReadyCallback callback,
                        gpointer user_data)
{
    NMGsmDevicePrivate *priv;
    GSimpleAsyncResult *result;

    g_return_if_fail (NM_IS_GSM_DEVICE (device));

    result = g_simple_async_result_new (G_OBJECT (device), callback, user_data, nm_gsm_device_get_imei_finish);

    priv = GET_PRIVATE (device);
    dbus_g_proxy_begin_call (priv->card_proxy,
                             "GetImei", modem_get_imei_cb,
                             result, g_object_unref,
                             G_TYPE_INVALID);
}

char *
nm_gsm_device_get_imei_finish (NMGsmDevice *device,
                               GAsyncResult *result,
                               GError **error)
{
    g_return_if_fail (NM_IS_GSM_DEVICE (device));

    if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result), error) ||
        !g_simple_async_result_is_valid (result, G_OBJECT (device), nm_gsm_device_get_imei_finish))
        return NULL;

    return g_strdup (g_simple_async_result_get_op_res_gpointer (G_SIMPLE_ASYNC_RESULT (result)));
}

static void
modem_get_imsi_cb (DBusGProxy *proxy,
                   DBusGProxyCall *call_id,
                   gpointer user_data)
{
    GSimpleAsyncResult *result = user_data;
    char *str = NULL;
    GError *error = NULL;

    if (!dbus_g_proxy_end_call (proxy, call_id, &error, G_TYPE_STRING, &str, G_TYPE_INVALID)) {
        g_simple_async_result_set_from_error (result, error);
        g_error_free (error);
    } else
        g_simple_async_result_set_op_res_gpointer (result, str, g_free);

    g_simple_async_result_complete (result);
}

void
nm_gsm_device_get_imsi (NMGsmDevice *device,
                        GAsyncReadyCallback callback,
                        gpointer user_data)
{
    NMGsmDevicePrivate *priv;
    GSimpleAsyncResult *result;

    g_return_if_fail (NM_IS_GSM_DEVICE (device));

    result = g_simple_async_result_new (G_OBJECT (device), callback, user_data, nm_gsm_device_get_imsi_finish);

    priv = GET_PRIVATE (device);
    dbus_g_proxy_begin_call (priv->card_proxy,
                             "GetImsi", modem_get_imsi_cb,
                             result, g_object_unref,
                             G_TYPE_INVALID);
}

char *
nm_gsm_device_get_imsi_finish (NMGsmDevice *device,
                               GAsyncResult *result,
                               GError **error)
{
    g_return_if_fail (NM_IS_GSM_DEVICE (device));

    if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result), error) ||
        !g_simple_async_result_is_valid (result, G_OBJECT (device), nm_gsm_device_get_imsi_finish))
        return NULL;

    return g_strdup (g_simple_async_result_get_op_res_gpointer (G_SIMPLE_ASYNC_RESULT (result)));
}

