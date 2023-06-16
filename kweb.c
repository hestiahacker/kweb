/*                    Minimal Kiosk Browser 

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Copyright 2013 Ralph Glass (Minimal Web Browser base code)                               
   Copyright 2013-2014 Guenter Kreidl (Minimal Kiosk Browser)

   Version 1.4
*/

#include <gtk/gtk.h>
#include <webkit/webkit.h>
#include <gdk/gdkkeysyms.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

static int window_count = 0;
static gboolean javascript = FALSE;
static gboolean kioskmode = FALSE;
static gboolean fullscreen = FALSE;
static gboolean alternate = FALSE;
static gboolean cookies_allowed = FALSE;
static gboolean private_browsing = TRUE;
static gboolean no_window = FALSE;
static gboolean external_download = FALSE;
static gboolean open_executable = FALSE;
static gboolean maximize = TRUE;
static gboolean experimental = TRUE;
static gboolean full_zoom = FALSE;
static gboolean startpage = TRUE;
static gboolean useOMX = TRUE;
static int defaultw = 1920;
static int defaulth = 1080;
static gfloat current_zoom = 1.0;
static WebKitSettings* settings;
static SoupCookieJar* cookiejar;
static char* homedir;
static char* homepage;
static char* homecommand;
static char* kb_commands = "";
static char* last_filepath = "";
static char* dldir;
static char* allowed_kb_commands = "+-zbhrcfpoqgtjnediwxy";
static gchar* search_str;
static int hclen;
static int iconsize = GTK_ICON_SIZE_LARGE;

static void destroy(GtkWidget* widget, gpointer* data)
{
        window_count--;
}

static void closeView(WebKitWebView* webView, GtkWindow* window)
{
        gtk_window_destroy(window);
}

static void goBack(GtkWidget* window, WebKitWebView* webView)
{
        webkit_web_view_go_back(webView);
}

static void goHome(GtkWidget* window, WebKitWebView* webView)
{
	webkit_web_view_load_uri(webView,homepage);
}

static void web_zoom_plus(GtkWidget* window, WebKitWebView* webView)
{
	current_zoom = webkit_web_view_get_zoom_level(webView) + .25;
	webkit_web_view_set_zoom_level(webView, current_zoom);
}

static void web_zoom_minus(GtkWidget* window, WebKitWebView* webView)
{
	current_zoom = webkit_web_view_get_zoom_level(webView) - 0.25;
	webkit_web_view_set_zoom_level(webView, current_zoom);
}

static void web_zoom_100(GtkWidget* window, WebKitWebView* webView)
{
        gfloat zoom = webkit_web_view_get_zoom_level(webView);
	if (zoom != 1.0) {
		webkit_web_view_set_zoom_level(webView, 1.0);
		current_zoom =1.0;
	}
}

static gboolean downloadRequested(WebKitWebView*  webView,
                              WebKitDownload* download, 
                              GtkEntry* entry)
{ if (external_download == TRUE)
{
        const char* uri = webkit_uri_request_get_uri(webkit_download_get_request(download));
        int r = 0;
        r = fork();
        if (r==0) {
                execl("/usr/local/bin/kwebhelper.py", "kwebhelper.py", "dl" , uri, NULL);
        }
}
else
{
	const char* fname = webkit_download_get_destination(download);
	const char* dluri = g_strjoin(NULL,"file://",homedir,"/Downloads/",fname, NULL );
	webkit_download_set_destination(download,dluri);
	const char* msg = g_strjoin(NULL,"downloading: ",fname, NULL );
	if (kioskmode == FALSE) {
	gtk_entry_buffer_set_text(gtk_entry_get_buffer(entry), msg, strlen(msg)); }
	
	return TRUE; }
}

static void searchText(WebKitWebView* webView, gchar* searchString)
{
//        webkit_web_view_unmark_text_matches(webView);
//        webkit_web_view_search_text(webView, searchString, false, true, true);
//        webkit_web_view_mark_text_matches(webView, searchString, false, 0);
//        webkit_web_view_set_highlight_text_matches(webView, true);
}

