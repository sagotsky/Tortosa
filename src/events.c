#include <gtk/gtk.h>
#include "events.h"
#include "backbone.h"
#include "window.h"
#include "gears.h"
#include "tabs.h"
#include "menus.h"
#include "dbg.h"
#include "tgregex.h"

/* key event handler */
gboolean event_key_press(GtkWidget *widget, GdkEventKey *event, backbone_t * backbone) 
{
	guint(g) = event->keyval;
  if ((event->state & (GDK_CONTROL_MASK|GDK_SHIFT_MASK)) == (GDK_CONTROL_MASK|GDK_SHIFT_MASK)) 
	{
    if (g == GDK_KEY_T) {
			backbone->time = event->time;
      new_tab(backbone);
      return TRUE;
    }
    if (g == GDK_KEY_C) {
			backbone->time = event->time;
      close_tab(gtk_window_get_focus(GTK_WINDOW(widget)), backbone);
      return TRUE;
    }  
	  if (g == GDK_KEY_Right) {
		 backbone->time = event->time;
      go_to_next_tab(backbone);
      return TRUE;
    }
		if (g == GDK_KEY_Left) {
		 backbone->time = event->time;
      go_to_prev_tab(backbone);
      return TRUE;
    }
		if (g == GDK_KEY_R) {
		 //backbone->time = event->time;
		 //display_resize_menu(event->time, backbone);
     reload_tortosa_configuration(backbone);
		 return TRUE;
    }
		if (g == GDK_KEY_G) {
		 backbone->time = event->time;
		 grab_move_window(backbone);
     return TRUE;
    }
		if (g == GDK_KEY_F) {
		 backbone->time = event->time;
		 toggle_fullscreen(backbone);
     return TRUE;
    }
		if (g == GDK_KEY_Down) {
		 backbone->time = event->time;
		 toggle_maximize(backbone);
     return TRUE;
    }
		if (g == GDK_KEY_Z) {
		 backbone->time = event->time;
		 reload_css_theme(backbone);
     return TRUE;
    }
		if (g == GDK_KEY_Y) {
		 backbone->time = event->time;
		 reload_vte_configuration(backbone);
     return TRUE;
    }	
			if (g == GDK_KEY_M) {
		 backbone->time = event->time;
		 display_main_menu(event->time, backbone, NULL, 666);
     return TRUE;
		}
			if (g == GDK_KEY_Q) {
		 quit_gracefully(backbone);
     return TRUE;
		}
}
	/*nothing handled*/
	return FALSE;
}

/* button mouse event handler*/
gboolean event_button_press(GtkWidget *widget, GdkEventButton *event, backbone_t * backbone)
{
	/*click with the right mouse button */
  if (event->type == GDK_BUTTON_PRESS  &&  event->button == 3)
	{
		backbone->time = event->time;
		gchar * match = NULL;
		int flavor;
		//match = get_regex_match_on_button_press(widget, event, &flavor, backbone);
		match = get_regex_match_for_tab_on_button_press(widget, event, &flavor, backbone);
		if (match)
		{
			/*register current flavor and match in widget tab_data*/
			GSList *found = NULL;
			found = g_slist_find_custom(backbone->tabs_data, widget, (GCompareFunc) find_node_by_widget);
			if (found)
			{
				((tab_data_t*) found->data)->current_flavor = flavor;
				((tab_data_t*) found->data)->current_match = match;
			}
		}

		display_main_menu(event->time, backbone, match, flavor);
		return TRUE;
	}
	/*nothing handled*/
	return FALSE;
}

gboolean get_window_state(GtkWidget * widget, GdkEventWindowState *event, backbone_t * backbone)
{
	if ( event->new_window_state &GDK_WINDOW_STATE_WITHDRAWN)
	{
		backbone->state.withdrawn = TRUE;
		//SENTINEL("withdraw\n");
	}
	else
		backbone->state.withdrawn = FALSE;
	
	if ( event->new_window_state &GDK_WINDOW_STATE_ICONIFIED)
	{
		backbone->state.iconified = TRUE;
		//SENTINEL("iconified\n");
	}
	else
		backbone->state.iconified = FALSE;
	
	if ( event->new_window_state &GDK_WINDOW_STATE_MAXIMIZED)
	{
		backbone->state.maximized = TRUE;
		//SENTINEL("maximized\n");
	}
	else
		backbone->state.maximized = FALSE;
	
	if ( event->new_window_state &GDK_WINDOW_STATE_STICKY)
	{
		backbone->state.sticky = TRUE;
		//SENTINEL("sticky\n");
	}
	else
		backbone->state.sticky = FALSE;
	
	if ( event->new_window_state &GDK_WINDOW_STATE_FULLSCREEN)
	{
		backbone->state.fullscreen = TRUE;
		//SENTINEL("fullscreen\n");
	}
	else
		backbone->state.fullscreen = FALSE;
	
	if ( event->new_window_state &GDK_WINDOW_STATE_ABOVE)
	{
		backbone->state.above = TRUE;
		//SENTINEL("above\n");
	}
	else
		backbone->state.above = FALSE;
	
	if ( event->new_window_state &GDK_WINDOW_STATE_BELOW)
	{
		backbone->state.below = TRUE;
		//SENTINEL("below\n");
	}
	else
		backbone->state.below = FALSE;
	
	return FALSE;
}

