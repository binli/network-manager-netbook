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
#include <dbus/dbus-glib-lowlevel.h>
#include <nbtk/nbtk-gtk.h>
#include <nm-device-ethernet.h>
#include <nm-device-wifi.h>
#include <nm-serial-device.h>
#include <nm-device-bt.h>
#include "nmn-applet.h"
#include "nmn-new-connection.h"

#include "nmn-model.h"
#include "nmn-list.h"

G_DEFINE_TYPE (NmnApplet, nmn_applet, GTK_TYPE_NOTEBOOK)

enum {
    PROP_0,
    PROP_MODEL,

    LAST_PROP
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), NMN_TYPE_APPLET, NmnAppletPrivate))

typedef struct {
    NmnModel *model;

    GtkWidget *pane;
    GtkWidget *list;

    GtkWidget *switches_table;
    GtkWidget *enable_wifi_label;
    GtkWidget *enable_wifi;
    GtkWidget *enable_ethernet_label;
    GtkWidget *enable_ethernet;
    GtkWidget *enable_3g_label;
    GtkWidget *enable_3g;
    GtkWidget *enable_wimax_label;
    GtkWidget *enable_wimax;
    GtkWidget *enable_bt_label;
    GtkWidget *enable_bt;
    GtkWidget *enable_network;
    GtkWidget *add_new_connection;

    GtkWidget *new_dialog;

    gboolean network_list_populated;

    gboolean disposed;
} NmnAppletPrivate;

enum {
    PAGE_NETWORKS       = 0,
    PAGE_ADD_CONNECTION = 1,
};

enum {
    SWITCHES_ROW_WIFI     = 0,
    SWITCHES_ROW_ETHERNET = 1,
    SWITCHES_ROW_3G       = 2,
    SWITCHES_ROW_WIMAX    = 3,
    SWITCHES_ROW_BT       = 4
};

static void add_new_connection_hide (GtkWidget *widget, gpointer user_data);
static void wifi_toggle_set_sensitive (NmnModel *model, GtkWidget *widget, gboolean suggested);

NmnApplet *
nmn_applet_new (NmnModel *model)
{
    g_return_val_if_fail (NMN_IS_MODEL (model), NULL);

    return NMN_APPLET (g_object_new (NMN_TYPE_APPLET,
                                     NMN_APPLET_MODEL, model,
                                     NULL));
}

static void
update_switches_visibility (NmnApplet *applet)
{
    NmnAppletPrivate *priv = GET_PRIVATE (applet);
    GtkTable *table;
    gboolean visible;
    int ethernet;
    int wifi;
    int modem;
    int wimax;
    int bt;

    ethernet = wifi = modem = wimax = bt = 0;

    if (!nmn_model_offline_mode_get_active (priv->model)) {
        const GPtrArray *devices;
        int i;

        devices = nm_client_get_devices (nmn_model_get_client (priv->model));
        for (i = 0; devices && i < devices->len; i++) {
            NMDevice *device = NM_DEVICE (g_ptr_array_index (devices, i));

            if (NM_IS_DEVICE_ETHERNET (device))
                ethernet++;
            else if (NM_IS_DEVICE_WIFI (device))
                wifi++;
            else if (NM_IS_SERIAL_DEVICE (device))
                modem++;
            else if (NM_IS_DEVICE_BT (device))
                bt++;
        }
    }

    table = GTK_TABLE (priv->switches_table);

    visible = wifi > 0;
    gtk_table_set_row_spacing (table, SWITCHES_ROW_WIFI, visible ? 6 : 0);
    g_object_set (priv->enable_wifi_label, "visible", visible, NULL);
    g_object_set (priv->enable_wifi, "visible", visible, NULL);

    visible = ethernet > 0;
    gtk_table_set_row_spacing (table, SWITCHES_ROW_ETHERNET, visible ? 6 : 0);
    g_object_set (priv->enable_ethernet_label, "visible", visible, NULL);
    g_object_set (priv->enable_ethernet, "visible", visible, NULL);

    visible = modem > 0;
    gtk_table_set_row_spacing (table, SWITCHES_ROW_3G, visible ? 6 : 0);
    g_object_set (priv->enable_3g_label, "visible", visible, NULL);
    g_object_set (priv->enable_3g, "visible", visible, NULL);

    visible = wimax > 0;
    gtk_table_set_row_spacing (table, SWITCHES_ROW_WIMAX, visible ? 6 : 0);
    g_object_set (priv->enable_wimax_label, "visible", visible, NULL);
    g_object_set (priv->enable_wimax, "visible", visible, NULL);

    visible = bt > 0;
    gtk_table_set_row_spacing (table, SWITCHES_ROW_BT, visible ? 6 : 0);
    g_object_set (priv->enable_bt_label, "visible", visible, NULL);
    g_object_set (priv->enable_bt, "visible", visible, NULL);
}

