#include <stdlib.h>
#include <vte/vte.h>
#include "gears.h"
#include "backbone.h"
#include "colors.h"
#include "tgregex.h"
#include "dbg.h"

/*Stuff for creating and freeing backbone*/
backbone_t * new_backbone( void)
{
	backbone_t * backbone;
	backbone = (backbone_t *) malloc( sizeof(backbone_t) );
	
	if ( backbone == NULL)
	{
		LOG_ERR("Malloc Failed");
		exit(EXIT_FAILURE);
	}
	backbone->state.withdrawn = FALSE;
	backbone->state.iconified = FALSE;
	backbone->state.maximized = FALSE;
	backbone->state.sticky = FALSE;
	backbone->state.fullscreen = FALSE;
	backbone->state.above = FALSE;
	backbone->state.below = FALSE;
	backbone->tabs_data = NULL;
	backbone->args.configuration_file = NULL;
	backbone->args.command_to_execute = NULL;
	return backbone;
}

void free_backbone( backbone_t * backbone)
{
	//SENTINEL("Free window stuff\n");
	FREE_GSTRING(backbone->window.title);
	FREE_GSTRING(backbone->window.wm_class);
	FREE_GSTRING(backbone->window.wm_name);
	FREE_GSTRING(backbone->window.role);
	FREE_GSTRING(backbone->window.background.color);
	
	//SENTINEL("Free notebook stuff\n");
	FREE_GSTRING(backbone->notebook.default_tab_name);
	FREE_GSTRING(backbone->notebook.active_tab.color);
	
	//SENTINEL("Free vte stuff\n");
	FREE_GSTRING(backbone->vte.foreground.color);
	FREE_GSTRING(backbone->vte.background.color);
	FREE_GSTRING(backbone->vte.command);
	int i =0;
	for( i =0; i<16; i++)
	{
		FREE_GSTRING(backbone->vte.palette[i].color);
	}
	FREE_GSTRING(backbone->vte.font);
	FREE_GSTRING(backbone->vte.cursor_color.color);
	
	//SENTINEL("Free css stuff\n");
	FREE_GSTRING(backbone->css.path);
	if(backbone->css.file)
		g_object_unref(backbone->css.file);
	
	//SENTINEL("Free other stuff\n");
	FREE_GSTRING(backbone->configuration.dir_path);
	FREE_GSTRING(backbone->configuration.file_path);
	/*no need to free args.configuration_file (already freed with FREE_GSTRING(backbone->configuration.file_path);*/
	FREE_GSTRING(backbone->args.command_to_execute);
	if(backbone->configuration.keyfile)
		g_key_file_free(backbone->configuration.keyfile);

	//SENTINEL("Free tabs_data slist stuff\n");
	free_slist_and_data(backbone->tabs_data);
	
	//SENTINEL("Free main backbone\n");
	free((void *) backbone);
}

void quit_gracefully(backbone_t * backbone)
{
	free_backbone(backbone);
	gtk_main_quit();
}

/*Stuff for filling backbone from configuration key file*/
void init_vte_color( backbone_t * backbone, int index, char * color)
{
	backbone->vte.palette[index].color = g_string_new(color);
	extended_gdk_rgba_parse(&backbone->vte.palette[index].rgba, backbone->vte.palette[index].color->str);
}

