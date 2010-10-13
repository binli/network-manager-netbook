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

#include <string.h>
#include <glib/gi18n.h>
#include "nmn-network-renderer.h"
#include "nm-list-item.h"
#include "nm-icon-cache.h"
#include "nm-connection-item.h"
#include "nm-device-item.h"
#include "nmn-connection-details.h"
#include "utils.h"

G_DEFINE_TYPE (NmnNetworkRenderer, nmn_network_renderer, NMN_TYPE_ITEM_RENDERER)

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), NMN_TYPE_NETWORK_RENDERER, NmnNetworkRendererPrivate))

typedef struct {
    GtkBox *vbox; /* child of self */
    GtkBox *hbox; /* child of vbox */

    GtkImage *icon;
    GtkLabel *name_and_status;
    GtkLabel *security_label;
    GtkWidget *expander;
    NmnConnectionDetails *details;
    GtkWidget *connect_button;
    GtkWidget *remove_button;

    gulong item_changed_id;
} NmnNetworkRendererPrivate;

static NmnConnectionDetails *get_details (NmnNetworkRenderer *self);
static void renderer_background_updated (NmnItemRenderer *item_renderer, gboolean prelight);

GtkWidget *
nmn_network_renderer_new (void)
{
    return (GtkWidget *) g_object_new (NMN_TYPE_NETWORK_RENDERER, NULL);
}

static void
update_details (NmnNetworkRenderer *self)
{
    NmnNetworkRendererPrivate *priv = GET_PRIVATE (self);
    NMConnection *connection;
    const char *hw_address;
    NMSettingIP4Config *setting = NULL;
    NMIP4Config *config = NULL;
    NMListItem *item;

    if (!priv->details)
        return;

    item = nmn_item_renderer_get_item (NMN_ITEM_RENDERER (self));
    connection = (NMConnection *) nm_connection_item_get_connection (NM_CONNECTION_ITEM (item));
    if (connection) {
        NMDevice *device;

        device = nm_device_item_get_device (NM_DEVICE_ITEM (item));
        setting = (NMSettingIP4Config *) nm_connection_get_setting (connection, NM_TYPE_SETTING_IP4_CONFIG);
    
        if (device && nm_device_get_state (device) == NM_DEVICE_STATE_ACTIVATED)
            config = nm_device_get_ip4_config (device);
    }

    hw_address = nm_device_item_get_hw_address (NM_DEVICE_ITEM (item));
    nmn_connection_details_set_data (priv->details, setting, config, hw_address);
}

static void
item_changed (NMListItem *item,
              GParamSpec *spec,
              gpointer user_data)
{
    NmnNetworkRenderer *self = NMN_NETWORK_RENDERER (user_data);
    NmnNetworkRendererPrivate *priv = GET_PRIVATE (self);
    const char *property = spec ? spec->name : NULL;
    char *str;

    if (!property || !strcmp (property, NM_LIST_ITEM_NAME) ||
        !strcmp (property, NM_LIST_ITEM_STATUS) || !strcmp (property, NM_CONNECTION_ITEM_CONNECTION)) {
        const char *status_str;
        const char *button_label;
        NMListItemStatus status;

        status = nm_list_item_get_status (item);
        switch (status) {
        case NM_LIST_ITEM_STATUS_CONNECTED:
            status_str = _("Connected");
            button_label = _("Disconnect");
            break;
        case NM_LIST_ITEM_STATUS_CONNECTING:
            status_str = _("Connecting");
            button_label = _("Cancel");
            break;
        default:
            status_str = _("Disconnected");
            button_label = _("Connect");
            break;
        }

        if (NM_IS_CONNECTION_ITEM (item) && nm_connection_item_get_connection (NM_CONNECTION_ITEM (item)))
            str = g_strdup_printf ("<big><b>%s - %s</b></big>", nm_list_item_get_name (item), status_str);
        else {
            const char *available_str = _("Available");

            str = g_strdup_printf ("<big><b>%s - %s</b></big>",
                                   nm_list_item_get_name (item),
                                   available_str);
        }

        gtk_label_set_markup (priv->name_and_status, str);
        g_free (str);

        gtk_button_set_label (GTK_BUTTON (priv->connect_button), button_label);
        update_details (self);
        renderer_background_updated (NMN_ITEM_RENDERER (self),
                                     nmn_item_renderer_is_prelight (NMN_ITEM_RENDERER (self)));
    }

    if (!property || !strcmp (property, NM_LIST_ITEM_ICON))
        gtk_image_set_from_pixbuf (priv->icon, nm_icon_cache_get (nm_list_item_get_icon (item)));

    if (!property || !strcmp (property, NM_LIST_ITEM_SECURITY))
        gtk_label_set_text (priv->security_label, nm_list_item_get_security (item));
}