void
nmn_applet_show (NmnApplet *applet)
{
}

void
nmn_applet_hide (NmnApplet *applet)
{
    add_new_connection_hide (NULL, applet);
}

/* enable/disable wifi button */

static void
enable_wifi_toggled (NbtkGtkLightSwitch *w,
                     gboolean active,
                     gpointer user_data)
{
    NmnAppletPrivate *priv = GET_PRIVATE (user_data);

    nmn_model_wifi_toggled (priv->model, active);
}

static void
wifi_toggle_set_sensitive (NmnModel *model,
                           GtkWidget *widget,
                           gboolean suggested)
{
    if (suggested == FALSE || nmn_model_wifi_can_change (model) == FALSE)
        gtk_widget_set_sensitive (widget, FALSE);
    else
        gtk_widget_set_sensitive (widget, TRUE);
}

static void
wifi_toggled (NmnModel *model,
              gboolean active,
              gpointer user_data)
{
    NmnAppletPrivate *priv = GET_PRIVATE (user_data);

    g_signal_handlers_block_by_func (priv->model, enable_wifi_toggled, user_data);
    nbtk_gtk_light_switch_set_active (NBTK_GTK_LIGHT_SWITCH (priv->enable_wifi), active);
    g_signal_handlers_unblock_by_func (priv->model, enable_wifi_toggled, user_data);

    wifi_toggle_set_sensitive (priv->model, priv->enable_wifi, TRUE);
}

static void
enable_wifi_setup (NmnApplet *applet)
{
    NmnAppletPrivate *priv = GET_PRIVATE (applet);

    g_signal_connect (priv->enable_wifi, "switch-flipped", G_CALLBACK (enable_wifi_toggled), applet);
    gtk_widget_show (priv->enable_wifi);

    g_signal_connect (priv->model, "wifi-toggled", G_CALLBACK (wifi_toggled), applet);
    wifi_toggled (priv->model, nmn_model_wifi_get_active (priv->model), applet);
}

/* enable/disable ethernet button */

static void
enable_ethernet_toggled (NbtkGtkLightSwitch *w,
                         gboolean active,
                         gpointer user_data)
{
    NmnAppletPrivate *priv = GET_PRIVATE (user_data);

    nmn_model_ethernet_toggled (priv->model, active);
}

static void
ethernet_toggled (NmnModel *model,
                  gboolean active,
                  gpointer user_data)
{
    NmnAppletPrivate *priv = GET_PRIVATE (user_data);

    g_signal_handlers_block_by_func (priv->model, enable_ethernet_toggled, user_data);
    nbtk_gtk_light_switch_set_active (NBTK_GTK_LIGHT_SWITCH (priv->enable_ethernet), active);
    g_signal_handlers_unblock_by_func (priv->model, enable_ethernet_toggled, user_data);
}

static void
enable_ethernet_setup (NmnApplet *applet)
{
    NmnAppletPrivate *priv = GET_PRIVATE (applet);

    g_signal_connect (priv->enable_ethernet, "switch-flipped", G_CALLBACK (enable_ethernet_toggled), applet);
    gtk_widget_show (priv->enable_ethernet);

    g_signal_connect (priv->model, "ethernet-toggled", G_CALLBACK (ethernet_toggled), applet);
    ethernet_toggled (priv->model, nmn_model_ethernet_get_active (priv->model), applet);
}

