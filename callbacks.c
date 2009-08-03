#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"

#include "common.h"

#include "psdl.h"
#include "playlist.h"
#include "gtkplaylist.h"
#include "messagepump.h"
#include "messages.h"
#include "codec.h"

extern GtkWidget *mainwin;

static void
addfile_func (gpointer data, gpointer userdata) {
    ps_add_file (data);
    g_free (data);
}


void
on_volume_value_changed                (GtkRange        *range,
        gpointer         user_data)
{
    float db = -(60 - (gtk_range_get_value (range) * 0.6f));
    float a = db <= -60.f ? 0 : pow (10, db/20.f);
    psdl_set_volume (a);
}

int g_disable_seekbar_handler = 0;
void
on_playpos_value_changed               (GtkRange        *range,
        gpointer         user_data)
{
    if (g_disable_seekbar_handler) {
        return;
    }
    if (playlist_current.codec) {
        if (playlist_current.codec->info.duration > 0) {
            int val = gtk_range_get_value (range);
            int upper = gtk_adjustment_get_upper (gtk_range_get_adjustment (range));
            float time = playlist_current.codec->info.duration / (float)upper * (float)val;
            messagepump_push (M_SONGSEEK, 0, (int)time * 1000, 0);
        }
    }
}


// change properties
gboolean
on_playlist_configure_event            (GtkWidget       *widget,
        GdkEventConfigure *event,
        gpointer         user_data)
{
    gtkps_reconf (widget);
    return FALSE;
}

// redraw
gboolean
on_playlist_expose_event               (GtkWidget       *widget,
        GdkEventExpose  *event,
        gpointer         user_data)
{
    // draw visible area of playlist
    gtkps_expose (widget, event->area.x, event->area.y, event->area.width, event->area.height);

    return FALSE;
}

void
on_playlist_realize                    (GtkWidget       *widget,
        gpointer         user_data)
{
    GtkTargetEntry entry = {
        .target = "STRING",
        .flags = GTK_TARGET_SAME_WIDGET/* | GTK_TARGET_OTHER_APP*/,
        TARGET_SAMEWIDGET
    };
    // setup drag-drop source
//    gtk_drag_source_set (widget, GDK_BUTTON1_MASK, &entry, 1, GDK_ACTION_MOVE);
    // setup drag-drop target
    gtk_drag_dest_set (widget, GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_DROP, &entry, 1, GDK_ACTION_COPY | GDK_ACTION_MOVE);
    gtk_drag_dest_add_uri_targets (widget);
    gtk_drag_dest_set_track_motion (widget, TRUE);
}


gboolean
on_playlist_button_press_event         (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    if (event->button == 1) {
        gtkps_mouse1_pressed (event->state, event->x, event->y, event->time);
    }
    return FALSE;
}

gboolean
on_playlist_button_release_event       (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    if (event->button == 1) {
        gtkps_mouse1_released (event->state, event->x, event->y, event->time);
    }
    return FALSE;
}

gboolean
on_playlist_motion_notify_event        (GtkWidget       *widget,
                                        GdkEventMotion  *event,
                                        gpointer         user_data)
{
    gtkps_mousemove (event);
    return FALSE;
}


void
on_playscroll_value_changed            (GtkRange        *range,
                                        gpointer         user_data)
{
    int newscroll = gtk_range_get_value (GTK_RANGE (range));
    gtkps_scroll (newscroll);
}


void
on_open_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
}


void
on_add_files_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    GtkWidget *dlg = gtk_file_chooser_dialog_new ("Add file(s) to playlist...", GTK_WINDOW (mainwin), GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL);

    GtkFileFilter* flt;
    flt = gtk_file_filter_new ();
    gtk_file_filter_set_name (flt, "Supported music files");
    gtk_file_filter_add_pattern (flt, "*.ogg");
    gtk_file_filter_add_pattern (flt, "*.mod");
    gtk_file_filter_add_pattern (flt, "*.wav");
    gtk_file_filter_add_pattern (flt, "*.mp3");
    gtk_file_filter_add_pattern (flt, "*.nsf");
    gtk_file_filter_add_pattern (flt, "*.flac");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dlg), flt);
    gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dlg), flt);
    flt = gtk_file_filter_new ();
    gtk_file_filter_set_name (flt, "Other files (*)");
    gtk_file_filter_add_pattern (flt, "*");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dlg), flt);
    gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (dlg), TRUE);

    if (gtk_dialog_run (GTK_DIALOG (dlg)) == GTK_RESPONSE_OK)
    {
        GSList *lst = gtk_file_chooser_get_filenames (GTK_FILE_CHOOSER (dlg));
        g_slist_foreach(lst, addfile_func, NULL);
        g_slist_free (lst);
    }
    gtk_widget_destroy (dlg);
    gtkps_setup_scrollbar ();
    GtkWidget *widget = lookup_widget (mainwin, "playlist");
    draw_playlist (widget, 0, 0, widget->allocation.width, widget->allocation.height);
    gtkps_expose (widget, 0, 0, widget->allocation.width, widget->allocation.height);
}


void
on_add_folder1_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

    GtkWidget *dlg = gtk_file_chooser_dialog_new ("Add folder to playlist...", GTK_WINDOW (mainwin), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL);

    if (gtk_dialog_run (GTK_DIALOG (dlg)) == GTK_RESPONSE_OK)
    {
        gchar *folder = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dlg));
        if (folder) {
            ps_add_dir (folder);
            g_free (folder);
        }
    }
    gtk_widget_destroy (dlg);
    gtkps_setup_scrollbar ();
    GtkWidget *widget = lookup_widget (mainwin, "playlist");
    draw_playlist (widget, 0, 0, widget->allocation.width, widget->allocation.height);
    gtkps_expose (widget, 0, 0, widget->allocation.width, widget->allocation.height);
}


