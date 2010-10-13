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
#include "nmn-gsm-pin-request-renderer.h"
#include "libnm-gtk-gsm-device.h"
#include "nm-gsm-pin-request-item.h"
#include "nm-icon-cache.h"
#include "nm-connection-item.h"
#include "nm-device-item.h"
#include "nmn-connection-details.h"
#include "utils.h"

G_DEFINE_TYPE (NmnGsmPinRequestRenderer, nmn_gsm_pin_request_renderer, NMN_TYPE_ITEM_RENDERER)

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), NMN_TYPE_GSM_PIN_REQUEST_RENDERER, NmnGsmPinRequestRendererPrivate))

typedef struct {
    NMListItem *item;

    GtkWidget *box_to_hide;
    GtkWidget *entry;
    GtkWidget *unlock_button;
    GtkWidget *disable_pin;
} NmnGsmPinRequestRendererPrivate;

GtkWidget *
nmn_gsm_pin_request_renderer_new (void)
{
    return (GtkWidget *) g_object_new (NMN_TYPE_GSM_PIN_REQUEST_RENDERER, NULL);
}

static void
item_warning (NMListItem *item,
              const char *message,
              gpointer user_data)
{
    NmnGsmPinRequestRendererPrivate *priv = GET_PRIVATE (user_data);

    gtk_editable_select_region (GTK_EDITABLE (priv->entry), 0, -1);
    gtk_widget_grab_focus (priv->entry);
}

static void
unlock (NmnGsmPinRequestRenderer *self)
{
    NmnGsmPinRequestRendererPrivate *priv = GET_PRIVATE (self);
    NMListItem *item;
    const char *text;
    gboolean disable_pin;

    text = gtk_entry_get_text (GTK_ENTRY (priv->entry));
    disable_pin = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->disable_pin));

    item = nmn_item_renderer_get_item (NMN_ITEM_RENDERER (self));
    nm_gsm_pin_request_item_unlock (NM_GSM_PIN_REQUEST_ITEM (item), text, disable_pin);
}

static void
sensitize_widgets (NmnGsmPinRequestRenderer *self)
{
    NmnGsmPinRequestRendererPrivate *priv = GET_PRIVATE (self);
    const char *text;
    gboolean sensitive;

    text = gtk_entry_get_text (GTK_ENTRY (priv->entry));
    if (text && strlen (text) > 0)
        sensitive = TRUE;
    else
        sensitive = FALSE;

    gtk_widget_set_sensitive (priv->unlock_button, sensitive);
    gtk_widget_set_sensitive (priv->disable_pin, sensitive);
}

static void
pin_entry_changed (GtkEditable *editable,
                   gpointer user_data)
{
    nmn_item_renderer_hide_error (NMN_ITEM_RENDERER (user_data));
    sensitize_widgets (NMN_GSM_PIN_REQUEST_RENDERER (user_data));
}

static void
init_widgets (NmnGsmPinRequestRenderer *self)
{
    NmnGsmPinRequestRendererPrivate *priv = GET_PRIVATE (self);
    GtkWidget *parent_container;
    GtkWidget *hbox;
    GtkWidget *vbox;
    GtkWidget *w;
    char *str;
    NMListItem *item;

    item = nmn_item_renderer_get_item (NMN_ITEM_RENDERER (self));
    g_signal_connect (item, "warning", G_CALLBACK (item_warning), self);

    parent_container = nmn_item_renderer_get_content_area (NMN_ITEM_RENDERER (self));

    hbox = gtk_hbox_new (FALSE, 12);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);
    gtk_container_add (GTK_CONTAINER (parent_container), hbox);

    w = gtk_image_new_from_pixbuf (nm_icon_cache_get (nm_list_item_get_icon (item)));
    gtk_box_pack_start (GTK_BOX (hbox), w, FALSE, FALSE, 0);

    vbox = gtk_vbox_new (FALSE, 6);
    gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);

    w = gtk_label_new ("");
    gtk_label_set_use_markup (GTK_LABEL (w), TRUE);
    str = g_strdup_printf ("<big><b>%s</b></big>", nm_list_item_get_name (item));
    gtk_label_set_markup (GTK_LABEL (w), str);
    g_free (str);
    gtk_misc_set_alignment (GTK_MISC (w), 0.0, 0.5);
    gtk_box_pack_start (GTK_BOX (vbox), w, FALSE, FALSE, 0);

    hbox = gtk_hbox_new (FALSE, 12);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
    priv->box_to_hide = hbox;

    w = gtk_entry_new ();
    gtk_entry_set_visibility (GTK_ENTRY (w), FALSE);
    gtk_box_pack_start (GTK_BOX (hbox), w, FALSE, FALSE, 0);
    priv->entry = w;
    g_signal_connect_swapped (w, "activate", G_CALLBACK (unlock), self);
    g_signal_connect (w, "changed", G_CALLBACK (pin_entry_changed), self);

    w = gtk_button_new_with_label (_("Unlock"));
    gtk_box_pack_start (GTK_BOX (hbox), w, FALSE, FALSE, 0);
    priv->unlock_button = w;
    g_signal_connect_swapped (w, "clicked", G_CALLBACK (unlock), self);

    w = gtk_check_button_new_with_label (_("Disable PIN"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), TRUE);
    gtk_box_pack_start (GTK_BOX (hbox), w, FALSE, FALSE, 0);
    priv->disable_pin = w;

    gtk_widget_show_all (GTK_WIDGET (self));
}

static void
renderer_background_updated (NmnItemRenderer *item_renderer,
                             gboolean prelight)
{
    NmnGsmPinRequestRendererPrivate *priv = GET_PRIVATE (item_renderer);

    g_object_set (priv->box_to_hide, "visible", prelight, NULL);
}

static void
renderer_item_changed (NmnItemRenderer *item_renderer)
{
    init_widgets (NMN_GSM_PIN_REQUEST_RENDERER (item_renderer));
    renderer_background_updated (item_renderer, nmn_item_renderer_is_prelight (item_renderer));
    sensitize_widgets (NMN_GSM_PIN_REQUEST_RENDERER (item_renderer));
}

static void
nmn_gsm_pin_request_renderer_init (NmnGsmPinRequestRenderer *item)
{
}

static void
nmn_gsm_pin_request_renderer_class_init (NmnGsmPinRequestRendererClass *class)
{
    GObjectClass *object_class = G_OBJECT_CLASS (class);
    NmnItemRendererClass *renderer_class = NMN_ITEM_RENDERER_CLASS (class);

    g_type_class_add_private (object_class, sizeof (NmnGsmPinRequestRendererPrivate));

    renderer_class->background_updated = renderer_background_updated;
    renderer_class->item_changed = renderer_item_changed;
}
