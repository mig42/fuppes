/***************************************************************************
 *            fuppes_applet.c
 *
 *  FUPPES - Free UPnP Entertainment Service
 *
 *  Copyright (C) 2007 Ulrich Völkel <u-voelkel@users.sourceforge.net>
 ****************************************************************************/

/* 
 * this applet is based on the trashapplet from the gnome-applets-2.18.0 package
 *
 * Copyright (c) 2004  Michiel Sikkes <michiel@eyesopened.nl>,
 *               2004  Emmanuele Bassi <ebassi@gmail.com>
 */

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as 
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */


#include <stdio.h>
#include <string.h>
#include <time.h>

#ifdef HAVE_LIBNOTIFY
#include <libnotify/notify.h>
#include <libnotify/notification.h>
#include <libnotify/notify-enum-types.h>
#endif

#include <libgnome/gnome-help.h>
#include "fuppes-applet.h"
#include "../../include/fuppes.h"


#define PIXMAPSDIR FUPPES_PIXMAPSDIR
#define FUPPES_SVG PIXMAPSDIR "/fuppes.svg"


G_DEFINE_TYPE(FuppesApplet, fuppes_applet, PANEL_TYPE_APPLET)

static void     fuppes_applet_destroy            (GtkObject         *object);

static void     fuppes_applet_size_allocate      (GtkWidget         *widget,
						 GdkRectangle     *allocation);
/*static gboolean fuppes_applet_button_release     (GtkWidget         *widget,
						 GdkEventButton    *event);
static gboolean fuppes_applet_key_press          (GtkWidget         *widget,
						 GdkEventKey       *event);*/
/*static void     trash_applet_drag_leave         (GtkWidget         *widget,
						 GdkDragContext    *context,
						 guint              time_);
static gboolean trash_applet_drag_motion        (GtkWidget         *widget,
						 GdkDragContext    *context,
						 gint               x,
						 gint               y,
						 guint              time_);
static void     trash_applet_drag_data_received (GtkWidget         *widget,
						 GdkDragContext    *context,
						 gint               x,
						 gint               y,
						 GtkSelectionData  *selectiondata,
						 guint              info,
						 guint              time_); */

static void     fuppes_applet_change_orient      (PanelApplet     *panel_applet,
						 PanelAppletOrient  orient);
/*static void     trash_applet_change_background  (PanelApplet     *panel_applet,
						 PanelAppletBackgroundType type,
						 GdkColor        *colour,
						 GdkPixmap       *pixmap);
static void trash_applet_theme_change (GtkIconTheme *icon_theme, gpointer data);

*/
static void fuppes_applet_do_start_stop    (BonoboUIComponent *component,
				      FuppesApplet       *applet,
				      const gchar       *cname);
static void fuppes_applet_show_about  (BonoboUIComponent *component,
				      FuppesApplet       *applet,
				      const gchar       *cname);

/*static void trash_applet_open_folder (BonoboUIComponent *component,
				      TrashApplet       *applet,
				      const gchar       *cname); */
static void fuppes_applet_show_help   (BonoboUIComponent *component,
				      FuppesApplet       *applet,
				      const gchar       *cname);

/*static const GtkTargetEntry drop_types[] = {
	{ "text/uri-list", 0, 0 }
};*/
//static const gint n_drop_types = G_N_ELEMENTS (drop_types);

static const char fuppes_applet_menu_xml [] =
   "<popup name=\"button3\">\n"
   "   <menuitem name=\"StartStop Item\" "
   "             verb=\"FuppesStartStop\" "
   "           _label=\"_Start/_Stop\"\n"
   "          pixtype=\"stock\" "
   "          pixname=\"gtk-refresh\"/>\n"
   "   <separator/>\n"
   "   <menuitem name=\"Open Help Item\" verb=\"HelpFuppes\" _label=\"_Help\" pixtype=\"stock\" pixname=\"gtk-help\" />\n "
   "   <menuitem name=\"About Item\" verb=\"AboutFuppes\" _label=\"_About\" pixtype=\"stock\" pixname=\"gtk-about\" />\n "
   "</popup>\n";