static void
update_connection_cb (NMSettingsConnectionInterface *connection,
                      GError *error,
                      gpointer user_data)
{
    NMListItem *item;

    item = nmn_item_renderer_get_item (NMN_ITEM_RENDERER (user_data));

    if (error) {
        char *msg;

        msg = g_strdup_printf (_("Could not update connection: %s"), error->message);
        nm_list_item_warning (item, msg);
        g_free (msg);
    } else
        nm_list_item_connect (item);
}

static void
connect_with_updated_details (NmnItemRenderer *self,
                              NMConnection *connection,
                              gboolean new_connection)
{
    NmnConnectionDetails *details;
    gboolean changed = FALSE;

    details = GET_PRIVATE (self)->details;
    if (details) {
        NMSetting *current_config;
        NMSetting *new_config;

        current_config = nm_connection_get_setting (connection, NM_TYPE_SETTING_IP4_CONFIG);
        new_config = NM_SETTING (nmn_connection_details_get_data (NMN_CONNECTION_DETAILS (details)));

        if (current_config == NULL || nm_setting_compare (current_config, new_config, 0) == FALSE) {
            nm_connection_add_setting (connection, new_config);
            changed = TRUE;
        }
    }

    if (new_connection) {
        NMConnectionItem *connection_item = NM_CONNECTION_ITEM (nmn_item_renderer_get_item (self));

        nm_connection_item_new_connection (connection_item, connection, TRUE);
        g_object_unref (connection);
    } else if (changed)
        nm_settings_connection_interface_update (NM_SETTINGS_CONNECTION_INTERFACE (connection),
                                                 update_connection_cb, self);
    else
        nm_list_item_connect (nmn_item_renderer_get_item (self));
}

static void
connection_created_cb (GObject *object,
                       GAsyncResult *result,
                       gpointer user_data)
{
    NMConnection *connection;
    GError *error = NULL;

    connection = nm_connection_item_create_connection_finish (NM_CONNECTION_ITEM (object), result, &error);
    if (connection)
        connect_with_updated_details (NMN_ITEM_RENDERER (user_data), connection, TRUE);
}

static void
connect_button_clicked (GtkButton *button, gpointer user_data)
{
    NmnItemRenderer *self = NMN_ITEM_RENDERER (user_data);
    NMListItem *item;

    nmn_item_renderer_hide_error (self);

    item = nmn_item_renderer_get_item (self);
    if (nm_list_item_get_status (item) == NM_LIST_ITEM_STATUS_DISCONNECTED) {
        NMConnectionItem *connection_item;
        NMConnection *connection;

        connection_item = NM_CONNECTION_ITEM (nmn_item_renderer_get_item (self));
        connection = (NMConnection *) nm_connection_item_get_connection (connection_item);
        if (connection)
            connect_with_updated_details (self, connection, FALSE);
        else
            /* We don't have a connection yet, so create one */
            nm_connection_item_create_connection (connection_item, connection_created_cb, self);
    } else
        nm_list_item_disconnect (item);
}

static void
details_changed (NmnConnectionDetails *details,
                 gboolean complete,
                 gpointer user_data)
{
    NmnNetworkRendererPrivate *priv = GET_PRIVATE (user_data);

    nmn_item_renderer_hide_error (NMN_ITEM_RENDERER (user_data));
    gtk_widget_set_sensitive (GTK_WIDGET (priv->connect_button), complete);
}

