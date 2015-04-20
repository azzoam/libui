// 6 april 2015
#include "uipriv_unix.h"

struct window {
	uiWindow w;
	GtkWidget *widget;
	GtkContainer *container;
	GtkWindow *window;
	uiParent *content;
	int (*onClosing)(uiWindow *, void *);
	void *onClosingData;
	int margined;
	gulong destroyBlocker;
};

static gboolean onClosing(GtkWidget *win, GdkEvent *e, gpointer data)
{
	struct window *w = (struct window *) data;

	// manually destroy the window ourselves; don't let the delete-event handler do it
	if ((*(w->onClosing))(uiWindow(w), w->onClosingData))
		uiWindowDestroy(uiWindow(w));
	return TRUE;		// don't continue to the default delete-event handler; we destroyed the window by now
}

static int defaultOnClosing(uiWindow *w, void *data)
{
	return 1;
}

static void destroyBlocker(GtkWidget *widget, gpointer data)
{
	complain("attempt to dispose uiWindow at %p before uiWindowDestroy()", data);
}

// TODO should we change the GtkWindow's child first?
static void windowDestroy(uiWindow *ww)
{
	struct window *w = (struct window *) ww;

	// first, hide the window to prevent our cleanup from being noticed
	gtk_widget_hide(w->widget);
	// next, destroy the content uiParent
	uiParentDestroy(w->content);
	// now that we cleaned up properly, we can mark our window as ready to be destroyed
	g_signal_handler_disconnect(w->widget, w->destroyBlocker);
	// finally, destroy the window
	gtk_widget_destroy(w->widget);
	// and free ourselves
	uiFree(w);
}

static uintptr_t windowHandle(uiWindow *ww)
{
	struct window *w = (struct window *) ww;

	return (uintptr_t) (w->widget);
}

static char *windowTitle(uiWindow *ww)
{
	struct window *w = (struct window *) ww;

	return g_strdup(gtk_window_get_title(w->window));
}

static void windowSetTitle(uiWindow *ww, const char *title)
{
	struct window *w = (struct window *) ww;

	gtk_window_set_title(w->window, title);
}

static void windowShow(uiWindow *ww)
{
	struct window *w = (struct window *) ww;

	// don't use gtk_widget_show_all(); that will override user hidden settings
	gtk_widget_show(w->widget);
}

static void windowHide(uiWindow *ww)
{
	struct window *w = (struct window *) ww;
	gtk_widget_hide(w->widget);
}

static void windowOnClosing(uiWindow *ww, int (*f)(uiWindow *, void *), void *data)
{
	struct window *w = (struct window *) ww;

	w->onClosing = f;
	w->onClosingData = data;
}

static void windowSetChild(uiWindow *ww, uiControl *c)
{
	struct window *w = (struct window *) ww;

	uiParentSetMainControl(w->content, c);
	uiParentUpdate(w->content);
}

static int windowMargined(uiWindow *ww)
{
	struct window *w = (struct window *) ww;

	return w->margined;
}

static void windowSetMargined(uiWindow *ww, int margined)
{
	struct window *w = (struct window *) ww;

	w->margined = margined;
	if (w->margined)
		uiParentSetMargins(w->content, gtkXMargin, gtkYMargin, gtkXMargin, gtkYMargin);
	else
		uiParentSetMargins(w->content, 0, 0, 0, 0);
	uiParentUpdate(w->content);
}

uiWindow *uiNewWindow(const char *title, int width, int height, int hasMenubar)
{
	struct window *w;
	GtkWidget *vbox;
	GtkWidget *contentWidget;

	w = uiNew(struct window);

	w->widget = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	w->container = GTK_CONTAINER(w->widget);
	w->window = GTK_WINDOW(w->widget);

	gtk_window_set_title(w->window, title);
	// TODO this does not take menus or CSD into account
	gtk_window_resize(w->window, width, height);

	g_signal_connect(w->widget, "delete-event", G_CALLBACK(onClosing), w);
	w->destroyBlocker = g_signal_connect(w->widget, "destroy", G_CALLBACK(destroyBlocker), w);

	if (hasMenubar) {
		vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
		gtk_widget_show_all(vbox);
		gtk_container_add(w->container, vbox);

		gtk_container_add(GTK_CONTAINER(vbox), makeMenubar());

		w->content = uiNewParent((uintptr_t) GTK_CONTAINER(vbox));
		contentWidget = GTK_WIDGET(uiParentHandle(w->content));
		gtk_widget_set_hexpand(contentWidget, TRUE);
		gtk_widget_set_halign(contentWidget, GTK_ALIGN_FILL);
		gtk_widget_set_vexpand(contentWidget, TRUE);
		gtk_widget_set_valign(contentWidget, GTK_ALIGN_FILL);
	} else
		w->content = uiNewParent((uintptr_t) (w->container));

	w->onClosing = defaultOnClosing;

	uiWindow(w)->Destroy = windowDestroy;
	uiWindow(w)->Handle = windowHandle;
	uiWindow(w)->Title = windowTitle;
	uiWindow(w)->SetTitle = windowSetTitle;
	uiWindow(w)->Show = windowShow;
	uiWindow(w)->Hide = windowHide;
	uiWindow(w)->OnClosing = windowOnClosing;
	uiWindow(w)->SetChild = windowSetChild;
	uiWindow(w)->Margined = windowMargined;
	uiWindow(w)->SetMargined = windowSetMargined;

	return uiWindow(w);
}
