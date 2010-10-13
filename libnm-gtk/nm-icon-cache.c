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

#include <gtk/gtk.h>
#include "nm-icon-cache.h"

static GtkIconTheme *icon_theme = NULL;
static GHashTable *cache = NULL;

void
nm_icon_cache_invalidate (void)
{
    if (cache) {
        g_hash_table_destroy (cache);
        cache = NULL;
    }

    if (icon_theme)
        icon_theme = NULL;
}

static void
init_icon_theme (void)
{
    char **path = NULL;
    int n_path;
    int i;

    icon_theme = gtk_icon_theme_get_default ();
    g_signal_connect (icon_theme, "changed", G_CALLBACK (nm_icon_cache_invalidate), NULL);

    gtk_icon_theme_get_search_path (icon_theme, &path, &n_path);
    for (i = n_path - 1; i >= 0; i--) {
        if (g_strcmp0 (ICONDIR, path[i]) == 0)
            break;
    }

    if (i < 0)
        gtk_icon_theme_append_search_path (icon_theme, ICONDIR);

    g_strfreev (path);
}

GdkPixbuf *
nm_icon_cache_get (const char *icon_name)
{
    GdkPixbuf *pixbuf;
    GError *error = NULL;

    g_return_val_if_fail (icon_name != NULL, NULL);

    if (G_UNLIKELY (icon_theme == NULL))
        init_icon_theme ();

    if (G_UNLIKELY (cache == NULL)) {
        cache = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);
        pixbuf = NULL;
    } else
        pixbuf = (GdkPixbuf *) g_hash_table_lookup (cache, icon_name);

    if (!pixbuf) {
        pixbuf = gtk_icon_theme_load_icon (icon_theme, icon_name, 48, 0, &error);

        if (!pixbuf) {
            /* Try harder, using our own icons if theme doesn't provide something */
            char *path;

            path = g_strconcat (ICONDIR, icon_name, ".png", NULL);
            pixbuf = gdk_pixbuf_new_from_file (path, NULL);
            g_free (path);
        }

        if (pixbuf)
            g_hash_table_insert (cache, g_strdup (icon_name), pixbuf);
    }

    if (error) {
        if (!pixbuf)
            g_warning ("Error loading icon '%s': %s", icon_name, error->message);

        g_error_free (error);
    }

    return pixbuf;
}