/* enable/disable 3G button */

static void
enable_3g_toggled (NbtkGtkLightSwitch *w,
                   gboolean active,
                   gpointer user_data)
{
    NmnAppletPrivate *priv = GET_PRIVATE (user_data);

    nmn_model_modems_toggled (priv->model, active);
}

static void
modems_toggled (NmnModel *model,
                gboolean active,
                gpointer user_data)
{
    NmnAppletPrivate *priv = GET_PRIVATE (user_data);

    g_signal_handlers_block_by_func (priv->model, enable_3g_toggled, user_data);
    nbtk_gtk_light_switch_set_active (NBTK_GTK_LIGHT_SWITCH (priv->enable_3g), active);
    g_signal_handlers_unblock_by_func (priv->model, enable_3g_toggled, user_data);
}

static void
enable_3g_setup (NmnApplet *applet)
{
    NmnAppletPrivate *priv = GET_PRIVATE (applet);

    g_signal_connect (priv->enable_3g, "switch-flipped", G_CALLBACK (enable_3g_toggled), applet);
    gtk_widget_show (priv->enable_3g);

    g_signal_connect (priv->model, "modems-toggled", G_CALLBACK (modems_toggled), applet);
    modems_toggled (priv->model, nmn_model_modems_get_active (priv->model), applet);
}

/* enable/disable Bluetooth button */

static void
enable_bt_toggled (NbtkGtkLightSwitch *w,
                   gboolean active,
                   gpointer user_data)
{
    NmnAppletPrivate *priv = GET_PRIVATE (user_data);

    nmn_model_bt_toggled (priv->model, active);
}

static void
bt_toggled (NmnModel *model,
            gboolean active,
            gpointer user_data)
{
    NmnAppletPrivate *priv = GET_PRIVATE (user_data);

    g_signal_handlers_block_by_func (priv->model, enable_bt_toggled, user_data);
    nbtk_gtk_light_switch_set_active (NBTK_GTK_LIGHT_SWITCH (priv->enable_bt), active);
    g_signal_handlers_unblock_by_func (priv->model, enable_bt_toggled, user_data);
}

static void
enable_bt_setup (NmnApplet *applet)
{
    NmnAppletPrivate *priv = GET_PRIVATE (applet);

    g_signal_connect (priv->enable_bt, "switch-flipped", G_CALLBACK (enable_bt_toggled), applet);
    gtk_widget_show (priv->enable_bt);

    g_signal_connect (priv->model, "bt-toggled", G_CALLBACK (bt_toggled), applet);
    bt_toggled (priv->model, nmn_model_bt_get_active (priv->model), applet);
}

/* enable/disable Offline mode button */

static void
enable_network_toggled (NbtkGtkLightSwitch *w,
                        gboolean active,
                        gpointer user_data)
{
    NmnAppletPrivate *priv = GET_PRIVATE (user_data);

    nmn_model_offline_mode_toggled (priv->model, active);
}

static void
offline_toggled (NmnModel *model,
                 gboolean active,
                 gpointer user_data)
{
    NmnAppletPrivate *priv = GET_PRIVATE (user_data);

    g_signal_handlers_block_by_func (priv->model, enable_network_toggled, user_data);
    nbtk_gtk_light_switch_set_active (NBTK_GTK_LIGHT_SWITCH (priv->enable_network), active);
    g_signal_handlers_unblock_by_func (priv->model, enable_network_toggled, user_data);

    update_switches_visibility (NMN_APPLET (user_data));
}

static void
enable_network_setup (NmnApplet *applet)
{
    NmnAppletPrivate *priv = GET_PRIVATE (applet);

    g_signal_connect (priv->enable_network, "switch-flipped", G_CALLBACK (enable_network_toggled), applet);
    gtk_widget_show (priv->enable_network);

    g_signal_connect (priv->model, "offline-mode-toggled", G_CALLBACK (offline_toggled), applet);
    offline_toggled (priv->model, nmn_model_offline_mode_get_active (priv->model), applet);
}

