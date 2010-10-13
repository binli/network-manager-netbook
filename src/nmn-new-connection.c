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
#include <glib/gi18n.h>
#include <NetworkManager.h>
#include <nm-gsm-device.h>
#include <nm-cdma-device.h>
#include <nm-connection.h>
#include <nm-settings-interface.h>
#include <nm-setting-connection.h>
#include <nm-setting-serial.h>
#include <nm-setting-ppp.h>
#include <nm-setting-gsm.h>
#include <nm-setting-cdma.h>
#include <nm-setting-ip4-config.h>
#include <nm-utils.h>
#include <nm-mobile-providers.h>
#include "nmn-new-connection.h"
#include "nm-list-model.h"
#include "nm-wifi-item.h"
#include "nm-gsm-pin-request-item.h"
#include "nm-gconf-settings.h"
#include "nmn-list.h"
#include "wireless-dialog.h"

G_DEFINE_TYPE (NmnNewConnection, nmn_new_connection, GTK_TYPE_VBOX)

enum {
    PROP_0,
    PROP_MODEL,

    LAST_PROP
};

enum {
    CLOSE,

    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), NMN_TYPE_NEW_CONNECTION, NmnNewConnectionPrivate))

typedef struct {
    NmnModel *model;

    /* WiFi widgets */
    GtkWidget *wifi_list_container;
    GtkWidget *wifi_list;

    /* 3G widgets */
    GtkWidget *mobile_label;
    GtkWidget *mobile_list;
    GtkWidget *mobile_save;
    gboolean mobile_list_populated;
    gulong mobile_toggled_id;

    gboolean disposed;
} NmnNewConnectionPrivate;

GtkWidget *
nmn_new_connection_create (NmnModel *model)
{
    g_return_val_if_fail (NMN_IS_MODEL (model), NULL);

    return GTK_WIDGET (g_object_new (NMN_TYPE_NEW_CONNECTION,
                                     NMN_NEW_CONNECTION_MODEL, model,
                                     NULL));
}

static void
connect_requested (gpointer instance, gpointer user_data)
{
    gtk_widget_hide (GTK_WIDGET (user_data));
}

static void
connect_cb (gpointer user_data,
            const char *object_path,
            GError *error)
{
    /* FIXME: Report the error somewhere */
    g_warning ("connect_cb: %s %s", object_path,
               error ? error->message : "success");
}

static void
save_timestamp_cb (NMSettingsConnectionInterface *connection,
		   GError *error,
		   gpointer user_data)
{
	if (error) {
		g_warning ("Error saving connection %s timestamp: (%d) %s",
			   nm_connection_get_path (NM_CONNECTION (connection)),
			   error->code,
			   error->message);
	}
}

static gboolean
save_connection (NmnModel *model, NMConnection *connection)
{
    NMSettingConnection *s_con;
    NMSettingsConnectionInterface *gconf_connection;
    NMGConfSettings *settings;

    settings = NM_GCONF_SETTINGS (nmn_model_get_user_settings (model));

    gconf_connection = nm_settings_interface_get_connection_by_path (NM_SETTINGS_INTERFACE (settings),
                                                                     nm_connection_get_path (connection));
    if (!gconf_connection)
        return FALSE;
    s_con = NM_SETTING_CONNECTION (nm_connection_get_setting (connection, NM_TYPE_SETTING_CONNECTION));
    g_object_set (s_con, NM_SETTING_CONNECTION_TIMESTAMP, (guint64) time (NULL), NULL);
    nm_settings_connection_interface_update (gconf_connection, save_timestamp_cb, NULL);

    return TRUE;
}