static void set_default_config(backbone_t *backbone)
{
	backbone->window.title=g_string_new("Tortosa");
	backbone->window.wm_class=g_string_new("Tortosa");
	backbone->window.wm_name=g_string_new("Tortosa");
	backbone->window.role=g_string_new("Tortosa");
	backbone->window.width=0;
	backbone->window.height=0;
	backbone->window.decorated=TRUE;
	backbone->window.background.color=g_string_new("#000000");
	extended_gdk_rgba_parse( &backbone->window.background.rgba, backbone->window.background.color->str);

	backbone->notebook.show_border=TRUE;
	backbone->notebook.show_tabs=TRUE;
	backbone->notebook.tabs_position=GTK_POS_TOP;
	backbone->notebook.default_tab_name=g_string_new(g_path_get_basename(g_getenv("SHELL")));
	backbone->notebook.tab_name_max_len=0;	
	backbone->notebook.active_tab.color = NULL;
	
	backbone->vte.command=g_string_new(g_getenv("SHELL"));
	backbone->vte.user_valid_palette=FALSE;
	backbone->vte.foreground.color=NULL;
	backbone->vte.background.color=NULL;
	backbone->vte.palette[0].color=NULL;
	backbone->vte.palette[1].color=NULL;
	backbone->vte.palette[2].color=NULL;
	backbone->vte.palette[3].color=NULL;
	backbone->vte.palette[4].color=NULL;
	backbone->vte.palette[5].color=NULL;
	backbone->vte.palette[6].color=NULL;
	backbone->vte.palette[7].color=NULL;
	backbone->vte.palette[8].color=NULL;
	backbone->vte.palette[9].color=NULL;
	backbone->vte.palette[10].color=NULL;
	backbone->vte.palette[11].color=NULL;
	backbone->vte.palette[12].color=NULL;
	backbone->vte.palette[13].color=NULL;
	backbone->vte.palette[14].color=NULL;
	backbone->vte.palette[15].color=NULL;
	backbone->vte.scrollback_lines=-1;
	backbone->vte.highlight.color= NULL;
	backbone->vte.cursor_color.color=NULL;
	backbone->vte.cursor_blink=VTE_CURSOR_BLINK_SYSTEM;
	backbone->vte.cursor_shape=VTE_CURSOR_SHAPE_BLOCK;
	backbone->vte.bell_audible = FALSE;
	backbone->vte.font = NULL;
	backbone->css.path=NULL;
	backbone->css.file=NULL;
}

static void get_key_string( GKeyFile * keyfile, gchar * group, gchar * key,  GString **destination, const gchar * default_value)
{
	GError *error = NULL;
	gchar * val;
	val = g_key_file_get_string(keyfile,group,key,&error);
	if ((val == NULL && error->code == G_KEY_FILE_ERROR_KEY_NOT_FOUND) || (val == NULL && error->code == G_KEY_FILE_ERROR_GROUP_NOT_FOUND))
		*destination = g_string_new(default_value);
	else
		*destination = g_string_new(val);
	if( error != NULL)
	{
		g_error_free(error);
		error = NULL;
	}
}

static void get_key_bool( GKeyFile * keyfile, gchar * group, gchar * key,  gboolean *destination, gboolean default_value)
{
	GError *error=NULL;
	*destination = g_key_file_get_boolean(keyfile,group,key,&error);
	if(error && (error->code == G_KEY_FILE_ERROR_KEY_NOT_FOUND || error->code == G_KEY_FILE_ERROR_GROUP_NOT_FOUND))
		*destination = default_value;
	if( error != NULL)
	{
		g_error_free(error);
		error = NULL;
	}
}

void load_window_configuration(backbone_t *backbone)
{
	backbone->window.title = g_string_new("Tortosa");	
	get_key_string(backbone->configuration.keyfile,"Window","wm_class",&backbone->window.wm_class, backbone->window.title->str);
	get_key_string(backbone->configuration.keyfile,"Window","wm_name",&backbone->window.wm_name, backbone->window.title->str);
	get_key_string(backbone->configuration.keyfile,"Window","wm_role",&backbone->window.role, backbone->window.title->str);
	backbone->window.width = g_key_file_get_integer(backbone->configuration.keyfile,"Window","width",NULL);
	backbone->window.height = g_key_file_get_integer(backbone->configuration.keyfile,"Window","height",NULL);
	get_key_bool(backbone->configuration.keyfile, "Window","decorated",&backbone->window.decorated ,TRUE);
	get_key_string(backbone->configuration.keyfile,"Window","background",&backbone->window.background.color, "#000000");
	extended_gdk_rgba_parse( &backbone->window.background.rgba, backbone->window.background.color->str);
}