static void
add_new_connection_show (GtkButton *button,
                         gpointer user_data)
{
    NmnAppletPrivate *priv = GET_PRIVATE (user_data);

    nmn_new_connection_update (NMN_NEW_CONNECTION (priv->new_dialog));
    gtk_notebook_set_current_page (GTK_NOTEBOOK (user_data), PAGE_ADD_CONNECTION);
}

static void
add_new_connection_hide (GtkWidget *widget,
                         gpointer user_data)
{
    gtk_notebook_set_current_page (GTK_NOTEBOOK (user_data), PAGE_NETWORKS);
}

/* add new connection button */

static void
add_new_connection_setup (NmnApplet *applet)
{
    NmnAppletPrivate *priv = GET_PRIVATE (applet);

    priv->new_dialog = nmn_new_connection_create (priv->model);
    gtk_notebook_append_page (GTK_NOTEBOOK (applet), priv->new_dialog, NULL);
    gtk_widget_modify_bg (priv->new_dialog, GTK_STATE_NORMAL, &gtk_widget_get_style (priv->new_dialog)->white);
    g_signal_connect (priv->new_dialog, "close", G_CALLBACK (add_new_connection_hide), applet);
    g_signal_connect (priv->add_new_connection, "clicked", G_CALLBACK (add_new_connection_show), applet);
}

static gboolean
devices_changed_cb (gpointer user_data)
{
    update_switches_visibility (NMN_APPLET (user_data));
    return FALSE;
}

static void
devices_changed (NMClient *client,
                 NMDevice *device,
                 gpointer user_data)
{
    /* Do it in the idle handler, otherwise the client still has the device
       which is being removed here */
    g_idle_add (devices_changed_cb, user_data);
}