static void
wifi_dialog_done (NMAWirelessDialog *dialog,
                  gint response,
                  gpointer user_data)
{
    NmnNewConnectionPrivate *priv = GET_PRIVATE (user_data);
    NMConnection *connection;

    if (response != GTK_RESPONSE_OK)
        goto done;

    connection = nma_wireless_dialog_get_connection (dialog);
    save_connection (priv->model, connection);

    nm_client_activate_connection (nmn_model_get_client (priv->model),
                                   NM_DBUS_SERVICE_USER_SETTINGS,
                                   nm_connection_get_path (connection),
                                   nma_wireless_dialog_get_device (dialog),
                                   "/",
                                   connect_cb,
                                   NULL);

 done:
    gtk_widget_hide (GTK_WIDGET (dialog));
    gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
wifi_search (GtkButton *button, gpointer user_data)
{
    NmnNewConnectionPrivate *priv = GET_PRIVATE (user_data);
    GtkWidget *dialog;

    dialog = nma_wireless_dialog_hidden_new (nmn_model_get_client (priv->model));
    g_signal_connect (dialog, "done", G_CALLBACK (wifi_dialog_done), user_data);
    nma_wireless_dialog_show (NMA_WIRELESS_DIALOG (dialog));
}

static gboolean
wifi_model_visible_func (GtkTreeModel *model,
                         GtkTreeIter  *iter,
                         gpointer      data)
{
    NMListItem *item = NULL;
    gboolean visible = FALSE;

    gtk_tree_model_get (model, iter, NM_LIST_MODEL_COL_ITEM, &item, -1);
    if (item) {
        /* Show unconfigured WiFi items */
        if (NM_IS_WIFI_ITEM (item) && !nm_connection_item_get_connection (NM_CONNECTION_ITEM (item)))
            visible = TRUE;

        g_object_unref (item);
    }

    return visible;
}

static void
wifi_page_init (NmnNewConnection *connection)
{
    NmnNewConnectionPrivate *priv = GET_PRIVATE (connection);
    GtkTreeModel *wifi_model;

    wifi_model = gtk_tree_model_filter_new (GTK_TREE_MODEL (priv->model), NULL);
    gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (wifi_model),
                                            wifi_model_visible_func, NULL, NULL);

    priv->wifi_list = nmn_list_new_with_model (wifi_model);
    g_object_unref (wifi_model);
    //g_signal_connect (priv->wifi_list, "connect-requested", G_CALLBACK (connect_requested), connection);
    gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (priv->wifi_list_container), priv->wifi_list);
    gtk_widget_show_all (priv->wifi_list);
}

/* Mobile */

#define MOBILE_COL_NAME     0
#define MOBILE_COL_PROVIDER 1
#define MOBILE_COL_METHOD   2

static gboolean
mobile_have_device (NmnNewConnection *connection, NMMobileProviderType *type)
{
    NmnNewConnectionPrivate *priv = GET_PRIVATE (connection);
    NMClient *client;
    const GPtrArray *devices;
    int i;
    gboolean found = FALSE;

    client = nmn_model_get_client (priv->model);
    devices = nm_client_get_devices (client);
    for (i = 0; !found && devices && i < devices->len; i++) {
        NMDevice *device = NM_DEVICE (g_ptr_array_index (devices, i));

        if (NM_IS_GSM_DEVICE (device)) {
            found = TRUE;
            *type = NM_MOBILE_ACCESS_METHOD_TYPE_GSM;
        }

        if (NM_IS_CDMA_DEVICE (device)) {
            found = TRUE;
            *type = NM_MOBILE_ACCESS_METHOD_TYPE_CDMA;
        }
    }

    return found;
}

static gboolean
gsm_is_locked (NmnModel *nmn_model)
{
    GtkTreeModel *model = GTK_TREE_MODEL (nmn_model);
    GtkTreeIter iter;
    gboolean valid;
    gboolean found = FALSE;

    valid = gtk_tree_model_get_iter_first (model, &iter);
    while (valid && !found) {
        NMListItem *item = NULL;

        gtk_tree_model_get (model, &iter, NM_LIST_MODEL_COL_ITEM, &item, -1);
        if (item) {
            if (NM_IS_GSM_PIN_REQUEST_ITEM (item))
                found = TRUE;

            g_object_unref (item);
        }

        valid = gtk_tree_model_iter_next (model, &iter);
    }

    return found;
}

