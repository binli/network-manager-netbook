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
#include "nmn-item-renderer.h"
#include "nm-list-item.h"
#include "nm-icon-cache.h"
#include "nm-connection-item.h"
#include "nm-device-item.h"
#include "nmn-connection-details.h"
#include "gtkinfobar.h"
#include "utils.h"

G_DEFINE_TYPE (NmnItemRenderer, nmn_item_renderer, GTK_TYPE_EVENT_BOX)

enum {
    PROP_0,
    PROP_ITEM,

    LAST_PROP
};

enum {
    BACKGROUND_UPDATED,
    ITEM_CHANGED,

    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), NMN_TYPE_ITEM_RENDERER, NmnItemRendererPrivate))

typedef struct {
    NMListItem *item;

    GtkWidget *content_area;
    GtkWidget *info_bar;
    GtkWidget *info_label;

    gboolean prelight;
    GdkColor prelight_color;
    GdkColor connection_item_color;
} NmnItemRendererPrivate;

static void
update_background (NmnItemRenderer *self)
{
    NmnItemRendererPrivate *priv = GET_PRIVATE (self);
    GdkColor *color;

    if (priv->prelight) {
        color = &priv->prelight_color;
    } else {
        if (NM_IS_CONNECTION_ITEM (priv->item) && nm_connection_item_get_connection (NM_CONNECTION_ITEM (priv->item)))
            color = &priv->connection_item_color;
        else
            color = NULL;
    }

    gtk_widget_modify_bg (GTK_WIDGET (self), GTK_STATE_NORMAL, color);
    g_signal_emit (self, signals[BACKGROUND_UPDATED], 0, priv->prelight);
}

NMListItem *
nmn_item_renderer_get_item (NmnItemRenderer *self)
{
    g_return_val_if_fail (NMN_IS_ITEM_RENDERER (self), NULL);

    return GET_PRIVATE (self)->item;
}

static void
item_warning (NMListItem *item,
              const char *message,
              gpointer user_data)
{
    nmn_item_renderer_show_error (NMN_ITEM_RENDERER (user_data), message);
}

void
nmn_item_renderer_set_item (NmnItemRenderer *self,
                            NMListItem *item)
{
    NmnItemRendererPrivate *priv;

    g_return_if_fail (NMN_IS_ITEM_RENDERER (self));
    g_return_if_fail (NM_IS_LIST_ITEM (item));

    priv = GET_PRIVATE (self);
    g_return_if_fail (priv->item == NULL);

    priv->item = g_object_ref (item);
    g_signal_connect (item, "warning", G_CALLBACK (item_warning), self);

    g_signal_emit (self, signals[ITEM_CHANGED], 0);
}

gboolean
nmn_item_renderer_is_prelight (NmnItemRenderer *self)
{
    g_return_val_if_fail (NMN_IS_ITEM_RENDERER (self), FALSE);

    return GET_PRIVATE (self)->prelight;
}

GtkWidget *
nmn_item_renderer_get_content_area (NmnItemRenderer *self)
{
    g_return_val_if_fail (NMN_IS_ITEM_RENDERER (self), NULL);

    return GET_PRIVATE (self)->content_area;
}

void
nmn_item_renderer_show_error (NmnItemRenderer *self,
                              const char *message)
{
    NmnItemRendererPrivate *priv = GET_PRIVATE (self);

    g_return_if_fail (NMN_IS_ITEM_RENDERER (self));
    g_return_if_fail (message != NULL);

    gtk_label_set_text (GTK_LABEL (priv->info_label), message);
    gtk_widget_show (priv->info_bar);
}

void
nmn_item_renderer_hide_error (NmnItemRenderer *self)
{
    g_return_if_fail (NMN_IS_ITEM_RENDERER (self));

    gtk_widget_hide (GET_PRIVATE (self)->info_bar);
}

static gboolean
enter_notify_event (GtkWidget *widget,
                    GdkEventCrossing *event)
{
    NmnItemRendererPrivate *priv = GET_PRIVATE (widget);

    priv->prelight = TRUE;
    update_background (NMN_ITEM_RENDERER (widget));

    return TRUE;
}

static gboolean
leave_notify_event (GtkWidget *widget,
                    GdkEventCrossing *event)
{
    NmnItemRendererPrivate *priv = GET_PRIVATE (widget);

    if (event->detail != GDK_NOTIFY_INFERIOR) {
        priv->prelight = FALSE;
        update_background (NMN_ITEM_RENDERER (widget));
    }

    return TRUE;
}

static void
nmn_item_renderer_init (NmnItemRenderer *item)
{
    NmnItemRendererPrivate *priv = GET_PRIVATE (item);
    GtkWidget *w;

    gdk_color_parse ("#cbcbcb", &priv->prelight_color);
    gdk_color_parse ("#e8e8e8", &priv->connection_item_color);

    priv->content_area = gtk_vbox_new (FALSE, 6);
    gtk_container_add (GTK_CONTAINER (item), priv->content_area);

    priv->info_bar = egg_info_bar_new ();
    gtk_widget_set_no_show_all (priv->info_bar, TRUE);
    gtk_box_pack_end (GTK_BOX (priv->content_area), priv->info_bar, FALSE, FALSE, 0);

    priv->info_label = gtk_label_new (NULL);
    w = egg_info_bar_get_content_area (EGG_INFO_BAR (priv->info_bar));
    gtk_container_add (GTK_CONTAINER (w), priv->info_label);
    gtk_widget_show (priv->info_label);
}

static void
dispose (GObject *object)
{
    NmnItemRendererPrivate *priv = GET_PRIVATE (object);

    if (priv->item) {
        g_object_unref (priv->item);
        priv->item = NULL;
    }

    G_OBJECT_CLASS (nmn_item_renderer_parent_class)->dispose (object);
}

static void
nmn_item_renderer_class_init (NmnItemRendererClass *class)
{
    GObjectClass *object_class = G_OBJECT_CLASS (class);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

    g_type_class_add_private (object_class, sizeof (NmnItemRendererPrivate));

    object_class->dispose = dispose;

    widget_class->enter_notify_event = enter_notify_event;
    widget_class->leave_notify_event = leave_notify_event;

    /* signals */
    signals[BACKGROUND_UPDATED] = g_signal_new
        ("background-updated",
         G_OBJECT_CLASS_TYPE (class),
         G_SIGNAL_RUN_LAST,
         G_STRUCT_OFFSET (NmnItemRendererClass, background_updated),
         NULL, NULL,
         g_cclosure_marshal_VOID__BOOLEAN,
         G_TYPE_NONE, 1,
         G_TYPE_BOOLEAN);

    signals[ITEM_CHANGED] = g_signal_new
        ("item-changed",
         G_OBJECT_CLASS_TYPE (class),
         G_SIGNAL_RUN_LAST,
         G_STRUCT_OFFSET (NmnItemRendererClass, item_changed),
         NULL, NULL,
         g_cclosure_marshal_VOID__VOID,
         G_TYPE_NONE, 0);
}
