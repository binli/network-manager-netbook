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
#include <nm-setting-8021x.h>
#include "nm-gsm-pin-request-item.h"
#include "libnm-gtk-gsm-device.h"

G_DEFINE_TYPE (NMGsmPinRequestItem, nm_gsm_pin_request_item, NM_TYPE_LIST_ITEM)

enum {
	UNLOCKED,

	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), NM_TYPE_GSM_PIN_REQUEST_ITEM, NMGsmPinRequestItemPrivate))

typedef struct {
    NMGsmDevice *device;
} NMGsmPinRequestItemPrivate;


NMListItem *
nm_gsm_pin_request_item_new (NMGsmDevice *device)
{
    NMListItem *item;

    g_return_val_if_fail (NM_IS_GSM_DEVICE (device), NULL);

    item = (NMListItem *) g_object_new (NM_TYPE_GSM_PIN_REQUEST_ITEM,
                                        NM_LIST_ITEM_TYPE_NAME, _("3G"),
                                        NULL);

    if (item)
        GET_PRIVATE (item)->device = g_object_ref (device);

    return item;
}

static void
pin_disabled (GObject *obj, GAsyncResult *result, gpointer user_data)
{
    NMGsmPinRequestItem *self = NM_GSM_PIN_REQUEST_ITEM (user_data);
    GError *error = NULL;

    if (nm_gsm_device_enable_pin_finish (NM_GSM_DEVICE (obj), result, &error)) {
        g_debug ("PIN disabled successfully");
    } else {
        char *msg;

        msg = g_strdup_printf (_("PIN unlocking failed: %s"), error->message);
        nm_list_item_warning (NM_LIST_ITEM (self), msg);
        g_free (msg);
        g_error_free (error);
    }
}

static void
pin_unlocked (GObject *obj, GAsyncResult *result, gpointer user_data)
{
    NMGsmPinRequestItem *self = NM_GSM_PIN_REQUEST_ITEM (user_data);
    GError *error = NULL;

    if (nm_gsm_device_send_pin_finish (NM_GSM_DEVICE (obj), result, &error)) {
        g_debug ("PIN unlocked successfully");
        g_signal_emit (self, signals[UNLOCKED], 0);
    } else {
        char *msg;

        msg = g_strdup_printf (_("PIN unlocking failed: %s"), error->message);
        nm_list_item_warning (NM_LIST_ITEM (self), msg);
        g_free (msg);
        g_error_free (error);
    }
}

void
nm_gsm_pin_request_item_unlock (NMGsmPinRequestItem *self,
                                const char *pin,
                                gboolean disable_pin)
{
    NMGsmPinRequestItemPrivate *priv = GET_PRIVATE (self);

    g_return_if_fail (NM_IS_GSM_PIN_REQUEST_ITEM (self));
    g_return_if_fail (pin != NULL);

    if (disable_pin)
        nm_gsm_device_enable_pin (priv->device, pin, FALSE, pin_disabled, self);
    else
        nm_gsm_device_send_pin (priv->device, pin, pin_unlocked, self);
}

gboolean
nm_gsm_pin_request_item_unlock_finish (NMGsmPinRequestItem *self,
                                       GAsyncResult *result,
                                       GError **error)
{
    g_return_val_if_fail (NM_IS_GSM_PIN_REQUEST_ITEM (self), FALSE);

    if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result), error) ||
        !g_simple_async_result_is_valid (result, G_OBJECT (self), nm_gsm_pin_request_item_unlock_finish))
        return FALSE;

    

    return TRUE;
}

static int
priority (NMListItem *item)
{
    return NM_LIST_ITEM_PRIORITY_DEV_GSM + NM_LIST_ITEM_CLASS (nm_gsm_pin_request_item_parent_class)->priority (item);
}

/*****************************************************************************/

static void
nm_gsm_pin_request_item_init (NMGsmPinRequestItem *self)
{
    g_object_set (self,
                  NM_LIST_ITEM_NAME, _("Locked GSM device"),
                  NM_LIST_ITEM_ICON, "nm-device-wwan",
                  NM_LIST_ITEM_SHOW_DELETE, FALSE,
                  NULL);
}

static void
dispose (GObject *object)
{
    NMGsmPinRequestItemPrivate *priv = GET_PRIVATE (object);

    if (priv->device) {
        g_object_unref (priv->device);
        priv->device = NULL;
    }

    G_OBJECT_CLASS (nm_gsm_pin_request_item_parent_class)->dispose (object);
}

static void
nm_gsm_pin_request_item_class_init (NMGsmPinRequestItemClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    NMListItemClass *list_class = NM_LIST_ITEM_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (NMGsmPinRequestItemPrivate));

    object_class->dispose = dispose;
    list_class->priority = priority;

    /* Signals */
    signals[UNLOCKED] =
		g_signal_new ("unlocked",
					  G_OBJECT_CLASS_TYPE (object_class),
					  G_SIGNAL_RUN_FIRST,
					  G_STRUCT_OFFSET (NMGsmPinRequestItemClass, unlocked),
					  NULL, NULL,
					  g_cclosure_marshal_VOID__VOID,
					  G_TYPE_NONE, 0);
}