static void
mobile_status_update (NmnNewConnection *connection)
{
    NmnNewConnectionPrivate *priv = GET_PRIVATE (connection);
    NMMobileProviderType type;
    gboolean active;

    if (!nmn_model_modems_get_active (priv->model)) {
        active = FALSE;
        gtk_label_set_text (GTK_LABEL (priv->mobile_label), _("3G disabled"));
    } else {
        active = mobile_have_device (connection, &type);

        if (active) {
            if (type == NM_MOBILE_ACCESS_METHOD_TYPE_GSM && gsm_is_locked (priv->model)) {
                gtk_label_set_text (GTK_LABEL (priv->mobile_label), _("3G modem is locked"));
                active = FALSE;
            } else
                gtk_label_set_text (GTK_LABEL (priv->mobile_label), _("Internal 3G modem and SIM card detected"));
        } else
            gtk_label_set_text (GTK_LABEL (priv->mobile_label), _("No modems detected"));
    }

    gtk_widget_set_sensitive (priv->mobile_list, active);
    gtk_widget_set_sensitive (priv->mobile_save, active);
}

static gboolean
method_list_has_type (GSList *list, NMMobileProviderType type)
{
    GSList *iter;

    for (iter = list; iter; iter = iter->next) {
        NMMobileAccessMethod *method = iter->data;

        if (method->type == type)
            return TRUE;
    }

    return FALSE;
}

static gboolean
provider_list_has_type (GSList *list, NMMobileProviderType type)
{
    GSList *iter;

    for (iter = list; iter; iter = iter->next) {
        NMMobileProvider *provider = (NMMobileProvider *) iter->data;

        if (method_list_has_type (provider->methods, type))
            return TRUE;
    }

    return FALSE;
}

typedef struct {
    GtkTreeStore *store;
    NMMobileProviderType provider_type;

    GtkTreeIter *parent_iter;
} MobilePopulateInfo;

static void
add_one_method (gpointer data, gpointer user_data)
{
    NMMobileAccessMethod *method = (NMMobileAccessMethod *) data;
    MobilePopulateInfo *info = (MobilePopulateInfo *) user_data;
    GtkTreeIter iter;

    if (method->type != info->provider_type)
        return;

    gtk_tree_store_append (info->store, &iter, info->parent_iter);

    if (method->type == NM_MOBILE_ACCESS_METHOD_TYPE_GSM) {
        char *txt;

        txt = g_strdup_printf ("%s (APN %s)", method->name, method->gsm_apn);
        gtk_tree_store_set (info->store, &iter,
                            MOBILE_COL_NAME, txt,
                            MOBILE_COL_METHOD, method,
                            -1);

        g_free (txt);
    } else {
        gtk_tree_store_set (info->store, &iter,
                            MOBILE_COL_NAME, method->name,
                            MOBILE_COL_METHOD, method,
                            -1);
    }
}

static void
add_one_provider (gpointer data, gpointer user_data)
{
    NMMobileProvider *provider = (NMMobileProvider *) data;
    MobilePopulateInfo *info = (MobilePopulateInfo *) user_data;
    GtkTreeIter *country_iter;
    GtkTreeIter iter;

    if (!method_list_has_type (provider->methods, info->provider_type))
        return;

    gtk_tree_store_append (info->store, &iter, info->parent_iter);
    gtk_tree_store_set (info->store, &iter,
                        MOBILE_COL_NAME, provider->name,
                        MOBILE_COL_PROVIDER, provider,
                        -1);

    country_iter = info->parent_iter;
    info->parent_iter = &iter;
    g_slist_foreach (provider->methods, add_one_method, info);
    info->parent_iter = country_iter;
}