static NmnConnectionDetails *
get_details (NmnNetworkRenderer *self)
{
    NmnNetworkRendererPrivate *priv = GET_PRIVATE (self);
    GtkWidget *alignment;
    NMListItem *item;

    if (priv->details)
        return priv->details;

    item = nmn_item_renderer_get_item (NMN_ITEM_RENDERER (self));
    if (!NM_IS_DEVICE_ITEM (item) || !nm_device_item_get_device (NM_DEVICE_ITEM (item)))
        return NULL;

    priv->details = nmn_connection_details_new ();
    update_details (self);

    alignment = gtk_alignment_new (0, 0, 0, 0);
    gtk_container_add (GTK_CONTAINER (alignment), GTK_WIDGET (priv->details));
    gtk_widget_show (alignment);
    gtk_box_pack_end (priv->vbox, alignment, FALSE, FALSE, 0);

    g_signal_connect (priv->details, "changed", G_CALLBACK (details_changed), self);
    details_changed (priv->details, nmn_connection_details_verify (priv->details), self);

    return priv->details;
}

static void
advanced_expanded (GtkExpander *expander,
                   GParamSpec *param_spec,
                   gpointer user_data)
{
    NmnConnectionDetails *details;

    nmn_item_renderer_hide_error (NMN_ITEM_RENDERER (user_data));
    details = get_details (NMN_NETWORK_RENDERER (user_data));
    if (details)
        g_object_set (details, "visible", gtk_expander_get_expanded (expander), NULL);
}

static void
remove_button_clicked (GtkButton *button, gpointer user_data)
{
    NmnItemRenderer *renderer = NMN_ITEM_RENDERER (user_data);
    GtkDialog *dialog;
    GtkWidget *label;
    const char *name;
    char *label_text;
    NMListItem *item;

    nmn_item_renderer_hide_error (renderer);
    item = nmn_item_renderer_get_item (renderer);
    dialog = GTK_DIALOG (gtk_dialog_new_with_buttons (_("Really remove?"),
                                                      NULL,
                                                      GTK_DIALOG_MODAL |
                                                      GTK_DIALOG_DESTROY_WITH_PARENT,
                                                      _("No, save"),
                                                      GTK_RESPONSE_REJECT,
                                                      _("Yes, delete"),
                                                      GTK_RESPONSE_ACCEPT,
                                                      NULL));

    gtk_dialog_set_has_separator (dialog, FALSE);
    gtk_dialog_set_default_response (dialog, GTK_RESPONSE_ACCEPT);
    gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
    gtk_window_set_icon_name (GTK_WINDOW (dialog), GTK_STOCK_DELETE);

    name = nm_list_item_get_name (item);
    label_text = g_strdup_printf (_("Do you want to remove the '%s' %s network? "
                                    "This\nwill forget the password and you will"
                                    " no longer be\nautomatically connected to "
                                    "'%s'."),
                                  name,
                                  nm_list_item_get_type_name (item),
                                  name);

    label = gtk_label_new (label_text);
    g_free (label_text);

    gtk_box_set_spacing (GTK_BOX (dialog->vbox), 12);
    gtk_box_pack_start (GTK_BOX (dialog->vbox), label, TRUE, TRUE, 6);
    gtk_widget_show_all (GTK_WIDGET (dialog));

    if (gtk_dialog_run (dialog) == GTK_RESPONSE_ACCEPT)
        nm_list_item_delete (item);

    gtk_widget_destroy (GTK_WIDGET (dialog));
    nm_utils_dialog_done ();
}