static void setJbutton(GtkWidget* window)
{
	GtkWidget *Jbutton = g_object_get_data(G_OBJECT(window), "jbutton");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON( Jbutton),  javascript);
}

static void setCbutton(GtkWidget* window)
{
	GtkWidget *Cbutton = g_object_get_data(G_OBJECT(window), "cbutton");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Cbutton),  cookies_allowed);
}

static void setDbutton(GtkWidget* window)
{
	GtkWidget *Dbutton = g_object_get_data(G_OBJECT(window), "dbutton");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Dbutton),  external_download);
}

static void setZbutton(GtkWidget* window)
{
	GtkWidget *Zbutton = g_object_get_data(G_OBJECT(window), "zbutton");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Zbutton),  full_zoom);
}

static void setObutton(GtkWidget* window)
{
	GtkWidget *Obutton = g_object_get_data(G_OBJECT(window), "obutton");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Obutton),  useOMX);
}


static gboolean setButtons(GtkWidget* window, void* event, WebKitWebView* webView)
{
	if (kioskmode == FALSE) {
	setJbutton(window);
	setCbutton(window);
	setDbutton(window);
	setZbutton(window);
	setObutton(window);
 	}
	current_zoom = webkit_web_view_get_zoom_level(webView);
	return FALSE;	
}

static void toggleJavascript(GtkWidget* item, WebKitWebView* webView)
{
	javascript = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(item));
        g_object_set(G_OBJECT(settings), "enable-scripts", javascript , NULL);
        webkit_web_view_reload(webView);
}

static void toggleDownload(GtkWidget* item, WebKitWebView* webView)
{
	external_download = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(item));
}

static void toggleOmx(GtkWidget* item, WebKitWebView* webView)
{
	useOMX = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(item));
}


static void toggleZoom(GtkWidget* item, WebKitWebView* webView)
{
	full_zoom = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(item));
	webkit_web_view_set_zoom_level(webView,full_zoom);
}

static void reload(GtkWidget* window, WebKitWebView* webView)
{
        webkit_web_view_reload(webView);
}

static void togglefullscreen(GtkWidget* window, WebKitWebView* webView)
{
	if (no_window == FALSE) {
        fullscreen=!fullscreen;
	if (fullscreen == TRUE) {
	gtk_window_fullscreen(GTK_WINDOW(window));}
	else {
	gtk_window_unfullscreen(GTK_WINDOW(window));}
	}
}

static void toggleCookies(GtkWidget* item, WebKitWebView* webView)
{
	cookies_allowed = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(item));
	if (cookies_allowed == TRUE) {
	soup_cookie_jar_set_accept_policy(cookiejar, SOUP_COOKIE_JAR_ACCEPT_NO_THIRD_PARTY); }
	else {
	soup_cookie_jar_set_accept_policy(cookiejar, SOUP_COOKIE_JAR_ACCEPT_NEVER);}
}

static void YT_Reload(GtkWidget* window, WebKitWebView* webView)
{
	const gchar* uri = webkit_web_view_get_uri(webView);
	        int r = 0;
        	r = fork();
        	if (r == 0) {
                	execl("/usr/local/bin/kwebhelper.py", "kwebhelper.py", "ytdl", uri, NULL);
      			}
}