void load_tabs_configuration(backbone_t *backbone)
{
	get_key_bool(backbone->configuration.keyfile, "Tabs", "show_border", &backbone->notebook.show_border ,TRUE);
	get_key_bool(backbone->configuration.keyfile, "Tabs", "show_tabs", &backbone->notebook.show_tabs ,TRUE);
	
	GString * position_string;
	get_key_string(backbone->configuration.keyfile,"Tabs","tabs_position",&position_string, "top");
	if ( g_strcmp0(position_string->str, "top") == 0)
		backbone->notebook.tabs_position = GTK_POS_TOP;
	if ( g_strcmp0(position_string->str, "bottom") == 0)
		backbone->notebook.tabs_position = GTK_POS_BOTTOM;
	if ( g_strcmp0(position_string->str, "left") == 0)
		backbone->notebook.tabs_position = GTK_POS_LEFT;
	if ( g_strcmp0(position_string->str, "right") == 0)
		backbone->notebook.tabs_position = GTK_POS_RIGHT;
	g_string_free(position_string, TRUE);
		
	backbone->notebook.tab_name_max_len = g_key_file_get_integer(backbone->configuration.keyfile,"Tabs","tab_name_max_len",NULL);
	get_key_string(backbone->configuration.keyfile,"Tabs","default_tab_name",&backbone->notebook.default_tab_name, g_path_get_basename(g_getenv("SHELL")));
	if ( (backbone->notebook.tab_name_max_len > 0) && ( backbone->notebook.default_tab_name->len > backbone->notebook.tab_name_max_len ))
	{
		backbone->notebook.default_tab_name = g_string_truncate(backbone->notebook.default_tab_name,backbone->notebook.tab_name_max_len);
	}
	
	get_key_string(backbone->configuration.keyfile,"Tabs","active_tab_color",&backbone->notebook.active_tab.color, NULL);
	if( backbone->notebook.active_tab.color->len == 0 )
	{
		g_string_free(backbone->notebook.active_tab.color, TRUE);
		backbone->notebook.active_tab.color = NULL;
	}
	else
		extended_gdk_rgba_parse( &backbone->notebook.active_tab.rgba, backbone->notebook.active_tab.color->str);
}