static void
init_widgets (NmnNetworkRenderer *self)
{
    NmnNetworkRendererPrivate *priv = GET_PRIVATE (self);
    GtkWidget *parent_container;
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *w;

    parent_container = nmn_item_renderer_get_content_area (NMN_ITEM_RENDERER (self));

    priv->vbox = GTK_BOX (gtk_vbox_new (FALSE, 6));
    gtk_container_set_border_width (GTK_CONTAINER (priv->vbox), 6);
    gtk_container_add (GTK_CONTAINER (parent_container), GTK_WIDGET (priv->vbox));

    priv->hbox = GTK_BOX (gtk_hbox_new (FALSE, 6));
    gtk_container_set_border_width (GTK_CONTAINER (priv->hbox), 6);
    gtk_box_pack_start (priv->vbox, GTK_WIDGET (priv->hbox), FALSE, FALSE, 0);

    priv->icon = GTK_IMAGE (gtk_image_new ());
    gtk_box_pack_start (priv->hbox, GTK_WIDGET (priv->icon), FALSE, FALSE, 0);

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_box_pack_start (priv->hbox, vbox, FALSE, FALSE, 0);

    w = gtk_label_new ("");
    gtk_label_set_use_markup (GTK_LABEL (w), TRUE);
    gtk_misc_set_alignment (GTK_MISC (w), 0.0, 0.5);
    gtk_box_pack_start (GTK_BOX (vbox), w, FALSE, FALSE, 0);
    priv->name_and_status = GTK_LABEL (w);

    hbox = gtk_hbox_new (FALSE, 12);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    w = gtk_label_new ("");
    gtk_box_pack_start (GTK_BOX (hbox), w, FALSE, FALSE, 0);
    priv->security_label = GTK_LABEL (w);

    /* FIXME: this should be visibile only for NMDeviceItems */
    w = gtk_expander_new (_("Advanced"));
    gtk_box_pack_start (GTK_BOX (hbox), w, FALSE, FALSE, 0);
    priv->expander = w;
    g_signal_connect (w, "notify::expanded", G_CALLBACK (advanced_expanded), self);

    w = gtk_button_new_with_label (_("Connect"));
    gtk_box_pack_start (GTK_BOX (hbox), w, FALSE, FALSE, 0);
    priv->connect_button = w;
    g_signal_connect (w, "clicked", G_CALLBACK (connect_button_clicked), self);

    /* Remove button */
    vbox = gtk_vbox_new (FALSE, 0);
    gtk_box_pack_end (priv->hbox, vbox, FALSE, FALSE, 0);

    w = gtk_button_new ();
    gtk_widget_set_no_show_all (w, TRUE);
    gtk_button_set_image (GTK_BUTTON (w),
                          gtk_image_new_from_icon_name ("edit-clear", GTK_ICON_SIZE_MENU));

    gtk_button_set_relief (GTK_BUTTON (w), GTK_RELIEF_NONE);
    gtk_button_set_image_position (GTK_BUTTON (w), GTK_POS_RIGHT);
    gtk_widget_set_tooltip_text (w, _("Remove connection"));
    gtk_box_pack_start (GTK_BOX (vbox), w, TRUE, FALSE, 0);
    priv->remove_button = w;
    g_signal_connect (w, "clicked", G_CALLBACK (remove_button_clicked), self);

    gtk_widget_show_all (GTK_WIDGET (self));
}

static void
renderer_background_updated (NmnItemRenderer *item_renderer,
                             gboolean prelight)
{
    NmnNetworkRendererPrivate *priv = GET_PRIVATE (item_renderer);
    NMListItem *item;
    gboolean show_delete;

    item = nmn_item_renderer_get_item (item_renderer);
    if (prelight && item && nm_list_item_get_show_delete (item))
        show_delete = TRUE;
    else
        show_delete = FALSE;

    g_object_set (priv->connect_button, "visible", prelight, NULL);
    g_object_set (priv->remove_button, "visible", show_delete, NULL);
    g_object_set (priv->expander, "visible", prelight, NULL);
}

static void
renderer_item_changed (NmnItemRenderer *item_renderer)
{
    NmnNetworkRendererPrivate *priv = GET_PRIVATE (item_renderer);
    NMListItem *item;

    init_widgets (NMN_NETWORK_RENDERER (item_renderer));

    item = nmn_item_renderer_get_item (item_renderer);
    if (item) {
        priv->item_changed_id = g_signal_connect (item, "notify", G_CALLBACK (item_changed), item_renderer);
        item_changed (item, NULL, item_renderer);
    }
    /* FIXME: disconnect previous handlers */
}

static void
nmn_network_renderer_init (NmnNetworkRenderer *item)
{
}

static void
dispose (GObject *object)
{
    NmnNetworkRendererPrivate *priv = GET_PRIVATE (object);

    if (priv->item_changed_id) {
        NMListItem *item;

        item = nmn_item_renderer_get_item (NMN_ITEM_RENDERER (object));
        g_signal_handler_disconnect (item, priv->item_changed_id);
        priv->item_changed_id = 0;
    }

    G_OBJECT_CLASS (nmn_network_renderer_parent_class)->dispose (object);
}

static void
nmn_network_renderer_class_init (NmnNetworkRendererClass *class)
{
    GObjectClass *object_class = G_OBJECT_CLASS (class);
    NmnItemRendererClass *renderer_class = NMN_ITEM_RENDERER_CLASS (class);

    g_type_class_add_private (object_class, sizeof (NmnNetworkRendererPrivate));

    object_class->dispose = dispose;

    renderer_class->background_updated = renderer_background_updated;
    renderer_class->item_changed = renderer_item_changed;
}