static void activateEntry(GtkWidget* entry, gpointer* gdata)
{
        WebKitWebView* webView = g_object_get_data(G_OBJECT(entry), "webView");
        const gchar* entry_str = gtk_entry_buffer_get_text(gtk_entry_get_buffer(GTK_ENTRY(entry)));
        /* search on Google if prefix ? */
        if (strncmp(entry_str , "?", 1) == 0 ){ 
                gchar* s = g_strdup(entry_str);
                s++; // remove prefix
                gtk_entry_buffer_set_text(gtk_entry_get_buffer(GTK_ENTRY(entry)),
				g_strjoin(NULL, search_str, s, NULL),
				strlen(g_strjoin(NULL, search_str, s, NULL)));
		if ((startpage == TRUE) && (full_zoom == TRUE) && (current_zoom != 1.0)) { 
			full_zoom = FALSE;
	                webkit_web_view_set_zoom_level(webView,1.0);
			if (kioskmode == FALSE) {
				GtkWidget* window = g_object_get_data(G_OBJECT(entry), "window");
				setZbutton(window); }
			}
        /* search text on page if prefix / */
        } else if (strncmp(entry_str, "/", 1) == 0 ){
                gchar* s = g_strdup(entry_str);
                s++; // remove prefix
                searchText(webView, s); 
                return;
        /* execute command if prefix # */
        } else if (strncmp(entry_str, "#", 1) == 0 ){
        	int r = 0;
        	r = fork();
        	if (r == 0) {
                execl("/usr/local/bin/kwebhelper.py", "kwebhelper.py", "cmd", entry_str, NULL);
        }
		
                return;
        /* put http:// in front */
        } else if ((strncmp( entry_str, "http", 4) != 0 ) && (strncmp( entry_str, "file", 4) != 0 ) && (strncmp( entry_str, "ftp", 3) != 0 )) { 
                gtk_entry_buffer_set_text(gtk_entry_get_buffer(GTK_ENTRY(entry)),
                                    g_strjoin(NULL, "http://", entry_str, NULL),
                                    strlen(g_strjoin(NULL, "http://", entry_str, NULL)));
        }
        const gchar* uri = gtk_entry_buffer_get_text(gtk_entry_get_buffer(GTK_ENTRY(entry)));
        webkit_web_view_load_uri(webView, uri);
	gtk_widget_grab_focus( GTK_WIDGET(webView));
}

static gboolean navigationPolicyDecision(WebKitWebView*          webView,
                                         WebKitWebView*          frame,
                                         WebKitURIRequest*       request,
                                         WebKitNavigationAction* action,
                                         WebKitPolicyDecision*   decision,
                                         GtkEntry*               entry )
{
        const char* uri = webkit_uri_request_get_uri(request);
	const char* siteuri = webkit_web_view_get_uri(webView);
	if ((strncmp(uri, homecommand, hclen ) == 0 )  && ((strncmp(uri, "file:///", 8 ) == 0)  || (strncmp(siteuri,"http://localhost:",17) == 0) || (strncmp(siteuri,"http://localhost/",17) == 0) )){
        	int r = 0;
        	r = fork();
        	if (r == 0) {
                	execl("/usr/local/bin/kwebhelper.py", "kwebhelper.py", "cmd", uri, NULL);
      			}
	webkit_policy_decision_ignore(decision);
	return TRUE;
	}

//	if ((webkit_web_frame_get_parent(frame) == NULL) && (kioskmode == FALSE)) {
//        	gtk_entry_set_text(entry, uri); }
//        return FALSE;
	return TRUE;
}

static gboolean mimePolicyDecision( WebKitWebView*        webView,
                                    WebKitWebView*        frame,
                                    WebKitURIRequest*     request,
                                    gchar*                mimetype,
                                    WebKitPolicyDecision* policy_decision,
                                    gpointer*             user_data)
{	const char* uri = webkit_uri_request_get_uri(request);

        if (((strncmp(mimetype, "video/", 6) == 0) || (strncmp(mimetype, "audio/", 6) == 0) ||
            (strncmp(mimetype, "application/octet-stream", 24) == 0)) && (useOMX == TRUE)) {
        int r = 0;
        r = fork();
        if (r == 0) {
                execl("/usr/local/bin/kwebhelper.py", "kwebhelper.py", "av", uri, mimetype, NULL);
        }
                webkit_policy_decision_ignore(policy_decision);
                return TRUE;
        }

        else if (strncmp(mimetype, "application/pdf", 15) == 0) {        

        int r = 0;
        r = fork();
        if (r == 0) {
                execl("/usr/local/bin/kwebhelper.py", "kwebhelper.py", "pdf", uri, NULL);
        }
                webkit_policy_decision_ignore(policy_decision);
                return TRUE;
        }

        return FALSE;
}

