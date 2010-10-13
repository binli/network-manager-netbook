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

#include "nm-list-item.h"

G_DEFINE_TYPE (NMListItem, nm_list_item, G_TYPE_INITIALLY_UNOWNED)

enum {
    PROP_0,
    PROP_NAME,
    PROP_TYPE_NAME,
    PROP_ICON,
    PROP_SECURITY,
    PROP_STATUS,
    PROP_SHOW_DELETE,

    LAST_PROP
};

enum {
	REQUEST_REMOVE,
    WARNING,

	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), NM_TYPE_LIST_ITEM, NMListItemPrivate))

typedef struct {
    char *name;
    char *type_name;
    char *security;
    char *icon;
    NMListItemStatus status;
    gboolean show_delete;

    gboolean disposed;
} NMListItemPrivate;

const char *
nm_list_item_get_name (NMListItem *self)
{
    g_return_val_if_fail (NM_IS_LIST_ITEM (self), NULL);

    return GET_PRIVATE (self)->name;
}

const char *
nm_list_item_get_type_name (NMListItem *self)
{
    g_return_val_if_fail (NM_IS_LIST_ITEM (self), NULL);

    return GET_PRIVATE (self)->type_name;
}

const char *
nm_list_item_get_icon (NMListItem *self)
{
    g_return_val_if_fail (NM_IS_LIST_ITEM (self), NULL);

    return GET_PRIVATE (self)->icon;
}

const char *
nm_list_item_get_security (NMListItem *self)
{
    g_return_val_if_fail (NM_IS_LIST_ITEM (self), NULL);

    return GET_PRIVATE (self)->security;
}

NMListItemStatus
nm_list_item_get_status (NMListItem *self)
{
    g_return_val_if_fail (NM_IS_LIST_ITEM (self), NM_LIST_ITEM_STATUS_DISCONNECTED);

    return GET_PRIVATE (self)->status;
}

gboolean
nm_list_item_get_show_delete (NMListItem *self)
{
    g_return_val_if_fail (NM_IS_LIST_ITEM (self), FALSE);

    return GET_PRIVATE (self)->show_delete;
}

void
nm_list_item_request_remove (NMListItem *self)
{
    g_return_if_fail (NM_IS_LIST_ITEM (self));

    g_signal_emit (self, signals[REQUEST_REMOVE], 0);
}

void
nm_list_item_connect (NMListItem *self)
{
    g_return_if_fail (NM_IS_LIST_ITEM (self));

    if (NM_LIST_ITEM_GET_CLASS (self)->connect)
        NM_LIST_ITEM_GET_CLASS (self)->connect (self);
}

void
nm_list_item_disconnect (NMListItem *self)
{
    g_return_if_fail (NM_IS_LIST_ITEM (self));

    if (NM_LIST_ITEM_GET_CLASS (self)->disconnect)
        NM_LIST_ITEM_GET_CLASS (self)->disconnect (self);
}

void
nm_list_item_delete (NMListItem *self)
{
    g_return_if_fail (NM_IS_LIST_ITEM (self));

    if (NM_LIST_ITEM_GET_CLASS (self)->delete)
        NM_LIST_ITEM_GET_CLASS (self)->delete (self);
}

static int
priority (NMListItem *self)
{
    return 0;
}

static int
nm_list_item_get_priority (NMListItem *item)
{
    return NM_LIST_ITEM_GET_CLASS (item)->priority (item);
}

int
nm_list_item_compare (NMListItem *self,
                      NMListItem *other)
{
    int priority;
    int other_priority;

    g_return_val_if_fail (NM_IS_LIST_ITEM (self), 1);
    g_return_val_if_fail (NM_IS_LIST_ITEM (other), 1);

    priority = nm_list_item_get_priority (self);
    other_priority = nm_list_item_get_priority (other);

    if (priority > other_priority)
        return -1;
    else if (priority < other_priority)
        return 1;

    return g_strcmp0 (nm_list_item_get_name (self), nm_list_item_get_name (other));
}

void
nm_list_item_warning (NMListItem *self,
                      const char *message)
{
    g_return_if_fail (NM_IS_LIST_ITEM (self));
    g_return_if_fail (message != NULL);

    g_signal_emit (self, signals[WARNING], 0, message);
}

/*****************************************************************************/

static void
nm_list_item_init (NMListItem *self)
{
}

