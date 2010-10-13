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

#include <nm-device-ethernet.h>
#include <nm-device-wifi.h>
#include <nm-gsm-device.h>
#include <nm-cdma-device.h>
#include <nm-device-bt.h>

#include "nm-device-handler.h"
#include "nm-ethernet-provider.h"
#include "nm-wifi-provider.h"
#include "nm-gsm-provider.h"
#include "nm-cdma-provider.h"
#include "nm-bt-provider.h"

G_DEFINE_TYPE (NMDeviceHandler, nm_device_handler, G_TYPE_OBJECT)

enum {
    PROP_0,
    PROP_CLIENT,

    LAST_PROP
};

enum {
	PROVIDER_ADDED,
	PROVIDER_REMOVED,

	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), NM_TYPE_DEVICE_HANDLER, NMDeviceHandlerPrivate))

typedef struct {
    NMClient *client;
    GSList *providers;

    gboolean disposed;
} NMDeviceHandlerPrivate;


NMDeviceHandler *
nm_device_handler_new (NMClient *client)
{
    g_return_val_if_fail (NM_IS_CLIENT (client), NULL);

    return (NMDeviceHandler *) g_object_new (NM_TYPE_DEVICE_HANDLER,
                                             NM_DEVICE_HANDLER_CLIENT, client,
                                             NULL);
}

GSList *
nm_device_handler_get_providers (NMDeviceHandler *self)
{
    g_return_val_if_fail (NM_IS_DEVICE_HANDLER (self), NULL);

    return GET_PRIVATE (self)->providers;
}

static NMItemProvider *
create_provider (NMDeviceHandler *self, NMDevice *device)
{
    NMDeviceHandlerPrivate *priv = GET_PRIVATE (self);
    NMItemProvider *provider;
    GType type;

    type = G_OBJECT_TYPE (device);

    if (type == NM_TYPE_DEVICE_ETHERNET)
        provider = nm_ethernet_provider_new (priv->client, NM_DEVICE_ETHERNET (device));
    else if (type == NM_TYPE_DEVICE_WIFI)
        provider = nm_wifi_provider_new (priv->client, NM_DEVICE_WIFI (device));
    else if (type == NM_TYPE_GSM_DEVICE)
        provider = nm_gsm_provider_new (priv->client, NM_GSM_DEVICE (device));
    else if (type == NM_TYPE_CDMA_DEVICE)
        provider = nm_cdma_provider_new (priv->client, NM_CDMA_DEVICE (device));
    else if (type == NM_TYPE_DEVICE_BT)
        provider = nm_bt_provider_new (priv->client, NM_DEVICE_BT (device));
    else {
        g_warning ("Unknown device type %s", G_OBJECT_TYPE_NAME (device));
        provider = NULL;
    }

    return provider;
}

static void
device_added (NMClient *client,
              NMDevice *device,
              gpointer user_data)
{
    NMDeviceHandler *self = NM_DEVICE_HANDLER (user_data);
    NMDeviceHandlerPrivate *priv = GET_PRIVATE (self);
    NMItemProvider *provider;

    provider = create_provider (self, device);
    if (provider) {
        priv->providers = g_slist_prepend (priv->providers, provider);
        g_signal_emit (self, signals[PROVIDER_ADDED], 0, provider);
    }
}

static void
device_removed (NMClient *client,
                NMDevice *device,
                gpointer user_data)
{
    NMDeviceHandler *self = NM_DEVICE_HANDLER (user_data);
    NMDeviceHandlerPrivate *priv = GET_PRIVATE (self);
    GSList *iter;

    for (iter = priv->providers; iter; iter = iter->next) {
        NMDeviceProvider *provider = (NMDeviceProvider *) iter->data;

        if (nm_device_provider_get_device (provider) == device) {
            priv->providers = g_slist_delete_link (priv->providers, iter);
            g_signal_emit (self, signals[PROVIDER_REMOVED], 0, provider);
            g_object_unref (provider);
            break;
        }
    }
}

/*****************************************************************************/

static void
nm_device_handler_init (NMDeviceHandler *self)
{
}

static void
set_property (GObject *object, guint prop_id,
              const GValue *value, GParamSpec *pspec)
{
    NMDeviceHandlerPrivate *priv = GET_PRIVATE (object);

    switch (prop_id) {
    case PROP_CLIENT:
        /* Construct only */
        priv->client = g_value_dup_object (value);
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
    NMDeviceHandlerPrivate *priv = GET_PRIVATE (object);

    switch (prop_id) {
    case PROP_CLIENT:
        g_value_set_object (value, priv->client);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
constructed (GObject *object)
{
    NMDeviceHandlerPrivate *priv = GET_PRIVATE (object);
    const GPtrArray *devices;
    int i;

    if (G_OBJECT_CLASS (nm_device_handler_parent_class)->constructed)
        G_OBJECT_CLASS (nm_device_handler_parent_class)->constructed (object);

    g_signal_connect (priv->client, "device-added",   G_CALLBACK (device_added), object);
    g_signal_connect (priv->client, "device-removed", G_CALLBACK (device_removed), object);

    devices = nm_client_get_devices (priv->client);
    for (i = 0; devices && devices->len > i; i++)
        device_added (priv->client, NM_DEVICE (g_ptr_array_index (devices, i)), object);
}

static void
dispose (GObject *object)
{
    NMDeviceHandlerPrivate *priv = GET_PRIVATE (object);
    const GPtrArray *devices;
    int i;

    if (!priv->disposed) {
        g_signal_handlers_disconnect_matched (priv->client, G_SIGNAL_MATCH_DATA,
                                              0, 0, NULL, NULL, object);

        devices = nm_client_get_devices (priv->client);
        for (i = 0; devices && devices->len > i; i++)
            device_removed (priv->client, NM_DEVICE (g_ptr_array_index (devices, i)), object);

        priv->disposed = TRUE;
    }

    G_OBJECT_CLASS (nm_device_handler_parent_class)->dispose (object);
}

static void
nm_device_handler_class_init (NMDeviceHandlerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (NMDeviceHandlerPrivate));

    object_class->set_property = set_property;
    object_class->get_property = get_property;
    object_class->constructed = constructed;
    object_class->dispose = dispose;

    /* Properties */
    g_object_class_install_property
        (object_class, PROP_CLIENT,
         g_param_spec_object (NM_DEVICE_HANDLER_CLIENT,
                              "NMClient",
                              "NMClient",
                              NM_TYPE_CLIENT,
                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    /* Signals */
    signals[PROVIDER_ADDED] =
		g_signal_new ("provider-added",
					  G_OBJECT_CLASS_TYPE (object_class),
					  G_SIGNAL_RUN_FIRST,
					  G_STRUCT_OFFSET (NMDeviceHandlerClass, provider_added),
					  NULL, NULL,
					  g_cclosure_marshal_VOID__OBJECT,
					  G_TYPE_NONE, 1,
					  NM_TYPE_ITEM_PROVIDER);

    signals[PROVIDER_REMOVED] =
		g_signal_new ("provider-removed",
					  G_OBJECT_CLASS_TYPE (object_class),
					  G_SIGNAL_RUN_FIRST,
					  G_STRUCT_OFFSET (NMDeviceHandlerClass, provider_removed),
					  NULL, NULL,
					  g_cclosure_marshal_VOID__OBJECT,
					  G_TYPE_NONE, 1,
					  NM_TYPE_ITEM_PROVIDER);
}