static WebKitWebView* createWebView (WebKitWebView*  parentWebView,
                                     WebKitWebView*  frame,
                                     gchar*          arg)
{
        window_count++;
        GtkWidget*     window = gtk_window_new();
        GtkWidget*     vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        GtkWidget*     scrolledWindow = gtk_scrolled_window_new();
        GtkWidget*     uriEntry = gtk_entry_new();
        WebKitWebView* webView = WEBKIT_WEB_VIEW(webkit_web_view_new());

        webkit_settings_set_enable_javascript (settings, javascript);
        webkit_settings_set_allow_file_access_from_file_urls(settings, TRUE);
        webkit_settings_set_allow_universal_access_from_file_urls(settings, TRUE);
        webkit_settings_set_enable_spatial_navigation(settings, TRUE);

	if(experimental == TRUE) {
        g_object_set(G_OBJECT(settings), "enable-webgl", TRUE,NULL); }

        webkit_web_view_set_settings(WEBKIT_WEB_VIEW(webView), settings);
	webkit_web_view_set_zoom_level(webView,full_zoom);
	if (full_zoom == FALSE) {
	webkit_web_view_set_zoom_level(webView, current_zoom); }

        g_object_set_data( G_OBJECT(uriEntry), "webView", webView);
        g_object_set_data( G_OBJECT(uriEntry), "window", window);

	g_object_set_data( G_OBJECT(window), "uriEntry", uriEntry);
	g_object_set_data( G_OBJECT(window), "webView", webView);

	gtk_window_set_default_size(GTK_WINDOW(window), defaultw, defaulth);

	if(((maximize == TRUE) || (kioskmode == TRUE)) && (no_window == FALSE)) {
	gtk_window_maximize(GTK_WINDOW(window));  }
	if ((kioskmode == TRUE) && (fullscreen == TRUE)) {
		gtk_window_fullscreen(GTK_WINDOW(window)); }

        gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolledWindow), GTK_WIDGET(webView));
        gtk_window_set_child(GTK_WINDOW(window), scrolledWindow);

        g_signal_connect(window,   "destroy",             G_CALLBACK(destroy),            NULL);
        g_signal_connect(webView,  "close",               G_CALLBACK(closeView),          window);
	if (kioskmode == FALSE) {
        g_signal_connect(uriEntry, "activate",            G_CALLBACK(activateEntry),      NULL); }
/*        g_signal_connect(window, "focus-in-event",            G_CALLBACK(setButtons),      webView); */

        g_signal_connect(webView,  "create",              G_CALLBACK(createWebView),      NULL);

        if (kioskmode == TRUE) {
            if (arg == NULL) {
                webkit_web_view_load_uri(webView, homepage); }
            else {
                webkit_web_view_load_uri(webView, arg); }  }
        else {
            gtk_widget_activate(uriEntry); }

        gtk_widget_grab_focus( GTK_WIDGET(webView));
        gtk_widget_show(window);
        return webView;
}

void signal_catcher (int signal)
{
int status;
int chpid = waitpid(-1, & status,WNOHANG );
}

/* main */