static void
add_one_country (gpointer key, gpointer value, gpointer user_data)
{
    GSList *providers = (GSList *) value;
    MobilePopulateInfo *info = (MobilePopulateInfo *) user_data;
    GtkTreeIter iter;

    if (!provider_list_has_type (providers, info->provider_type))
        return;

    gtk_tree_store_append (info->store, &iter, NULL);
    gtk_tree_store_set (info->store, &iter,
                        MOBILE_COL_NAME, key,
                        -1);

    info->parent_iter = &iter;
    g_slist_foreach (providers, add_one_provider, info);
}

static void
mobile_list_populate (NmnNewConnection *connection)
{
    NmnNewConnectionPrivate *priv = GET_PRIVATE (connection);
    GHashTable *mobile_providers;
    GtkTreeStore *store;
    GtkTreeModel *sort_model;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    NMMobileProviderType provider_type;
    MobilePopulateInfo info;

    if (priv->mobile_list_populated || !mobile_have_device (connection, &provider_type))
        return;

    mobile_providers = nm_mobile_providers_parse (NULL);
    if (!mobile_providers)
        return;

    store = gtk_tree_store_new (3, G_TYPE_STRING, NM_TYPE_MOBILE_PROVIDER, NM_TYPE_MOBILE_ACCESS_METHOD);

    info.store = store;
    info.provider_type = provider_type;
    g_hash_table_foreach (mobile_providers, add_one_country, &info);
    g_hash_table_destroy (mobile_providers);

    sort_model = gtk_tree_model_sort_new_with_model (GTK_TREE_MODEL (store));
    gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (sort_model),
                                          MOBILE_COL_NAME, GTK_SORT_ASCENDING);

    gtk_tree_view_set_model (GTK_TREE_VIEW (priv->mobile_list), sort_model);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Country",
                                                       renderer,
                                                       "text", MOBILE_COL_NAME,
                                                       NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (priv->mobile_list), column);
    gtk_tree_view_column_set_clickable (column, TRUE);

    priv->mobile_list_populated = TRUE;
}

static void
mobile_tree_selection_changed (GtkTreeSelection *selection,
                               gpointer user_data)
{
    NmnNewConnection *connection = (NmnNewConnection *) user_data;
    NmnNewConnectionPrivate *priv = GET_PRIVATE (connection);
    GtkTreeModel *model;
    NMMobileAccessMethod *method = NULL;
    GtkTreeIter iter;

    if (gtk_tree_selection_get_selected (selection, &model, &iter))
        gtk_tree_model_get (model, &iter, MOBILE_COL_METHOD, &method, -1);

    gtk_widget_set_sensitive (priv->mobile_save, method != NULL);
    if (method)
        nm_mobile_access_method_unref (method);
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
    } else if (method->type == NM_MOBILE_ACCESS_METHOD_TYPE_CDMA) {
        type_setting = nm_setting_cdma_new ();
        g_object_set (type_setting, NM_SETTING_CDMA_NUMBER, "#777", NULL);
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
mobile_save_clicked (GtkButton *button, gpointer user_data)
{
    NmnNewConnection *self = (NmnNewConnection *) user_data;
    NmnNewConnectionPrivate *priv = GET_PRIVATE (self);
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    NMMobileProvider *provider = NULL;
    NMMobileAccessMethod *method = NULL;
    GtkTreeIter method_iter;
    GtkTreeIter provider_iter;

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->mobile_list));
    if (!gtk_tree_selection_get_selected (selection, &model, &method_iter))
        return;

    if (!gtk_tree_model_iter_parent (model, &provider_iter, &method_iter))
        return;

    gtk_tree_model_get (model, &method_iter, MOBILE_COL_METHOD, &method, -1);
    gtk_tree_model_get (model, &provider_iter, MOBILE_COL_PROVIDER, &provider, -1);

    if (provider && method) {
        NMConnection *connection;

        connection = NULL;
        connection = mobile_new_connection (provider, method);

        if (connection) {
            nm_gconf_settings_add_connection (NM_GCONF_SETTINGS (nmn_model_get_user_settings (priv->model)),
                                              connection);
            g_object_unref (connection);
        }

        gtk_widget_hide (GTK_WIDGET (self));
    }

    if (provider)
        nm_mobile_provider_unref (provider);

    if (method)
        nm_mobile_access_method_unref (method);
}