static const BonoboUIVerb fuppes_applet_menu_verbs [] = {
	BONOBO_UI_UNSAFE_VERB ("FuppesStartStop", fuppes_applet_do_start_stop),
	//BONOBO_UI_UNSAFE_VERB ("OpenTrash", trash_applet_open_folder),
	BONOBO_UI_UNSAFE_VERB ("AboutFuppes", fuppes_applet_show_about),
	BONOBO_UI_UNSAFE_VERB ("HelpFuppes", fuppes_applet_show_help),
	BONOBO_UI_VERB_END
};

/*static void trash_applet_queue_update (TrashApplet  *applet);
static void item_count_changed        (TrashMonitor *monitor,
				       TrashApplet  *applet); */

static void
fuppes_applet_class_init (FuppesAppletClass *class)
{
	GTK_OBJECT_CLASS (class)->destroy = fuppes_applet_destroy;
	GTK_WIDGET_CLASS (class)->size_allocate = fuppes_applet_size_allocate;
	//GTK_WIDGET_CLASS (class)->button_release_event = trash_applet_button_release;
	//GTK_WIDGET_CLASS (class)->key_press_event = trash_applet_key_press;
	/*GTK_WIDGET_CLASS (class)->drag_leave = trash_applet_drag_leave;
	GTK_WIDGET_CLASS (class)->drag_motion = trash_applet_drag_motion;
	GTK_WIDGET_CLASS (class)->drag_data_received = trash_applet_drag_data_received;*/
	PANEL_APPLET_CLASS (class)->change_orient = fuppes_applet_change_orient;
	//PANEL_APPLET_CLASS (class)->change_background = trash_applet_change_background;
}

static GtkImage* svg_image;
static FuppesApplet* g_applet;

static void
fuppes_applet_init (FuppesApplet *applet)
{
	/*GnomeVFSResult res;
	GnomeVFSURI *trash_uri;*/

	//gtk_window_set_default_icon_name (TRASH_ICON_FULL);

	panel_applet_set_flags (PANEL_APPLET (applet), PANEL_APPLET_EXPAND_MINOR);

	/* get the default gconf client */
	/*if (!client)
		client = gconf_client_get_default ();*/

	applet->size = 0;
  applet->is_started = FALSE;
  g_applet = applet;
//	applet->new_size = 0;

	switch (panel_applet_get_orient (PANEL_APPLET (applet))) {
	case PANEL_APPLET_ORIENT_LEFT:
	case PANEL_APPLET_ORIENT_RIGHT:
		applet->orient = GTK_ORIENTATION_VERTICAL;
		break;
	case PANEL_APPLET_ORIENT_UP:
	case PANEL_APPLET_ORIENT_DOWN:
		applet->orient = GTK_ORIENTATION_HORIZONTAL;
		break;
	}
	applet->tooltips = gtk_tooltips_new ();
  gtk_tooltips_set_tip(applet->tooltips, GTK_WIDGET(applet), _("FUPPES UPnP Server (stopped)"), NULL);
	g_object_ref (applet->tooltips);
	gtk_object_sink (GTK_OBJECT (applet->tooltips));

  //char* szDir = PIXMAPSDIR "/fuppes.svg";
  svg_image = GTK_IMAGE(gtk_image_new_from_file(FUPPES_SVG));
	applet->image = gtk_image_new();
	gtk_container_add (GTK_CONTAINER (applet), applet->image);
	gtk_widget_show (applet->image);
	//applet->icon_state = TRASH_STATE_UNKNOWN;
  
  #ifdef HAVE_LIBNOTIFY
  if(!notify_init("fuppes"))
    printf("notify_init() failed\n");
  #endif        
}