int main( int argc, char* argv[] )
{
        homedir = getenv("HOME");
        const char* cookie_file_name = g_strjoin(NULL, homedir, "/.web_cookie_jar", NULL);
        gtk_init();
//        SoupSession* session = webkit_get_default_session();
        SoupSessionFeature* feature;
        cookiejar = soup_cookie_jar_text_new(cookie_file_name,FALSE);
        feature = (SoupSessionFeature*)(cookiejar);
//        soup_session_add_feature(session, feature);
        soup_cookie_jar_set_accept_policy(cookiejar, SOUP_COOKIE_JAR_ACCEPT_NEVER);
        settings = webkit_settings_new();
        homepage = g_strjoin(NULL,"file://",homedir,"/homepage.html", NULL );
	search_str = "https://startpage.com/do/search?q=";
	struct stat stb;
	if (stat(g_strjoin(NULL,homedir,"/homepage.html", NULL ), &stb) != 0) {
	homepage = g_strjoin(NULL,"file://",homedir,"/", NULL ); }
        homecommand = "file:///homepage.html?cmd=";
	hclen = strlen(homecommand);
	if (argv[1] != NULL) {
	if (strncmp(argv[1],"-",1) == 0) {
		const char* arg1 = argv[1];
		int i;
		char res[] = "";
		for (i=1; i<strlen(arg1);i++) {
			if (arg1[i] == 'K') {
				kioskmode = TRUE;
                                fullscreen = TRUE;
				}
		else if (arg1[i] == 'A') {
			alternate = TRUE;}
		else if (arg1[i] == 'J') {
			javascript = TRUE;}
		else if (arg1[i] == 'E') {
			cookies_allowed = TRUE;
                        soup_cookie_jar_set_accept_policy(cookiejar, SOUP_COOKIE_JAR_ACCEPT_NO_THIRD_PARTY);}
		else if (arg1[i] == 'I') {}
		else if (arg1[i] == 'T') {}
		else if (arg1[i] == 'S') {
			iconsize = GTK_ICON_SIZE_NORMAL;}
		else if (arg1[i] == 'H') {
			if(argv[2] != NULL) {
			homepage = argv[2];}
			}
		else if (arg1[i] == 'L') {
			if(argv[2] != NULL) {
			if ( (strncmp(argv[2],"http://localhost:",17) == 0)  || (strncmp(argv[2],"http://localhost/",17) == 0) ) {
			homecommand = g_strjoin(NULL,argv[2],"homepage.html?cmd=", NULL );
			hclen = strlen(homecommand);
			homepage = argv[2];
			} } }
		else if (arg1[i] == 'P') {
			private_browsing=FALSE;}
		else if (arg1[i] == 'N') {
			no_window=TRUE;}
		else if (arg1[i] == 'M') {
			maximize=FALSE;}
		else if (arg1[i] == 'W') {
			external_download=TRUE;}
		else if (arg1[i] == 'X') {
			open_executable=TRUE;}
		else if (arg1[i] == 'Z') {
			full_zoom=TRUE;}
		else if (arg1[i] == 'G') {
			startpage=FALSE;
			search_str = "https://www.google.com/search?as_q=";
			}
		else if (arg1[i] == 'F') {
			experimental=FALSE;}
		else if (arg1[i] == 'Y') {
			useOMX=FALSE;}
		else if (arg1[i] == '0') {
			defaultw = 640;
			defaulth = 480; }
		else if (arg1[i] == '1') {
			defaultw = 768;
			defaulth = 576; }
		else if (arg1[i] == '2') {
			defaultw = 800;
			defaulth = 600; }
		else if (arg1[i] == '3') {
			defaultw = 1024;
			defaulth = 768; }
		else if (arg1[i] == '4') {
			defaultw = 1280;
			defaulth = 1024; }
		else if (arg1[i] == '5') {
			defaultw = 1280;
			defaulth = 720; }
		else if (arg1[i] == '6') {
			defaultw = 1366;
			defaulth = 768; }
		else if (arg1[i] == '7') {
			defaultw = 1600;
			defaulth = 900; }
		else if (arg1[i] == '8') {
			defaultw = 1600;
			defaulth = 1050; }
		else if (arg1[i] == '9') {
			defaultw = 1920;
			defaulth = 1200; }
		else { if(strchr(allowed_kb_commands,arg1[i]) != NULL) {
			int len = strlen(res);
			res[len + 1] = res[len];
			res[len] = arg1[i]; }
			}
			}
		kb_commands = g_strjoin(NULL, res,NULL);
		if(argv[2] != NULL) {
		    createWebView( NULL, NULL, argv[2] ); }
		else {
                    createWebView( NULL, NULL, NULL );}
		}
	else {createWebView( NULL, NULL, argv[1] );} 
	}
	else {
        	createWebView( NULL, NULL, NULL ); }
	if (kioskmode == FALSE) {
	alternate = TRUE;}
	signal(SIGCHLD, signal_catcher);
	dldir = g_strjoin(NULL, homedir, "/Downloads", NULL);
	struct stat st = {0};
	if (stat(dldir, &st) == -1) {
		int result = mkdir(dldir, 0700); }
        // https://docs.gtk.org/gtk4/migrating-3to4.html#stop-using-gtk_main-and-related-apis
        while (g_list_model_get_n_items (gtk_window_get_toplevels ()) > 0)
                g_main_context_iteration (NULL, TRUE);
        return 0;
}