static void
set_property (GObject *object, guint prop_id,
              const GValue *value, GParamSpec *pspec)
{
    NMListItemPrivate *priv = GET_PRIVATE (object);

    switch (prop_id) {
    case PROP_NAME:
        g_free (priv->name);
        priv->name = g_value_dup_string (value);
        g_object_notify (object, NM_LIST_ITEM_NAME);
        break;
    case PROP_TYPE_NAME:
        g_free (priv->type_name);
        priv->type_name = g_value_dup_string (value);
        g_object_notify (object, NM_LIST_ITEM_TYPE_NAME);
        break;
    case PROP_ICON:
        g_free (priv->icon);
        priv->icon = g_value_dup_string (value);
        g_object_notify (object, NM_LIST_ITEM_ICON);
        break;
    case PROP_SECURITY:
        g_free (priv->security);
        priv->security = g_value_dup_string (value);
        g_object_notify (object, NM_LIST_ITEM_SECURITY);
        break;
    case PROP_STATUS:
        priv->status = g_value_get_int (value);
        g_object_notify (object, NM_LIST_ITEM_STATUS);
        break;
    case PROP_SHOW_DELETE:
        priv->show_delete = g_value_get_boolean (value);
        g_object_notify (object, NM_LIST_ITEM_SHOW_DELETE);
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
    NMListItem *self = NM_LIST_ITEM (object);

    switch (prop_id) {
    case PROP_NAME:
        g_value_set_string (value, nm_list_item_get_name (self));
        break;
    case PROP_TYPE_NAME:
        g_value_set_string (value, nm_list_item_get_type_name (self));
        break;
    case PROP_ICON:
        g_value_set_string (value, nm_list_item_get_icon (self));
        break;
    case PROP_SECURITY:
        g_value_set_string (value, nm_list_item_get_security (self));
        break;
    case PROP_STATUS:
        g_value_set_int (value, nm_list_item_get_status (self));
        break;
    case PROP_SHOW_DELETE:
        g_value_set_boolean (value, nm_list_item_get_show_delete (self));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
dispose (GObject *object)
{
    NMListItemPrivate *priv = GET_PRIVATE (object);

    if (!priv->disposed) {
        g_free (priv->name);
        g_free (priv->type_name);
        g_free (priv->icon);
        g_free (priv->security);

        priv->disposed = TRUE;
    }

    G_OBJECT_CLASS (nm_list_item_parent_class)->dispose (object);
}

static void
nm_list_item_class_init (NMListItemClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (NMListItemPrivate));

    object_class->set_property = set_property;
    object_class->get_property = get_property;
    object_class->dispose = dispose;

    klass->priority = priority;

    /* Properties */
    g_object_class_install_property
        (object_class, PROP_NAME,
         g_param_spec_string (NM_LIST_ITEM_NAME,
                              "Name",
                              "Name",
                              NULL,
                              G_PARAM_READWRITE));

    g_object_class_install_property
        (object_class, PROP_TYPE_NAME,
         g_param_spec_string (NM_LIST_ITEM_TYPE_NAME,
                              "TypeName",
                              "TypeName",
                              NULL,
                              G_PARAM_READWRITE));

    g_object_class_install_property
        (object_class, PROP_ICON,
         g_param_spec_string (NM_LIST_ITEM_ICON,
                              "Icon",
                              "Icon",
                              NULL,
                              G_PARAM_READWRITE));

    g_object_class_install_property
        (object_class, PROP_SECURITY,
         g_param_spec_string (NM_LIST_ITEM_SECURITY,
                              "Security",
                              "Security",
                              NULL,
                              G_PARAM_READWRITE));

    g_object_class_install_property
        (object_class, PROP_STATUS,
         g_param_spec_int (NM_LIST_ITEM_STATUS,
                           "Status",
                           "Status",
                           NM_LIST_ITEM_STATUS_DISCONNECTED,
                           NM_LIST_ITEM_STATUS_CONNECTED,
                           NM_LIST_ITEM_STATUS_DISCONNECTED,
                           G_PARAM_READWRITE));

    g_object_class_install_property
        (object_class, PROP_SHOW_DELETE,
         g_param_spec_boolean (NM_LIST_ITEM_SHOW_DELETE,
                               "Show delete",
                               "Show delete button",
                               FALSE,
                               G_PARAM_READWRITE));

    /* Signals */
    signals[REQUEST_REMOVE] =
		g_signal_new ("request-remove",
					  G_OBJECT_CLASS_TYPE (object_class),
					  G_SIGNAL_RUN_FIRST,
					  G_STRUCT_OFFSET (NMListItemClass, request_remove),
					  NULL, NULL,
					  g_cclosure_marshal_VOID__VOID,
					  G_TYPE_NONE, 0);

    signals[WARNING] =
		g_signal_new ("warning",
					  G_OBJECT_CLASS_TYPE (object_class),
					  G_SIGNAL_RUN_FIRST,
					  G_STRUCT_OFFSET (NMListItemClass, warning),
					  NULL, NULL,
					  g_cclosure_marshal_VOID__STRING,
					  G_TYPE_NONE, 1,
                      G_TYPE_STRING);
}
