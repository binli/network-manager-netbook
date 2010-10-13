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

#include "nm-status-icon.h"
#include "nm-list-item.h"
#include "nm-device-item.h"
#include "nm-icon-cache.h"

#define ACTIVATION_STEPS 6

G_DEFINE_TYPE (NMStatusIcon, nm_status_icon, GTK_TYPE_STATUS_ICON)

enum {
    PROP_0,
    PROP_MODEL,

    LAST_PROP
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), NM_TYPE_STATUS_ICON, NMStatusIconPrivate))

typedef struct {
    NMStatusModel *model;
    NMListItem *item;

    guint animation_id;
    guint animation_step;

    gboolean disposed;
} NMStatusIconPrivate;

GtkStatusIcon *
nm_status_icon_new (NMStatusModel *model)
{
    g_return_val_if_fail (NM_IS_STATUS_MODEL (model), NULL);

    return (GtkStatusIcon *) g_object_new (NM_TYPE_STATUS_ICON,
                                           NM_STATUS_ICON_MODEL, model,
                                           NULL);
}

static gboolean
activation_animation (gpointer data)
{
    NMStatusIconPrivate *priv = GET_PRIVATE (data);
    char *image;
    GdkPixbuf *pixbuf;
    int stage;

    if (++priv->animation_step > ACTIVATION_STEPS)
        priv->animation_step = 1;

    if (NM_IS_DEVICE_ITEM (priv->item)) {
        NMDeviceState state;
        
        state = nm_device_get_state (nm_device_item_get_device (NM_DEVICE_ITEM (priv->item)));
        switch (state) {
        case NM_DEVICE_STATE_PREPARE:
            stage = 1;
            break;
        case NM_DEVICE_STATE_CONFIG:
        case NM_DEVICE_STATE_NEED_AUTH:
            stage = 2;
            break;
        case NM_DEVICE_STATE_IP_CONFIG:
            stage = 3;
            break;
        default:
            stage = 1;
            break;
        }
    } else
        stage = 1;

    image = g_strdup_printf ("nm-stage%02d-connecting%04d", stage, priv->animation_step);
    pixbuf = nm_icon_cache_get (image);
    g_free (image);

    if (pixbuf)
        gtk_status_icon_set_from_pixbuf (GTK_STATUS_ICON (data), pixbuf);

    return TRUE;
}

static void
item_status_changed (NMListItem *item,
                     GParamSpec *spec,
                     gpointer user_data)
{
    NMStatusIconPrivate *priv = GET_PRIVATE (user_data);
    GdkPixbuf *pixbuf;

    if (priv->animation_id) {
        g_source_remove (priv->animation_id);
        priv->animation_id = 0;
    }

    switch (nm_list_item_get_status (item)) {
    case NM_LIST_ITEM_STATUS_CONNECTING:
        priv->animation_id = g_timeout_add (200, activation_animation, user_data);
        activation_animation (user_data);
        pixbuf = NULL;
        break;
    default:
        pixbuf = nm_icon_cache_get (nm_list_item_get_icon (item));
    }

    if (pixbuf)
        gtk_status_icon_set_from_pixbuf (GTK_STATUS_ICON (user_data), pixbuf);
}

static void
model_changed (NMStatusModel *model,
               NMListItem *active_item,
               gpointer user_data)
{
    NMStatusIcon *self = NM_STATUS_ICON (user_data);
    NMStatusIconPrivate *priv = GET_PRIVATE (self);

    if (active_item == priv->item)
        return;

    if (priv->item)
        g_signal_handlers_disconnect_by_func (priv->item, item_status_changed, self);

    priv->item = active_item;

    if (active_item) {
        g_signal_connect (active_item, "notify::" NM_LIST_ITEM_STATUS, G_CALLBACK (item_status_changed), self);
        g_signal_connect (active_item, "notify::" NM_LIST_ITEM_ICON, G_CALLBACK (item_status_changed), self);
        item_status_changed (active_item, NULL, self);
    } else {
        GdkPixbuf *pixbuf;

        pixbuf = nm_icon_cache_get ("nm-no-connection");
        if (pixbuf)
            gtk_status_icon_set_from_pixbuf (GTK_STATUS_ICON (self), pixbuf);
    }
}

/*****************************************************************************/

static void
nm_status_icon_init (NMStatusIcon *self)
{
}

static void
set_property (GObject *object, guint prop_id,
              const GValue *value, GParamSpec *pspec)
{
    NMStatusIconPrivate *priv = GET_PRIVATE (object);

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
    NMStatusIconPrivate *priv = GET_PRIVATE (object);

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
constructed (GObject *object)
{
    NMStatusIconPrivate *priv = GET_PRIVATE (object);

    if (G_OBJECT_CLASS (nm_status_icon_parent_class)->constructed)
        G_OBJECT_CLASS (nm_status_icon_parent_class)->constructed (object);

    g_signal_connect (priv->model, "changed", G_CALLBACK (model_changed), object);
}

static void
dispose (GObject *object)
{
    NMStatusIconPrivate *priv = GET_PRIVATE (object);

    if (!priv->disposed) {
        if (priv->animation_id)
            g_source_remove (priv->animation_id);

        if (priv->model)
            g_object_unref (priv->model);

        priv->disposed = TRUE;
    }

    G_OBJECT_CLASS (nm_status_icon_parent_class)->dispose (object);
}

static void
nm_status_icon_class_init (NMStatusIconClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (NMStatusIconPrivate));

    object_class->set_property = set_property;
    object_class->get_property = get_property;
    object_class->constructed = constructed;
    object_class->dispose = dispose;

    /* Properties */
    g_object_class_install_property
        (object_class, PROP_MODEL,
         g_param_spec_object (NM_STATUS_ICON_MODEL,
                              "NMStatusModel",
                              "NMStatusModel",
                              NM_TYPE_STATUS_MODEL,
                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}
