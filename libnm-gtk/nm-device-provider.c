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

#include "nm-device-provider.h"

G_DEFINE_ABSTRACT_TYPE (NMDeviceProvider, nm_device_provider, NM_TYPE_ITEM_PROVIDER)

enum {
    PROP_0,
    PROP_DEVICE,

    LAST_PROP
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), NM_TYPE_DEVICE_PROVIDER, NMDeviceProviderPrivate))

typedef struct {
    NMDevice *device;
    gulong state_changed_id;

    gboolean disposed;
} NMDeviceProviderPrivate;

NMDevice *
nm_device_provider_get_device (NMDeviceProvider *self)
{
    g_return_val_if_fail (NM_IS_DEVICE_PROVIDER (self), NULL);

    return GET_PRIVATE (self)->device;
}

gboolean
nm_device_provider_ready (NMDeviceProvider *self)
{
    g_return_val_if_fail (NM_IS_DEVICE_PROVIDER (self), FALSE);

    return nm_device_get_state (nm_device_provider_get_device (self)) >= NM_DEVICE_STATE_DISCONNECTED;
}

static void
device_state_changed (NMDevice *device,
                      NMDeviceState new_state,
                      NMDeviceState old_state,
                      NMDeviceStateReason reason,
                      gpointer user_data)
{
    NMDeviceProvider *self = NM_DEVICE_PROVIDER (user_data);

    if (old_state < NM_DEVICE_STATE_DISCONNECTED && new_state >= NM_DEVICE_STATE_DISCONNECTED)
        /* Device became usable */
        nm_item_provider_reset (NM_ITEM_PROVIDER (self));
    else if (old_state >= NM_DEVICE_STATE_DISCONNECTED && new_state < NM_DEVICE_STATE_DISCONNECTED)
        /* Device became unusable */
        nm_item_provider_clear (NM_ITEM_PROVIDER (self));

    if (NM_DEVICE_PROVIDER_GET_CLASS (self)->state_changed)
        NM_DEVICE_PROVIDER_GET_CLASS (self)->state_changed (self, new_state, old_state, reason);
}

/*****************************************************************************/

static void
nm_device_provider_init (NMDeviceProvider *self)
{
}

static void
set_property (GObject *object, guint prop_id,
              const GValue *value, GParamSpec *pspec)
{
    NMDeviceProviderPrivate *priv = GET_PRIVATE (object);

    switch (prop_id) {
    case PROP_DEVICE:
        /* Construct only */
        priv->device = g_value_dup_object (value);
        priv->state_changed_id = g_signal_connect (priv->device, "state-changed",
                                                   G_CALLBACK (device_state_changed), object);
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
    NMDeviceProviderPrivate *priv = GET_PRIVATE (object);

    switch (prop_id) {
    case PROP_DEVICE:
        g_value_set_object (value, priv->device);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
dispose (GObject *object)
{
    NMDeviceProviderPrivate *priv = GET_PRIVATE (object);

    if (!priv->disposed) {
        g_signal_handler_disconnect (priv->device, priv->state_changed_id);
        g_object_unref (priv->device);

        priv->disposed = TRUE;
    }

    G_OBJECT_CLASS (nm_device_provider_parent_class)->dispose (object);
}

static void
nm_device_provider_class_init (NMDeviceProviderClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (NMDeviceProviderPrivate));

    object_class->set_property = set_property;
    object_class->get_property = get_property;
    object_class->dispose = dispose;

    /* Properties */
    g_object_class_install_property
        (object_class, PROP_DEVICE,
         g_param_spec_object (NM_DEVICE_PROVIDER_DEVICE,
                              "NMDevice",
                              "NMDevice",
                              NM_TYPE_DEVICE,
                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}
