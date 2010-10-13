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

#ifndef NM_MOBILE_PROVIDERS_H
#define NM_MOBILE_PROVIDERS_H

#include <glib.h>
#include <glib-object.h>

#define NM_TYPE_MOBILE_PROVIDER (nm_mobile_provider_get_type ())
#define NM_TYPE_MOBILE_ACCESS_METHOD (nm_mobile_access_method_get_type ())

typedef enum {
    NM_MOBILE_ACCESS_METHOD_TYPE_UNKNOWN = 0,
    NM_MOBILE_ACCESS_METHOD_TYPE_GSM,
    NM_MOBILE_ACCESS_METHOD_TYPE_CDMA
} NMMobileProviderType;

typedef struct {
    char *mcc;
    char *mnc;
} NMGsmMccMnc;

typedef struct {
    char *name;
    /* maps lang (char *) -> name (char *) */
    GHashTable *lcl_names;

    char *username;
    char *password;
    char *gateway;
    GSList *dns; /* GSList of 'char *' */

    /* Only used with NM_PROVIDER_TYPE_GSM */
    char *gsm_apn;
    GSList *gsm_mcc_mnc; /* GSList of NMGsmMccMnc */

    /* Only used with NM_PROVIDER_TYPE_CDMA */
    GSList *cdma_sid; /* GSList of guint32 */

    NMMobileProviderType type;

    gint refs;
} NMMobileAccessMethod;

typedef struct {
    char *name;
    /* maps lang (char *) -> name (char *) */
    GHashTable *lcl_names;

    GSList *methods; /* GSList of NMMobileAccessMethod */

    gint refs;
} NMMobileProvider;


GType nm_mobile_provider_get_type (void);
GType nm_mobile_access_method_get_type (void);

NMMobileProvider *nm_mobile_provider_ref   (NMMobileProvider *provider);
void              nm_mobile_provider_unref (NMMobileProvider *provider);

NMMobileAccessMethod *nm_mobile_access_method_ref   (NMMobileAccessMethod *method);
void                  nm_mobile_access_method_unref (NMMobileAccessMethod *method);

/* Returns a hash table where keys are country names 'char *',
   values are a 'GSList *' of 'NMMobileProvider *'.
   Everything is destroyed with g_hash_table_destroy (). */

GHashTable *nm_mobile_providers_parse (GHashTable **out_ccs);

NMMobileAccessMethod *nm_mobile_provider_access_method_lookup (NMMobileProvider *provider,
                                                               NMMobileProviderType provider_type,
                                                               const char *imsi);

gboolean nm_mobile_provider_lookup (GHashTable *providers,
                                    NMMobileProviderType provider_type,
                                    const char *imsi,
                                    NMMobileProvider **provider,
                                    NMMobileAccessMethod **access_method);

void nm_mobile_providers_dump (GHashTable *providers);

#endif /* NM_MOBILE_PROVIDERS_H */