static void
nmn_applet_init (NmnApplet *applet)
{
    NmnAppletPrivate *priv = GET_PRIVATE (applet);
    GtkWidget *w;
    GtkWidget *vbox;
    GtkWidget *frame;
    GtkWidget *table;
    const char *markup_str;
    char *label;

    gtk_notebook_set_show_tabs (GTK_NOTEBOOK (applet), FALSE);

    priv->pane = gtk_hbox_new (FALSE, 6);
    gtk_container_set_border_width (GTK_CONTAINER (priv->pane), 6);
    gtk_notebook_append_page (GTK_NOTEBOOK (applet), priv->pane, NULL);

    /* left side (Networks list, add new connection button) */
    vbox = gtk_vbox_new (FALSE, 6);
    gtk_box_pack_start (GTK_BOX (priv->pane), vbox, TRUE, TRUE, 0);

    frame = nbtk_gtk_frame_new ();
    gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);

    priv->add_new_connection = gtk_button_new_with_label (_("Add new connection"));
    gtk_box_pack_start (GTK_BOX (vbox), priv->add_new_connection, FALSE, FALSE, 0);

    vbox = gtk_vbox_new (FALSE, 6);
    gtk_container_add (GTK_CONTAINER (frame), vbox);

    label = g_strdup_printf ("<span font_desc=\"Sans Bold 18\" foreground=\"#3e3e3e\">%s</span>", _("Networks"));
    w = gtk_label_new (NULL);
    gtk_label_set_markup (GTK_LABEL (w), label);
    g_free (label);
    gtk_misc_set_alignment (GTK_MISC (w), 0.0, 0.5);
    gtk_box_pack_start (GTK_BOX (vbox), w, FALSE, FALSE, 0);

    w = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (w), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    priv->list = nmn_list_new ();
    gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (w), priv->list);
    gtk_widget_show (priv->list);
    gtk_box_pack_start (GTK_BOX (vbox), w, TRUE, TRUE, 0);


    /* Right side, switches */
    vbox = gtk_vbox_new (FALSE, 6);
    gtk_box_pack_start (GTK_BOX (priv->pane), vbox, FALSE, FALSE, 0);

    frame = nbtk_gtk_frame_new ();
    gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);

    w = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (frame), w);

    table = gtk_table_new (5, 2, FALSE);
    priv->switches_table = table;
    gtk_table_set_row_spacings (GTK_TABLE (table), 6);
    gtk_container_set_border_width (GTK_CONTAINER (table), 6);
    gtk_box_pack_start (GTK_BOX (w), table, FALSE, FALSE, 0);

    markup_str = "<span font_desc=\"Sans Bold 12\" foreground=\"#3e3e3e\">%s</span>";

    w = gtk_label_new (NULL);
    label = g_strdup_printf (markup_str, _("WiFi"));
    gtk_label_set_markup (GTK_LABEL (w), label);
    g_free (label);
    priv->enable_wifi_label = w;
    gtk_misc_set_alignment (GTK_MISC (w), 0.2, 0.5);
    gtk_table_attach_defaults (GTK_TABLE (table), w, 0, 1, SWITCHES_ROW_WIFI, SWITCHES_ROW_WIFI + 1);
    priv->enable_wifi = nbtk_gtk_light_switch_new ();
    gtk_table_attach_defaults (GTK_TABLE (table), priv->enable_wifi, 1, 2,
                               SWITCHES_ROW_WIFI, SWITCHES_ROW_WIFI + 1);

    w = gtk_label_new (NULL);
    label = g_strdup_printf (markup_str, _("Wired"));
    gtk_label_set_markup (GTK_LABEL (w), label);
    g_free (label);
    priv->enable_ethernet_label = w;
    gtk_misc_set_alignment (GTK_MISC (w), 0.2, 0.5);
    gtk_table_attach_defaults (GTK_TABLE (table), w, 0, 1, SWITCHES_ROW_ETHERNET, SWITCHES_ROW_ETHERNET + 1);
    priv->enable_ethernet = nbtk_gtk_light_switch_new ();
    gtk_table_attach_defaults (GTK_TABLE (table), priv->enable_ethernet, 1, 2,
                               SWITCHES_ROW_ETHERNET, SWITCHES_ROW_ETHERNET + 1);

    w = gtk_label_new (NULL);
    label = g_strdup_printf (markup_str, _("3G"));
    gtk_label_set_markup (GTK_LABEL (w), label);
    g_free (label);
    priv->enable_3g_label = w;
    gtk_misc_set_alignment (GTK_MISC (w), 0.2, 0.5);
    gtk_table_attach_defaults (GTK_TABLE (table), w, 0, 1, SWITCHES_ROW_3G, SWITCHES_ROW_3G + 1);
    priv->enable_3g = nbtk_gtk_light_switch_new ();
    gtk_table_attach_defaults (GTK_TABLE (table), priv->enable_3g, 1, 2,
                               SWITCHES_ROW_3G, SWITCHES_ROW_3G + 1);


    w = gtk_label_new (NULL);
    label = g_strdup_printf (markup_str, _("WiMAX"));
    gtk_label_set_markup (GTK_LABEL (w), label);
    g_free (label);
    priv->enable_wimax_label = w;
    gtk_misc_set_alignment (GTK_MISC (w), 0.2, 0.5);
    gtk_table_attach_defaults (GTK_TABLE (table), w, 0, 1, SWITCHES_ROW_WIMAX, SWITCHES_ROW_WIMAX + 1);
    priv->enable_wimax = nbtk_gtk_light_switch_new ();
    gtk_table_attach_defaults (GTK_TABLE (table), priv->enable_wimax, 1, 2,
                               SWITCHES_ROW_WIMAX, SWITCHES_ROW_WIMAX + 1);

    w = gtk_label_new (NULL);
    label = g_strdup_printf (markup_str, _("Bluetooth"));
    gtk_label_set_markup (GTK_LABEL (w), label);
    g_free (label);
    priv->enable_bt_label = w;
    gtk_misc_set_alignment (GTK_MISC (w), 0.2, 0.5);
    gtk_table_attach_defaults (GTK_TABLE (table), w, 0, 1, SWITCHES_ROW_BT, SWITCHES_ROW_BT + 1);
    priv->enable_bt = nbtk_gtk_light_switch_new ();
    gtk_table_attach_defaults (GTK_TABLE (table), priv->enable_bt, 1, 2,
                               SWITCHES_ROW_BT, SWITCHES_ROW_BT + 1);

    frame = nbtk_gtk_frame_new ();
    gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

    table = gtk_table_new (2, 2, TRUE);
    gtk_container_add (GTK_CONTAINER (frame), table);

    w = gtk_label_new (NULL);
    label = g_strdup_printf (markup_str, _("Offline mode"));
    gtk_label_set_markup (GTK_LABEL (w), label);
    g_free (label);
    gtk_misc_set_alignment (GTK_MISC (w), 0.2, 0.5);
    gtk_table_attach_defaults (GTK_TABLE (table), w, 0, 1, 0, 1);
    priv->enable_network = nbtk_gtk_light_switch_new ();
    gtk_table_attach_defaults (GTK_TABLE (table), priv->enable_network, 1, 2, 0, 1);

    w = gtk_label_new (_("This will disable all your connections"));
    gtk_table_attach_defaults (GTK_TABLE (table), w, 0, 2, 1, 2);

    gtk_widget_show_all (priv->pane);
}