static void
fuppes_applet_destroy (GtkObject *object)
{
  // stop fuppes
  if(fuppes_is_started() == 0) {    
    fuppes_stop();
    fuppes_cleanup();    
  }    
  
  #ifdef HAVE_LIBNOTIFY
  notify_uninit();
  #endif
  
	FuppesApplet *applet = FUPPES_APPLET (object);
	
	applet->image = NULL;
	if (applet->tooltips)
		g_object_unref (applet->tooltips);
	applet->tooltips = NULL;
  
	(* GTK_OBJECT_CLASS (fuppes_applet_parent_class)->destroy) (object);
}

static void
fuppes_applet_size_allocate (GtkWidget    *widget,
			    GdkRectangle *allocation)
{
  GdkPixbuf   *pixbuf;
	FuppesApplet *applet = FUPPES_APPLET (widget);
	gint new_size;
  GError *error;

	if (applet->orient == GTK_ORIENTATION_HORIZONTAL)
		new_size = allocation->height;
	else
		new_size = allocation->width;

	if (new_size != applet->size) {
    applet->size = new_size;

  pixbuf = gtk_image_get_pixbuf (GTK_IMAGE (svg_image));
  pixbuf = gdk_pixbuf_scale_simple (pixbuf,
					new_size - 1,
					new_size - 1,
					GDK_INTERP_BILINEAR);
  gtk_image_set_from_pixbuf (GTK_IMAGE (applet->image), pixbuf);

	}

	(* GTK_WIDGET_CLASS (fuppes_applet_parent_class)->size_allocate) (widget, allocation);
}

static void
fuppes_applet_change_orient (PanelApplet       *panel_applet,
			    PanelAppletOrient  orient)
{
	FuppesApplet *applet = FUPPES_APPLET (panel_applet);

	switch (orient) {
	case PANEL_APPLET_ORIENT_LEFT:
	case PANEL_APPLET_ORIENT_RIGHT:
		applet->orient = GTK_ORIENTATION_VERTICAL;
		break;
	case PANEL_APPLET_ORIENT_UP:
	case PANEL_APPLET_ORIENT_DOWN:
		applet->orient = GTK_ORIENTATION_HORIZONTAL;
		break;
	}

	if (PANEL_APPLET_CLASS (fuppes_applet_parent_class)->change_orient)
		(* PANEL_APPLET_CLASS (fuppes_applet_parent_class)->change_orient) (panel_applet, orient);
}



void fuppes_error_callback(const char* sz_err)
{
  GtkWidget* dialog = gtk_message_dialog_new (NULL,
                                   GTK_DIALOG_DESTROY_WITH_PARENT,
                                   GTK_MESSAGE_ERROR,
                                   GTK_BUTTONS_CLOSE,
                                   sz_err);
  gtk_dialog_run(GTK_DIALOG (dialog));
  gtk_widget_destroy(dialog);
}

void log_callback(const char* sz_log)
{
  printf("panel applet: %s\n", sz_log);
}

void fuppes_notify_callback(const char* sz_title, const char* sz_msg)
{ 
  #ifdef HAVE_LIBNOTIFY
  NotifyNotification* pNotification; 
  pNotification = notify_notification_new(sz_title, sz_msg, NULL, NULL);
  
  //char* g_strdup_printf
  //NOTIFY_URGENCY_LOW 	 Low urgency. Used for unimportant notifications.
  //NOTIFY_URGENCY_NORMAL 	Normal urgency. Used for most standard notifications.
  //NOTIFY_URGENCY_CRITICAL 	Critical urgency. Used for very important notifications.  
  //notify_notification_set_urgency(pNotification, NOTIFY_URGENCY_LOW);
  
  GError* pError = NULL;
  if(!notify_notification_show(pNotification,  &pError)) {
    //
  }
  
  #else
  printf("fuppes_notify_callback(): %s\n", sz_msg);
  #endif 
}

