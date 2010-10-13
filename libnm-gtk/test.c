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
#include <nm-client.h>
#include <nm-remote-settings-system.h>
#include "nm-list-model.h"
#include "nm-device-model.h"
#include "nm-status-icon.h"
#include "nm-connection-model.h"
#include "nm-gconf-settings.h"

static void
setup_status_icon (NMListModel *model)
{
    NMStatusModel *status_model;
    GtkStatusIcon *status_icon;

    status_model = NM_STATUS_MODEL (nm_status_model_new (GTK_TREE_MODEL (model)));
    status_icon = nm_status_icon_new (status_model);
    g_object_unref (status_model);
}

static GtkWidget *
create_device_combo (void)
{
    GtkWidget *combo;
    GtkCellRenderer *renderer;

    combo = gtk_combo_box_new ();

    gtk_cell_layout_clear (GTK_CELL_LAYOUT (combo));

    renderer = gtk_cell_renderer_pixbuf_new ();
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, TRUE);
    gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (combo), renderer,
                                   "pixbuf", NM_DEVICE_MODEL_COL_ICON);

    renderer = gtk_cell_renderer_text_new ();
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, TRUE);
    gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (combo), renderer,
                                   "text", NM_DEVICE_MODEL_COL_IFACE);

    renderer = gtk_cell_renderer_text_new ();
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, TRUE);
    gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (combo), renderer,
                                   "text", NM_DEVICE_MODEL_COL_DESCRIPTION);

    return combo;
}

static GtkWidget *
create_connection_tree (void)
{
    GtkWidget *tree;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    
    tree = gtk_tree_view_new ();

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Name",
                                                       renderer,
                                                       "text", NM_CONNECTION_MODEL_COL_NAME,
                                                       NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Type",
                                                       renderer,
                                                       "text", NM_CONNECTION_MODEL_COL_TYPE,
                                                       NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

    return tree;
}

static GtkWidget *
create_list_tree (void)
{
    GtkWidget *tree;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    
    tree = gtk_tree_view_new ();

    renderer = gtk_cell_renderer_pixbuf_new ();
    column = gtk_tree_view_column_new_with_attributes ("Icon",
                                                       renderer,
                                                       "icon-name", NM_LIST_MODEL_COL_ICON,
                                                       NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Name",
                                                       renderer,
                                                       "text", NM_LIST_MODEL_COL_NAME,
                                                       NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Security",
                                                       renderer,
                                                       "text", NM_LIST_MODEL_COL_SECURITY,
                                                       NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Status",
                                                       renderer,
                                                       "text", NM_LIST_MODEL_COL_STATUS,
                                                       NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

    renderer = gtk_cell_renderer_toggle_new ();
    column = gtk_tree_view_column_new_with_attributes ("Show delete",
                                                       renderer,
                                                       "active", NM_LIST_MODEL_COL_SHOW_DELETE,
                                                       NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

    return tree;
}

static void
do_stuff (void)
{
    NMClient *client;
    DBusGConnection *bus;
    NMSettingsInterface *user_settings;
    NMSettingsInterface *system_settings;
    GtkTreeModel *model;
    GtkWidget *tree;
    GtkWidget *scrolled;
    GtkWidget *window;
    GtkWidget *box;

    /* Window setup */
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size (GTK_WINDOW (window), 400, 400);
    g_signal_connect (window, "delete-event", G_CALLBACK (gtk_main_quit), NULL);
    box = gtk_vbox_new (FALSE, 6);
    gtk_container_add (GTK_CONTAINER (window), box);

    client = nm_client_new ();
    bus = nm_object_get_connection (NM_OBJECT (client));
    system_settings = (NMSettingsInterface *) nm_remote_settings_system_new (bus);
    user_settings = (NMSettingsInterface *) nm_gconf_settings_new (bus);

    /* Device model/tree */
    model = GTK_TREE_MODEL (nm_device_model_new (client));
    tree = create_device_combo ();
    gtk_combo_box_set_model (GTK_COMBO_BOX (tree), model);
    g_object_unref (model);
    gtk_combo_box_set_active (GTK_COMBO_BOX (tree), 0);
    gtk_box_pack_start (GTK_BOX (box), tree, FALSE, TRUE, 0);

    /* Connection model/tree */
    model = nm_connection_model_new ();
    nm_connection_model_add_settings (NM_CONNECTION_MODEL (model), user_settings);
    nm_connection_model_add_settings (NM_CONNECTION_MODEL (model), system_settings);

    tree = create_connection_tree ();
    gtk_tree_view_set_model (GTK_TREE_VIEW (tree), model);
    g_object_unref (model);
    scrolled = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
                                    GTK_POLICY_AUTOMATIC,
                                    GTK_POLICY_AUTOMATIC);

    gtk_container_add (GTK_CONTAINER (scrolled), tree);
    gtk_box_pack_start (GTK_BOX (box), scrolled, TRUE, TRUE, 0);

    /* List model/tree */
    model = GTK_TREE_MODEL (nm_list_model_new (client));
    nm_list_model_add_settings (NM_LIST_MODEL (model), user_settings);
    nm_list_model_add_settings (NM_LIST_MODEL (model), system_settings);

    tree = create_list_tree ();
    gtk_tree_view_set_model (GTK_TREE_VIEW (tree), model);
    setup_status_icon (NM_LIST_MODEL (model));
    g_object_unref (model);

    scrolled = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
                                    GTK_POLICY_AUTOMATIC,
                                    GTK_POLICY_AUTOMATIC);

    gtk_container_add (GTK_CONTAINER (scrolled), tree);
    gtk_box_pack_start (GTK_BOX (box), scrolled, TRUE, TRUE, 0);

    gtk_widget_show_all (window);

    g_object_unref (user_settings);
    g_object_unref (system_settings);
    g_object_unref (client);
}

int
main (int argc, char *argv[])
{
    gtk_init (&argc, &argv);

    do_stuff ();
    gtk_main ();

    return 0;
}