static GObject*
constructor (GType type,
             guint n_construct_params,
             GObjectConstructParam *construct_params)
{
    GObject *object;
    NmnApplet *applet;
    NmnAppletPrivate *priv;
    NMClient *client;

    object = G_OBJECT_CLASS (nmn_applet_parent_class)->constructor
        (type, n_construct_params, construct_params);

    if (!object)
        return NULL;

    applet = NMN_APPLET (object);
    priv = GET_PRIVATE (applet);

    if (!priv->model) {
        g_warning ("Missing constructor arguments");
        g_object_unref (object);
        return NULL;
    }

    nmn_list_set_model (NMN_LIST (priv->list), GTK_TREE_MODEL (priv->model));
    enable_wifi_setup (applet);
    enable_ethernet_setup (applet);
    enable_3g_setup (applet);
    enable_bt_setup (applet);
    enable_network_setup (applet);
    add_new_connection_setup (applet);
    add_new_connection_hide (NULL, applet);

    client = nmn_model_get_client (priv->model);
    g_signal_connect (client, "device-added", G_CALLBACK (devices_changed), applet);
    g_signal_connect (client, "device-removed", G_CALLBACK (devices_changed), applet);
    update_switches_visibility (applet);

    return object;
}

static void
set_property (GObject *object, guint prop_id,
              const GValue *value, GParamSpec *pspec)
{
    NmnAppletPrivate *priv = GET_PRIVATE (object);

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
    NmnAppletPrivate *priv = GET_PRIVATE (object);

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
    NmnAppletPrivate *priv = GET_PRIVATE (object);

    if (!priv->disposed) {
        if (priv->model) {
            g_signal_handlers_disconnect_by_func (nmn_model_get_client (priv->model), devices_changed, object);
            g_object_unref (priv->model);
        }

        priv->disposed = TRUE;
    }

    G_OBJECT_CLASS (nmn_applet_parent_class)->dispose (object);
}

static void
nmn_applet_class_init (NmnAppletClass *class)
{
    GObjectClass *object_class = G_OBJECT_CLASS (class);

    g_type_class_add_private (object_class, sizeof (NmnAppletPrivate));

    object_class->constructor = constructor;
    object_class->set_property = set_property;
    object_class->get_property = get_property;
    object_class->dispose = dispose;

    /* properties */
    g_object_class_install_property
        (object_class, PROP_MODEL,
         g_param_spec_object (NMN_APPLET_MODEL,
                              "NmnModel",
                              "NmnModel",
                              NMN_TYPE_MODEL,
                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}
