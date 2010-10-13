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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include "nmn-applet.h"
#include "nm-status-model.h"
#include "nm-icon-cache.h"
#include "nm-status-icon.h"
#include "nmn-panel-client.h"

static void
panel_client_show (MplPanelClient *panel_client,
                   gpointer user_data)
{
    nmn_applet_show (NMN_APPLET (user_data));
}

static void
panel_client_hide (MplPanelClient *panel_client,
                   gpointer user_data)
{
    nmn_applet_hide (NMN_APPLET (user_data));
}

static void
dialog_done_cb (gpointer user_data)
{
    mpl_panel_client_request_focus (MPL_PANEL_CLIENT (user_data));
}

static void
status_icon_activated (GtkStatusIcon *status_icon,
                       gpointer user_data)
{
    gtk_widget_show (GTK_WIDGET (user_data));
}

int
main (int argc, char *argv[])
{
    NmnModel *model;
    NmnApplet *applet;
    GtkWidget *container;
    gboolean standalone = FALSE;
    gboolean desktop = FALSE;
    GError *error = NULL;
    GOptionEntry entries[] = {
        { "standalone", 's', 0, G_OPTION_ARG_NONE, &standalone, _("Run in standalone mode"), NULL },
        { "desktop", 'd', 0, G_OPTION_ARG_NONE, &desktop, _("Run in desktop mode"), NULL },
        { NULL }
    };

    bindtextdomain (GETTEXT_PACKAGE, NMNLOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);

    g_set_application_name (_("NetworkManager Netbook"));
    gtk_init_with_args (&argc, &argv, _("- NetworkManager Netbook"),
                        entries, GETTEXT_PACKAGE, &error);

    if (error) {
        g_printerr ("%s\n", error->message);
        g_error_free (error);
        return 1;
    }

    model = nmn_model_new ();
    applet = nmn_applet_new (model);

    if (standalone) {
        container = gtk_window_new (GTK_WINDOW_TOPLEVEL);
        g_signal_connect (container, "delete-event", (GCallback) gtk_main_quit, NULL);
        gtk_widget_set_size_request (container, 800, -1);
        gtk_widget_show (container);
    } else if (desktop) {
        NMStatusModel *status_model;
        GtkStatusIcon *status_icon;

        status_model = NM_STATUS_MODEL (nm_status_model_new (GTK_TREE_MODEL (model)));
        status_icon = nm_status_icon_new (status_model);
        g_object_unref (status_model);
        
        container = gtk_window_new (GTK_WINDOW_TOPLEVEL);
        g_signal_connect (status_icon, "activate", G_CALLBACK (status_icon_activated), container);
        g_signal_connect (container, "delete-event", (GCallback) gtk_widget_hide, NULL);
        gtk_widget_set_size_request (container, 800, -1);
    } else {
        NMStatusModel *status_model;
        NmnPanelClient *panel_client;

        /* Force to the moblin theme */
        gtk_settings_set_string_property (gtk_settings_get_default (),
                                          "gtk-theme-name",
                                          "Moblin-Netbook",
                                          NULL);

        status_model = NM_STATUS_MODEL (nm_status_model_new (GTK_TREE_MODEL (model)));
        panel_client = nmn_panel_client_new (status_model);
        g_object_unref (status_model);

        if (!panel_client)
            g_error ("Moblin panel client intialization failed.");

        g_signal_connect (panel_client, "show-begin", G_CALLBACK (panel_client_show), applet);
        g_signal_connect (panel_client, "show-end", G_CALLBACK (panel_client_hide), applet);
        nm_utils_set_dialog_done_cb (dialog_done_cb, panel_client);

        container = mpl_panel_gtk_get_window (MPL_PANEL_GTK (panel_client));
        gtk_widget_modify_bg (container, GTK_STATE_NORMAL, &gtk_widget_get_style (container)->white);
        gtk_widget_show (container);
    }

    gtk_widget_show (GTK_WIDGET (applet));
    gtk_container_add (GTK_CONTAINER (container), GTK_WIDGET (applet));

    gtk_main ();
    g_object_unref (model);
    nm_icon_cache_invalidate ();

    return 0;
}