static void
mobile_device_added (NMClient *client,
                     NMDevice *device,
                     gpointer user_data)
{
    if (NM_IS_GSM_DEVICE (device) || NM_IS_CDMA_DEVICE (device)) {
        NmnNewConnection *connection = NMN_NEW_CONNECTION (user_data);

        mobile_list_populate (connection);
    }
}

static void
mobile_page_init (NmnNewConnection *connection)
{
    NmnNewConnectionPrivate *priv = GET_PRIVATE (connection);
    NMClient *client = nmn_model_get_client (priv->model);

    g_signal_connect (client, "device-added", G_CALLBACK (mobile_device_added), connection);
    g_signal_connect_swapped (client, "device-removed", G_CALLBACK (mobile_status_update), connection);
    
    priv->mobile_toggled_id = g_signal_connect_swapped (priv->model, "modems-toggled",
                                                        G_CALLBACK (mobile_status_update),
                                                        connection);
}

void
nmn_new_connection_update (NmnNewConnection *self)
{
    g_return_if_fail (NMN_IS_NEW_CONNECTION (self));

    mobile_list_populate (self);
    mobile_status_update (self);
}

static void
close (NmnNewConnection *self)
{
    g_signal_emit (self, signals[CLOSE], 0);
}

static void
nmn_new_connection_init (NmnNewConnection *self)
{
    NmnNewConnectionPrivate *priv = GET_PRIVATE (self);
    GtkWidget *w;
    GtkWidget *notebook;
    GtkWidget *box;
    GtkWidget *button;
    GtkTreeSelection *selection;
    char *label;

    gtk_box_set_spacing (GTK_BOX (self), 6);
    gtk_container_set_border_width (GTK_CONTAINER (self), 6);

    label = g_strdup_printf ("<big><b>%s</b></big>", _("Add a new connection"));
    w = gtk_label_new (NULL);
    gtk_label_set_markup (GTK_LABEL (w), label);
    g_free (label);
    gtk_misc_set_alignment (GTK_MISC (w), 0.0, 0.5);
    gtk_box_pack_start (GTK_BOX (self), w, FALSE, FALSE, 0);

    notebook = gtk_notebook_new ();
    gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_LEFT);
    gtk_box_pack_start (GTK_BOX (self), notebook, TRUE, TRUE, 0);

    /* WiFi page */
    box = gtk_vbox_new (FALSE, 6);
    priv->wifi_list_container = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (priv->wifi_list_container),
                                    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start (GTK_BOX (box), priv->wifi_list_container, TRUE, TRUE, 0);

    w = gtk_alignment_new (0.0, 0.5, 0.0, 1.0);
    gtk_box_pack_start (GTK_BOX (box), w, FALSE, FALSE, 0);
    button = gtk_button_new_with_label (_("Hidden network"));
    gtk_container_add (GTK_CONTAINER (w), button);
    g_signal_connect (button, "clicked", G_CALLBACK (wifi_search), self);
    g_signal_connect (button, "clicked", G_CALLBACK (connect_requested), self);

    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), box, gtk_label_new (_("WiFi")));

    /* 3G page */
    box = gtk_vbox_new (FALSE, 6);
    priv->mobile_label = gtk_label_new (NULL);
    gtk_misc_set_alignment (GTK_MISC (priv->mobile_label), 0.0, 0.5);
    gtk_box_pack_start (GTK_BOX (box), priv->mobile_label, FALSE, TRUE, 0);

    w = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (w), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start (GTK_BOX (box), w, TRUE, TRUE, 0);

    priv->mobile_list = gtk_tree_view_new ();
    gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (priv->mobile_list), FALSE);
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->mobile_list));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
    g_signal_connect (selection, "changed", G_CALLBACK (mobile_tree_selection_changed), self);
    gtk_container_add (GTK_CONTAINER (w), priv->mobile_list);

    w = gtk_alignment_new (0.0, 0.5, 0.0, 1.0);
    gtk_box_pack_start (GTK_BOX (box), w, FALSE, FALSE, 0);
    priv->mobile_save = gtk_button_new_with_label (_("Save connection"));
    gtk_container_add (GTK_CONTAINER (w), priv->mobile_save);
    g_signal_connect (priv->mobile_save, "clicked", G_CALLBACK (mobile_save_clicked), self);

    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), box, gtk_label_new (_("3G")));

    /* Close button */
    w = gtk_alignment_new (0.0, 0.5, 0.0, 1.0);
    gtk_box_pack_start (GTK_BOX (self), w, FALSE, FALSE, 0);
    button = gtk_button_new_with_label (_("Return to Networking"));
    gtk_container_add (GTK_CONTAINER (w), button);
    g_signal_connect_swapped (button, "clicked", G_CALLBACK (close), self);

    gtk_widget_show_all (GTK_WIDGET (self));
}