void
on_preferences1_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_quit1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_clear1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_select_all1_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_remove1_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_crop1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_about1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


gboolean
on_playlist_scroll_event               (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
	GdkEventScroll *ev = (GdkEventScroll*)event;
    gtkps_handle_scroll_event (ev->direction);
    return FALSE;
}


void
on_stopbtn_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
    messagepump_push (M_STOPSONG, 0, 0, 0);
}


void
on_playbtn_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
    messagepump_push (M_PLAYSONG, 0, 0, 0);
}


void
on_pausebtn_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
    messagepump_push (M_PAUSESONG, 0, 0, 0);
}


void
on_prevbtn_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
    messagepump_push (M_PREVSONG, 0, 0, 0);
}


void
on_nextbtn_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
    messagepump_push (M_NEXTSONG, 0, 0, 0);
}


void
on_playrand_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
    messagepump_push (M_PLAYRANDOM, 0, 0, 0);
}


gboolean
on_mainwin_key_press_event             (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data)
{
    gtkps_keypress (event->keyval, event->state);
    return FALSE;
}


void
on_playlist_drag_begin                 (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gpointer         user_data)
{
}

gboolean
on_playlist_drag_motion                (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        guint            time,
                                        gpointer         user_data)
{
    gtkps_track_dragdrop (y);
    return FALSE;
}


gboolean
on_playlist_drag_drop                  (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        guint            time,
                                        gpointer         user_data)
{
    if (drag_context->targets) {
        GdkAtom target_type = GDK_POINTER_TO_ATOM (g_list_nth_data (drag_context->targets, TARGET_SAMEWIDGET));
        gtk_drag_get_data (widget, drag_context, target_type, time);
        return TRUE;
    }
    return FALSE;
}


void
on_playlist_drag_data_get              (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        GtkSelectionData *selection_data,
                                        guint            target_type,
                                        guint            time,
                                        gpointer         user_data)
{
    switch (target_type) {
    case TARGET_SAMEWIDGET:
        {
            // format as "STRING" consisting of array of pointers
            int nsel = ps_getselcount ();
            if (!nsel) {
                break; // something wrong happened
            }
            uint32_t *ptr = malloc (nsel * sizeof (uint32_t));
            int idx = 0;
            int i = 0;
            for (playItem_t *it = playlist_head; it; it = it->next, idx++) {
                if (it->selected) {
                    ptr[i] = idx;
                    i++;
                }
            }
            gtk_selection_data_set (selection_data, selection_data->target, sizeof (uint32_t) * 8, (gchar *)ptr, nsel * sizeof (uint32_t));
            free (ptr);
        }
        break;
    default:
        g_assert_not_reached ();
    }
}


void
on_playlist_drag_data_received         (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        GtkSelectionData *data,
                                        guint            target_type,
                                        guint            time,
                                        gpointer         user_data)
{
    gchar *ptr=(char*)data->data;
//    printf ("target type: %d\n", target_type);
    if (target_type == 0) { // uris
        if (!strncmp(ptr,"file:///",8) && (strlen(ptr)<=4096)) {
            // this happens when dropped from file manager
//            printf ("%s\n", ptr);
        }
    }
    else if (target_type == 1) {
        uint32_t *d= (uint32_t *)ptr;
        gtkps_handle_drag_drop (y, d, data->length/4);
    }
    gtk_drag_finish (drag_context, FALSE, FALSE, time);
}


void
on_playlist_drag_data_delete           (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gpointer         user_data)
{
}

void
on_playlist_drag_end                   (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gpointer         user_data)
{
    draw_playlist (widget, 0, 0, widget->allocation.width, widget->allocation.height);
    gtkps_expose (widget, 0, 0, widget->allocation.width, widget->allocation.height);
}


gboolean
on_playlist_drag_failed                (GtkWidget       *widget,
                                        GdkDragContext  *arg1,
                                        GtkDragResult    arg2,
                                        gpointer         user_data)
{
    return FALSE;
}


void
on_playlist_drag_leave                 (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        guint            time,
                                        gpointer         user_data)
{
    gtkps_track_dragdrop (-1);
}

void
on_voice1_clicked                      (GtkButton       *button,
                                        gpointer         user_data)
{
    codec_lock ();
    if (playlist_current.codec && playlist_current.codec->mutevoice) {
        playlist_current.codec->mutevoice (0, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)) ? 0 : 1);
    }
    codec_unlock ();
}


void
on_voice2_clicked                      (GtkButton       *button,
                                        gpointer         user_data)
{
    codec_lock ();
    if (playlist_current.codec && playlist_current.codec->mutevoice) {
        playlist_current.codec->mutevoice (1, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)) ? 0 : 1);
    }
    codec_unlock ();
}


void
on_voice3_clicked                      (GtkButton       *button,
                                        gpointer         user_data)
{
    codec_lock ();
    if (playlist_current.codec && playlist_current.codec->mutevoice) {
        playlist_current.codec->mutevoice (2, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)) ? 0 : 1);
    }
    codec_unlock ();
}


void
on_voice4_clicked                      (GtkButton       *button,
                                        gpointer         user_data)
{
    codec_lock ();
    if (playlist_current.codec && playlist_current.codec->mutevoice) {
        playlist_current.codec->mutevoice (3, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)) ? 0 : 1);
    }
    codec_unlock ();
}


void
on_voice5_clicked                      (GtkButton       *button,
                                        gpointer         user_data)
{
    codec_lock ();
    if (playlist_current.codec && playlist_current.codec->mutevoice) {
        playlist_current.codec->mutevoice (4, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)) ? 0 : 1);
    }
    codec_unlock ();
}