static void
fuppes_applet_do_start_stop (BonoboUIComponent *component,
		       FuppesApplet       *applet,
		       const gchar       *cname)
{
	//g_return_if_fail (FUPPES_IS_APPLET (applet));

	if(fuppes_is_started() != 0) {
	  fuppes_init(0, NULL, log_callback);
    fuppes_set_notify_callback(fuppes_notify_callback);
    fuppes_set_error_callback(fuppes_error_callback);
	  if(fuppes_start() == FUPPES_TRUE) {
      gtk_tooltips_set_tip (g_applet->tooltips, GTK_WIDGET (g_applet), _("FUPPES UPnP Server (started)"), NULL);
    }
  }
  else {
    // stop fuppes
    fuppes_stop();
    fuppes_cleanup();
    //applet->is_started = FALSE;
    gtk_tooltips_set_tip (g_applet->tooltips, GTK_WIDGET (g_applet), _("FUPPES UPnP Server (stopped)"), NULL);
  }
  

}

static void
fuppes_applet_show_help (BonoboUIComponent *component,
			FuppesApplet       *applet,
			const gchar       *cname)
{
        GError *err = NULL;

        g_return_if_fail (FUPPES_IS_APPLET (applet));

	/* FIXME - Actually, we need a user guide */
        gnome_help_display_desktop_on_screen
		(NULL, "trashapplet", "trashapplet", NULL,
		 gtk_widget_get_screen (GTK_WIDGET (applet)),
		 &err);

        if (err) {
        	//error_dialog (applet, _("There was an error displaying help: %s"), err->message);
        	g_error_free (err);
        }
}


static void
fuppes_applet_show_about (BonoboUIComponent *component,
			 FuppesApplet       *applet,
			 const gchar       *cname)
{
	static const char *authors[] = {
		"Ulrich Völkel <u-voelkel@users.sourceforge.net>",
		NULL
	};
	static const char *documenters[] = {
		"Ulrich Völkel <u-voelkel@users.sourceforge.net>",
		NULL
	};

	gtk_show_about_dialog
		(NULL,
		 "name", _("FUPPES Applet"),
		 "version", "0.7.2",
		 "copyright", "Copyright \xC2\xA9 2007 Computertechnik Völkel",
		 "comments", _("A FUPPES applet for GNOME."),
		 "authors", authors,
		 "documenters", documenters,
		 "translator-credits", _("translator-credits"),
		 "logo_icon_name", "fuppes",
		 NULL);
}


static gboolean
fuppes_applet_factory (PanelApplet *applet,
                      const gchar *iid,
		      gpointer     data)
{
	gboolean retval = FALSE;

  	if (!strcmp (iid, "OAFIID:FuppesApplet")) {
		// Set up the menu 
    panel_applet_setup_menu(PANEL_APPLET(applet),
                          fuppes_applet_menu_xml,
                          fuppes_applet_menu_verbs,
                          NULL);

		gtk_widget_show (GTK_WIDGET (applet));

		retval = TRUE;
	}

  	return retval;
}


/*PANEL_APPLET_BONOBO_FACTORY("OAFIID:FuppesApplet_Factory", 
                            PANEL_TYPE_APPLET,
                            "The Fuppes Applet",
                            "0",
                            fuppes_applet_factory,
                            NULL);*/

int
main (int argc, char *argv [])
{
	/* gettext stuff */
/*        bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
        bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
        textdomain (GETTEXT_PACKAGE);*/

//	gnome_authentication_manager_init();

#define VERSION "0.1"
#define PREFIX "/usr"
#define SYSCONFDIR "/etc"
#define DATADIR "/usr/share"
#define LIBDIR "/lib"

        gnome_program_init ("fuppes-applet", VERSION,
                            LIBGNOMEUI_MODULE,
                            argc, argv,
                            GNOME_CLIENT_PARAM_SM_CONNECT, FALSE,
                            GNOME_PROGRAM_STANDARD_PROPERTIES,
                            NULL); 
        return panel_applet_factory_main
		("OAFIID:FuppesApplet_Factory", FUPPES_TYPE_APPLET,
		 fuppes_applet_factory, NULL);
}