static GObject*
constructor (GType type,
             guint n_construct_params,
             GObjectConstructParam *construct_params)
{
    GObject *object;
    NmnNewConnectionPrivate *priv;

    object = G_OBJECT_CLASS (nmn_new_connection_parent_class)->constructor
        (type, n_construct_params, construct_params);

    if (!object)
        return NULL;

    priv = GET_PRIVATE (object);

    if (!priv->model) {
        g_warning ("Missing constructor arguments");
        g_object_unref (object);
        return NULL;
    }

    wifi_page_init (NMN_NEW_CONNECTION (object));
    mobile_page_init (NMN_NEW_CONNECTION (object));

    return object;
}

static void
set_property (GObject *object, guint prop_id,
              const GValue *value, GParamSpec *pspec)
{
    NmnNewConnectionPrivate *priv = GET_PRIVATE (object);

    switch (prop_id) {
    case PROP_MODEL:
        /* Construct only */
        priv->model = g_value_dup_object (value);
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
    NmnNewConnectionPrivate *priv = GET_PRIVATE (object);

    switch (prop_id) {
    case PROP_MODEL:
        g_value_set_object (value, priv->model);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
dispose (GObject *object)
{
    NmnNewConnectionPrivate *priv = GET_PRIVATE (object);

    if (priv->disposed)
        return;

    priv->disposed = TRUE;

    g_signal_handler_disconnect (priv->model, priv->mobile_toggled_id);

    if (priv->model)
        g_object_unref (priv->model);

    G_OBJECT_CLASS (nmn_new_connection_parent_class)->dispose (object);
}

static void
nmn_new_connection_class_init (NmnNewConnectionClass *class)
{
    GObjectClass *object_class = G_OBJECT_CLASS (class);

    g_type_class_add_private (object_class, sizeof (NmnNewConnectionPrivate));

    /* methods */
    object_class->constructor = constructor;
    object_class->set_property = set_property;
    object_class->get_property = get_property;
    object_class->dispose = dispose;

    /* properties */
    g_object_class_install_property
        (object_class, PROP_MODEL,
         g_param_spec_object (NMN_NEW_CONNECTION_MODEL,
                              "NmnModel",
                              "NmnModel",
                              NMN_TYPE_MODEL,
                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    /* signals */
    signals[CLOSE] = g_signal_new 
        ("close",
         G_OBJECT_CLASS_TYPE (class),
         G_SIGNAL_RUN_LAST,
         G_STRUCT_OFFSET (NmnNewConnectionClass, close),
         NULL, NULL,
         g_cclosure_marshal_VOID__VOID,
         G_TYPE_NONE, 0);
}