void load_vte_configuration(backbone_t *backbone)
{
	get_key_string(backbone->configuration.keyfile,"Vte","foreground",&backbone->vte.foreground.color, "#ffffff");
	get_key_string(backbone->configuration.keyfile,"Vte","background",&backbone->vte.background.color, "#000000");
	extended_gdk_rgba_parse( &backbone->vte.foreground.rgba, backbone->vte.foreground.color->str);
	extended_gdk_rgba_parse( &backbone->vte.background.rgba, backbone->vte.background.color->str);
		
	get_key_string(backbone->configuration.keyfile,"Vte","command",&backbone->vte.command,g_getenv("SHELL"));
	get_key_string(backbone->configuration.keyfile,"Vte","font",&backbone->vte.font,NULL);

	backbone->vte.user_valid_palette = TRUE;
	gchar * color_value;

#define LOAD_KEY_COLOR( I ) \
color_value = g_key_file_get_string(backbone->configuration.keyfile,"Vte","color" #I,NULL);\
if ( color_value != NULL )\
			init_vte_color( backbone, I, color_value);\
else\
{\
	backbone->vte.user_valid_palette = FALSE;\
	init_vte_color( backbone, I, "#000000");\
}\

	LOAD_KEY_COLOR(0)
	LOAD_KEY_COLOR(1)
	LOAD_KEY_COLOR(2)
	LOAD_KEY_COLOR(3)
	LOAD_KEY_COLOR(4)
	LOAD_KEY_COLOR(5)
	LOAD_KEY_COLOR(6)
	LOAD_KEY_COLOR(7)
	LOAD_KEY_COLOR(8)
	LOAD_KEY_COLOR(9)
	LOAD_KEY_COLOR(10)
	LOAD_KEY_COLOR(11)
	LOAD_KEY_COLOR(12)
	LOAD_KEY_COLOR(13)
	LOAD_KEY_COLOR(14)
	LOAD_KEY_COLOR(15)
#undef LOAD_KEY_COLOR
	backbone->vte.scrollback_lines = (glong) g_key_file_get_uint64(backbone->configuration.keyfile,"Vte","scrollback_lines", NULL);
	
	get_key_string(backbone->configuration.keyfile,"Vte","highlight",&backbone->vte.highlight.color,NULL);
	if( backbone->vte.highlight.color->len == 0 )
	{
		g_string_free(backbone->vte.highlight.color, TRUE);
		backbone->vte.highlight.color = NULL;
	}
	else
	{
		extended_gdk_rgba_parse( &backbone->vte.highlight.rgba, backbone->vte.highlight.color->str);
	}
	
	get_key_string(backbone->configuration.keyfile,"Vte","cursor_color",&backbone->vte.cursor_color.color,NULL);
	if( backbone->vte.cursor_color.color->len == 0 )
	{
		g_string_free(backbone->vte.cursor_color.color, TRUE);
		backbone->vte.cursor_color.color = NULL;
	}
	else
	{
		extended_gdk_rgba_parse( &backbone->vte.cursor_color.rgba, backbone->vte.cursor_color.color->str);
	}
	
	GString * cursor_blink;
	get_key_string(backbone->configuration.keyfile,"Vte","cursor_blink",&cursor_blink, "system");
	if ( g_strcmp0(cursor_blink->str, "system") == 0)
		backbone->vte.cursor_blink = VTE_CURSOR_BLINK_SYSTEM;
	if ( g_strcmp0(cursor_blink->str, "on") == 0)
		backbone->vte.cursor_blink = VTE_CURSOR_BLINK_ON;
	if ( g_strcmp0(cursor_blink->str, "off") == 0)
		backbone->vte.cursor_blink = VTE_CURSOR_BLINK_OFF;
	g_string_free(cursor_blink, TRUE);
	
	get_key_bool(backbone->configuration.keyfile, "Vte","bell_audible",&backbone->vte.bell_audible ,FALSE);
	
	GString * cursor_shape;
	get_key_string(backbone->configuration.keyfile,"Vte","cursor_shape",&cursor_shape, "block");
	if ( g_strcmp0(cursor_shape->str, "block") == 0)
		backbone->vte.cursor_shape = VTE_CURSOR_SHAPE_BLOCK;
	if ( g_strcmp0(cursor_shape->str, "ibeam") == 0)
		backbone->vte.cursor_shape = VTE_CURSOR_SHAPE_IBEAM;
	if ( g_strcmp0(cursor_shape->str, "underline") == 0)
		backbone->vte.cursor_shape = VTE_CURSOR_SHAPE_UNDERLINE;
	g_string_free(cursor_shape, TRUE);

}

void load_css_configuration(backbone_t * backbone)
{
	get_key_string(backbone->configuration.keyfile,"Css","file",&backbone->css.path,NULL);
	if(backbone->css.path->len == 0)
		backbone->css.file = NULL;
		
	if ( g_file_test(backbone->css.path->str, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
		backbone->css.file = g_file_new_for_path(backbone->css.path->str);
	else
	{
		backbone->css.file = NULL;
		LOG_WARN("Css file %s doesn\'t exist\n", backbone->css.path->str);
	}
}

void load_css_regexes_match(backbone_t * backbone)
{
	const gchar *string = gtk_css_provider_to_string(GTK_CSS_PROVIDER (backbone->provider));
	/*free if previously allocated*/
	GString * color = NULL;
	color = get_css_regex_match(backbone->css.window_background_color_regex, string);
	if(color)
	{	
		FREE_GSTRING(backbone->window.background.color);
		backbone->window.background.color = color;
		extended_gdk_rgba_parse( &backbone->window.background.rgba, backbone->window.background.color->str);
	}

	color = get_css_regex_match(backbone->css.notebook_tab_active_color_regex, string);
	if (color)
	{
		FREE_GSTRING(backbone->notebook.active_tab.color);
		backbone->notebook.active_tab.color = color;
		extended_gdk_rgba_parse( &backbone->notebook.active_tab.rgba, backbone->notebook.active_tab.color->str);
	}
}

gboolean load_config( backbone_t* backbone)
{
	gboolean ret;
	backbone->configuration.keyfile = g_key_file_new ();
	ret = g_key_file_load_from_file( backbone->configuration.keyfile,
																	 backbone->configuration.file_path->str,
																	 G_KEY_FILE_NONE,
																	 NULL);
	if (ret == FALSE)
	{	
		LOG_WARN("Unable to load the configuration file %s", backbone->configuration.file_path->str);
		
		set_default_config(backbone);
		g_key_file_free(backbone->configuration.keyfile);
		backbone->configuration.keyfile = NULL;
	}
	else
	{
		load_window_configuration(backbone);
		load_tabs_configuration(backbone);
		load_vte_configuration(backbone);
		load_css_configuration(backbone);
	}
	return ret;
}

gboolean reload_tortosa_configuration(backbone_t * backbone)
{
	if(backbone->configuration.keyfile)
		g_key_file_free(backbone->configuration.keyfile);
	gboolean ret;
	backbone->configuration.keyfile = g_key_file_new ();
	ret = g_key_file_load_from_file( backbone->configuration.keyfile,
																	 backbone->configuration.file_path->str,
																	 G_KEY_FILE_NONE,
																	 NULL);
	if (ret == FALSE)
	{	
		LOG_WARN("Unable to load the configuration file %s", backbone->configuration.file_path->str);
		g_key_file_free(backbone->configuration.keyfile);
		backbone->configuration.keyfile = NULL;
	}
	else	
	{
		/*window configuration*/
		FREE_GSTRING(backbone->window.title);
		FREE_GSTRING(backbone->window.wm_class);
		FREE_GSTRING(backbone->window.wm_name);
		FREE_GSTRING(backbone->window.role);
		backbone->window.height = 0;
		backbone->window.width = 0;
		backbone->window.decorated = TRUE;
		FREE_GSTRING(backbone->window.background.color);
		/*reload current configuration for window*/
		load_window_configuration(backbone);	
		apply_window_configuration(backbone->window.widget, backbone);
		
		/*notebook configuration*/
		backbone->notebook.show_border = TRUE;
		backbone->notebook.show_tabs = TRUE;
		backbone->notebook.tabs_position = GTK_POS_TOP;
		FREE_GSTRING(backbone->notebook.default_tab_name);
		FREE_GSTRING(backbone->notebook.active_tab.color);
		backbone->notebook.tab_name_max_len=0;
		load_tabs_configuration(backbone);
		apply_tabs_configuration(backbone);
		
		/*vte configuration*/
		backbone->vte.user_valid_palette=FALSE;
		FREE_GSTRING(backbone->vte.foreground.color);
		FREE_GSTRING(backbone->vte.background.color);
		FREE_GSTRING(backbone->vte.command);
		int i =0;
		for( i =0; i<16; i++)
		{
			FREE_GSTRING(backbone->vte.palette[i].color);
		}
		FREE_GSTRING(backbone->vte.highlight.color);
		FREE_GSTRING(backbone->vte.cursor_color.color);
		FREE_GSTRING(backbone->vte.font);
		load_vte_configuration(backbone);
		
		gint nb_vte=gtk_notebook_get_n_pages(GTK_NOTEBOOK(backbone->notebook.widget));
		
		for(i=0; i< nb_vte; i++)
		{
			GtkWidget *vte = gtk_notebook_get_nth_page(GTK_NOTEBOOK(backbone->notebook.widget), i);
			apply_vte_configuration(backbone, vte);
		}
		
		/*css configuration*/
		FREE_GSTRING(backbone->css.path);
		if(backbone->css.file)
			g_object_unref(backbone->css.file);
		load_css_configuration(backbone);
		if ( backbone->css.file != NULL)
		{
			gtk_css_provider_load_from_file( GTK_CSS_PROVIDER (backbone->provider), backbone->css.file, NULL/*&error*/);
			load_css_regexes_match(backbone);
		}
		else
		{
			LOG_WARN("No css file to reload or file doesn't exist check your tortosarc file.\n");
		}
		gtk_widget_queue_draw (GTK_WIDGET (backbone->window.widget));

	}
	return ret;
}

/*Stuff for managing GSList backbone->tabs_data*/
gint find_node_by_widget( gconstpointer node, gconstpointer widget)
{
	if( ((tab_data_t *) node)->widget == (GtkWidget *)widget)
		return 0;
	else
		return 1;
}

static free_tag_data (TagData *tagdata)
{
	g_slice_free (TagData, tagdata);
}

void remove_node_by_widget( GSList *slist, GtkWidget *widget)
{
	/*find the node for the widget*/
	GSList *found = NULL;
	tab_data_t * tab_data =NULL;
	found = g_slist_find_custom(slist, widget, (GCompareFunc) find_node_by_widget);
	/*remove it and free the data*/
	if (found)
	{
		/*release memory for tab_data slist*/
		tab_data = ((tab_data_t*) found->data);
		GSList *match_tags = tab_data->match_tags;
		slist = g_slist_remove(slist, found->data);	
		/*free tag data slist*/
		g_slist_foreach (match_tags, (GFunc) free_tag_data, NULL);
		g_slist_free (match_tags);
	}
	else
		LOG_WARN("data for tab not found....\n");
}

void free_slist_and_data( GSList * slist)
{
	g_slist_free_full(slist, (GDestroyNotify)g_free);
}

void  util_open_url (backbone_t * backbone )
{ 
	GError *error = NULL;
  char *uri;
	GtkWidget * vte = gtk_notebook_get_nth_page(GTK_NOTEBOOK(backbone->notebook.widget), gtk_notebook_get_current_page(GTK_NOTEBOOK(backbone->notebook.widget)));

	GSList *found = NULL;
	found = g_slist_find_custom(backbone->tabs_data, vte, (GCompareFunc) find_node_by_widget);
	int flavor;     
	gchar *match = NULL;
	if (found)
	{
		flavor = ((tab_data_t*) found->data)->current_flavor;
		match = ((tab_data_t*) found->data)->current_match;
	}
	if(match)
	{
		switch (flavor)
		{
			case FLAVOR_DEFAULT_TO_HTTP:
				uri = g_strdup_printf ("http://%s", match);
				break;
			case FLAVOR_EMAIL:
			  if (g_ascii_strncasecmp ("mailto:", match, 7) != 0)
					uri = g_strdup_printf ("mailto:%s", match);
				else
				  uri = g_strdup (match);
				break;
			case FLAVOR_VOIP_CALL:
			case FLAVOR_AS_IS:
				uri = g_strdup (match);
				break;
			default:
			  uri = NULL;
				 g_assert_not_reached ();
		}
		GdkScreen *screen;
	  screen = gtk_widget_get_screen (GTK_WIDGET (backbone->window.widget));
		if (!gtk_show_uri (screen, uri, backbone->time, &error))
		LOG_WARN("error opening uri %s", error->message);
	}
}

/*Check the window manager*/
gboolean window_manager_is_gnome_like(GdkScreen * screen)
{
	const char cinnamon[]="Mutter (Muffin)";
	const char gnomeshell[]="GNOME Shell";
	char * current_WM = gdk_x11_screen_get_window_manager_name(screen);
	//SENTINEL("%s\n", current_WM);
	gboolean is_gnome_like = FALSE;

	if ( g_strcmp0(current_WM, cinnamon) == 0)
		is_gnome_like = TRUE;
	if ( g_strcmp0(current_WM, gnomeshell) == 0)
		is_gnome_like = TRUE;

	return is_gnome_like;
}
