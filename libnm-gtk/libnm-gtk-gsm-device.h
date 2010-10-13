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
 * (C) Copyright 2010 Novell, Inc.
 */

#ifndef LIBNM_GTK_GSM_DEVICE_H
#define LIBNM_GTK_GSM_DEVICE_H

#include <gio/gio.h>
#include <nm-gsm-device.h>

void nm_gsm_device_enable (NMGsmDevice *device,
                           gboolean enabled,
                           GAsyncReadyCallback callback,
                           gpointer user_data);

gboolean nm_gsm_device_enable_finish (NMGsmDevice *device,
                                      GAsyncResult *result,
                                      GError **error);

void nm_gsm_device_send_pin (NMGsmDevice *device,
                             const char *pin,
                             GAsyncReadyCallback callback,
                             gpointer user_data);

gboolean nm_gsm_device_send_pin_finish (NMGsmDevice *device,
                                        GAsyncResult *result,
                                        GError **error);

void nm_gsm_device_enable_pin (NMGsmDevice *device,
                               const char *pin,
                               gboolean enabled,
                               GAsyncReadyCallback callback,
                               gpointer user_data);

gboolean nm_gsm_device_enable_pin_finish (NMGsmDevice *device,
                                          GAsyncResult *result,
                                          GError **error);

void nm_gsm_device_get_imei (NMGsmDevice *device,
                             GAsyncReadyCallback callback,
                             gpointer user_data);

char *nm_gsm_device_get_imei_finish (NMGsmDevice *device,
                                     GAsyncResult *result,
                                     GError **error);

void nm_gsm_device_get_imsi (NMGsmDevice *device,
                             GAsyncReadyCallback callback,
                             gpointer user_data);

char *nm_gsm_device_get_imsi_finish (NMGsmDevice *device,
                                     GAsyncResult *result,
                                     GError **error);

#endif /* LIBNM_GTK_GSM_DEVICE_H */
