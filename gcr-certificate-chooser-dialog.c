/*
 * gnome-keyring
 *
 * Copyright (C) 2008 Stefan Walter
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "gcr/gcr-icons.h"
#include "gcr/gcr-parser.h"

#include "gcr-dialog-util.h"
#include "gcr-secure-entry-buffer.h"
#include "gcr-certificate-chooser-dialog.h"
#include "gcr-viewer.h"
#include "gcr-viewer-widget.h"

#include "egg/egg-secure-memory.h"
#include <gtk/gtk.h>

#include <glib/gi18n-lib.h>

/**
 * SECTION:gcr-certificate-chooser-dialog
 * @title: GcrCertificateChooserDialog
 * @short_description: A dialog which allows selection of personal certificates
 *
 * A dialog which guides the user through selection of a certificate and
 * corresponding private key, located in files or PKCS\#11 tokens.
 */

/**
 * GcrCertificateChooserDialog:
 *
 * A certificate chooser dialog object.
 */

/**
 * GcrCertificateChooserDialogClass:
 *
 * Class for #GcrCertificateChooserDialog
 */
#define GCR_CERTIFICATE_CHOOSER_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GCR_TYPE_CERTIFICATE_CHOOSER_DIALOG, GcrCertificateChooserDialogClass))
#define GCR_IS_CERTIFICATE_CHOOSER_CLASS(klass)        (G_TYPE_CHECK_CLASS_TYPE ((klass), GCR_TYPE_CERTIFICATE_CHOOSER_DIALOG))
#define GCR_CERTIFICATE_CHOOSER_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GCR_TYPE_CERTIFICATE_CHOOSER_DIALOG, GcrCertificateChooserDialogClass))

enum {
	PROP_0,
	PROP_IMPORTER
};

struct _GcrCertificateChooserDialog {
       GtkDialog parent;
       GtkWidget *notebook;
       char *certificate_uri;
       char *key_uri;
       gboolean is_certificate_choosen;
       gboolean is_key_choosen;
       GtkWidget *default_button;
       GtkWidget *next_button;
       GtkWidget *previous_button;
       GtkWidget *page1_box;
       GtkWidget *page2_box;
       GtkWidget *page3_box;
       GtkWidget *page1_stack;
       GtkWidget *page2_stack;
       GtkWidget *page1_stack_switcher;
       GtkWidget *page2_stack_switcher;
       GtkWidget *page1_file_chooser;
       GtkWidget *page2_file_chooser;
       GcrViewerWidget *page1_viewer_widget;
       GcrViewerWidget *page2_viewer_widget;   
};

typedef struct _GcrCertificateChooserDialogClass GcrCertificateChooserDialogClass;

struct _GcrCertificateChooserDialogClass {
	GtkDialogClass parent;
};

G_DEFINE_TYPE (GcrCertificateChooserDialog, gcr_certificate_chooser_dialog, GTK_TYPE_DIALOG);

static void
on_next_button_clicked(GtkWidget *widget, gpointer *data)
{
       GcrCertificateChooserDialog *self = GCR_CERTIFICATE_CHOOSER_DIALOG (data);
       gchar *fname = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER (self->page1_file_chooser));

       printf("next-button %s\n", fname);
       g_free(self->certificate_uri);
       self->certificate_uri = fname;
}

static void
on_default_button_clicked(GtkWidget *widget, gpointer *data)
{
       GcrCertificateChooserDialog *self = GCR_CERTIFICATE_CHOOSER_DIALOG (data);
       gchar *fname = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER (self->page1_file_chooser));
       printf("default-button %s\n", fname);

       if (g_file_test(fname, G_FILE_TEST_IS_DIR)) {
	       gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER (self->page1_file_chooser), fname);
	       g_free(fname);
       } else {
	       /* This is used as a flag to the preview/added signals that this was *set* */
	       self->certificate_uri = fname;
	       gtk_file_chooser_set_filename(GTK_FILE_CHOOSER (self->page1_file_chooser), fname);
       }
}

static void
on_certificate_choosed(GcrViewerWidget *widget,
               GObject         *renderer,
               GcrParsed       *parsed,
               gpointer         *data)
{       GcrCertificateChooserDialog *self = GCR_CERTIFICATE_CHOOSER_DIALOG (data); 
        GckAttributes *attributes;
	GcrViewerWidget *viewer_widget = self->page1_viewer_widget;
	GcrViewer *viewer = gcr_viewer_widget_get_viewer(viewer_widget);
        char *filename = gtk_file_chooser_get_preview_filename(self->page1_file_chooser);

        if(g_strcmp0(self->key_uri,  filename) != 0)
            self->is_key_choosen = FALSE;
        if (self->certificate_uri && g_strcmp0(self->certificate_uri, filename) != 0) {
		g_free(self->certificate_uri);
		self->certificate_uri = NULL;
	}
        gulong class;
        attributes = gcr_renderer_get_attributes(renderer);
        
        if (gck_attributes_find_ulong (attributes, CKA_CLASS, &class) && class == CKO_CERTIFICATE) {
            self->is_certificate_choosen = TRUE;
            gtk_widget_set_sensitive(GTK_WIDGET(self->next_button), TRUE);
	    if (self->certificate_uri)
		    on_next_button_clicked(self->next_button, self);
	    else printf("not set\n");
        }

        if (gck_attributes_find_ulong (attributes, CKA_CLASS, &class) && class == CKO_PRIVATE_KEY){
            self->is_key_choosen = TRUE;
	    self->key_uri = filename;

        }
}        

static void
on_page1_file_activated(GtkWidget *widget, gpointer *data)
{
	GcrCertificateChooserDialog *self = GCR_CERTIFICATE_CHOOSER_DIALOG (data);
	GtkFileChooser *chooser = GTK_FILE_CHOOSER(widget);
	GcrViewerWidget *viewer_widget = self->page1_viewer_widget;
	GcrViewer *viewer = gcr_viewer_widget_get_viewer(viewer_widget);

	/* The user might have hit Ctrl-L and typed in a filename, and this
	   happens when they press Enter. It also happens when they double
	   click a file in the browser. */
	gchar *fname = gtk_file_chooser_get_filename(chooser);
	gtk_file_chooser_set_filename(chooser, fname);
	printf("fname chosen: %s\n", fname);
	g_free(fname);

	/* if Next button activated, then behave as if it's pressed */
}

	static void
on_page1_update_preview(GtkWidget *widget, gpointer *data)
{
	GcrCertificateChooserDialog *self = GCR_CERTIFICATE_CHOOSER_DIALOG (data);
	GtkFileChooser *chooser = GTK_FILE_CHOOSER(widget);
	GcrViewerWidget *viewer_widget = self->page1_viewer_widget;
	GcrViewer *viewer = gcr_viewer_widget_get_viewer(viewer_widget);

	gtk_widget_set_sensitive(GTK_WIDGET(self->next_button), FALSE);

	while (gcr_viewer_count_renderers(viewer))
		gcr_viewer_remove_renderer(viewer, gcr_viewer_get_renderer(viewer, 0));

	char *filename = gtk_file_chooser_get_preview_filename(chooser);
	if (!filename || g_file_test(filename, G_FILE_TEST_IS_DIR)) {
		gtk_file_chooser_set_preview_widget_active(chooser, FALSE);
		return;
        }
	if (self->certificate_uri && g_strcmp0(self->certificate_uri, filename) != 0) {
		g_free(self->certificate_uri);
		self->certificate_uri = NULL;
	}

	printf("Preview %s\n", filename);
	gcr_viewer_widget_load_file(viewer_widget, g_file_new_for_path(filename));
	gtk_file_chooser_set_preview_widget_active(chooser, TRUE);
	g_free(filename);
}

static void
gcr_certificate_chooser_dialog_constructed (GObject *obj)
{
	GcrCertificateChooserDialog *self = GCR_CERTIFICATE_CHOOSER_DIALOG (obj);
        GtkWidget *button, *content;
        gtk_window_set_title(GTK_WINDOW(self), "Certificate Chooser");
        G_OBJECT_CLASS (gcr_certificate_chooser_dialog_parent_class)->constructed (obj);
    
        self->notebook = gtk_notebook_new();
        gtk_notebook_set_tab_pos( GTK_NOTEBOOK(self->notebook),
                                 GTK_POS_LEFT );
        content = GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self)));
        gtk_container_add(GTK_CONTAINER(content), self->notebook);

	GtkFileFilter *filefilter = gtk_file_filter_new();
	gtk_file_filter_set_name (filefilter,"X.509 Certificate Format");
	gtk_file_filter_add_pattern (filefilter, "*.pem");
	gtk_file_filter_add_pattern (filefilter, "*.crt");
	gtk_file_filter_add_pattern (filefilter, "*.cer");
	gtk_file_filter_add_pattern (filefilter, "*.der");
	gtk_file_filter_add_pattern (filefilter, "*.crt");
	gtk_file_filter_add_pattern (filefilter, "*.p12");
	gtk_file_filter_add_pattern (filefilter, "*.pfx");

        /*Page1 Construction */
	self->default_button = gtk_button_new();
	g_signal_connect(GTK_WIDGET(self->default_button), "clicked", G_CALLBACK(on_default_button_clicked), self);
        gtk_container_add(GTK_CONTAINER(content), self->default_button);
	
        self->next_button = gtk_button_new_with_label("Next");
        self->page1_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        self->page1_stack = gtk_stack_new();
        self->page1_file_chooser = gtk_file_chooser_widget_new(GTK_FILE_CHOOSER_ACTION_OPEN);
        gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (self->page1_file_chooser), filefilter);
        gtk_stack_set_transition_type(GTK_STACK(self->page1_stack), GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
        gtk_stack_set_transition_duration(GTK_STACK(self->page1_stack), 1000);
        gtk_container_set_border_width(GTK_CONTAINER(self->page1_box), 1);
        gtk_notebook_append_page(GTK_NOTEBOOK(self->notebook), GTK_WIDGET(self->page1_box),GTK_WIDGET(gtk_label_new("Chooser a Certificate")));
        gtk_stack_add_titled(self->page1_stack, GTK_WIDGET(self->page1_file_chooser), "Files", "Chooser From File");
        gtk_stack_add_titled(self->page1_stack, GTK_WIDGET(gtk_label_new("Chooser after some time :)")), "Pkcs#11", "Chooser From PKCS#11");
        self->page1_stack_switcher = gtk_stack_switcher_new();
        gtk_stack_switcher_set_stack(self->page1_stack_switcher, self->page1_stack);
        gtk_box_pack_start(self->page1_box, self->page1_stack_switcher, TRUE, TRUE, 0);
        gtk_box_pack_start(self->page1_box, self->page1_stack, TRUE, TRUE, 0);
	self->page1_viewer_widget = gcr_viewer_widget_new();

	g_signal_connect(GTK_FILE_CHOOSER (self->page1_file_chooser), "update-preview", G_CALLBACK (on_page1_update_preview), self);
	g_signal_connect(GTK_FILE_CHOOSER (self->page1_file_chooser), "file-activated", G_CALLBACK (on_page1_file_activated), self);
	gtk_file_chooser_set_preview_widget(GTK_FILE_CHOOSER (self->page1_file_chooser), GTK_WIDGET (self->page1_viewer_widget));
        gtk_file_chooser_set_extra_widget(self->page1_file_chooser, self->next_button);
	gtk_widget_set_sensitive(GTK_WIDGET(self->next_button), FALSE);
        if(!gtk_file_chooser_get_preview_filename(self->page1_file_chooser)) 
	    gtk_file_chooser_set_preview_widget_active(self->page1_file_chooser, FALSE);

        g_signal_connect(GTK_WIDGET(self->page1_viewer_widget), "added", G_CALLBACK(on_certificate_choosed), self);
        g_signal_connect(GTK_WIDGET(self->next_button), "clicked", G_CALLBACK(on_next_button_clicked), self);
	gtk_widget_show_all(GTK_WIDGET (self));
	gtk_widget_hide(GTK_WIDGET(self->default_button));
	gtk_widget_set_can_default(GTK_WIDGET(self->default_button), TRUE);
	gtk_widget_grab_default(GTK_WIDGET(self->default_button));

        /*Page2 Construction */

        /*Page3 Construction */

	/* Add our various buttons */
	/*button = gtk_dialog_add_button (GTK_DIALOG (self), _("_Next"), GTK_RESPONSE_HELP);
	gtk_button_set_use_underline (GTK_BUTTON (button), TRUE);
	button = gtk_dialog_add_button (GTK_DIALOG (self), _("_OK"), GTK_RESPONSE_OK);
	gtk_button_set_use_underline (GTK_BUTTON (button), TRUE);
	gtk_dialog_set_default_response (GTK_DIALOG (self), GTK_RESPONSE_OK);
*/
       
	gtk_window_set_modal (GTK_WINDOW (self), TRUE);
}

static void
gcr_certificate_chooser_dialog_init (GcrCertificateChooserDialog *self)
{

}

static void
gcr_certificate_chooser_dialog_finalize (GObject *obj)
{
	GcrCertificateChooserDialog *self = GCR_CERTIFICATE_CHOOSER_DIALOG (obj);

	G_OBJECT_CLASS (gcr_certificate_chooser_dialog_parent_class)->finalize (obj);
}

static void
gcr_certificate_chooser_dialog_class_init (GcrCertificateChooserDialogClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->constructed = gcr_certificate_chooser_dialog_constructed;
	gobject_class->finalize = gcr_certificate_chooser_dialog_finalize;
}

/**
 * gcr_certificate_chooser_dialog_new:
 * @parent: the parent window
 *
 * Create a new certxificate chooser dialog.
 *
 * Returns: (transfer full): A new #GcrCertificateChooserDialog object
 */
GcrCertificateChooserDialog *
gcr_certificate_chooser_dialog_new (GtkWindow *parent)
{
	GcrCertificateChooserDialog *dialog;

	g_return_val_if_fail (parent == NULL || GTK_IS_WINDOW (parent), NULL);

	dialog = g_object_new (GCR_TYPE_CERTIFICATE_CHOOSER_DIALOG,
	                       "transient-for", parent,
	                       NULL);

	return g_object_ref_sink (dialog);
}


gboolean
gcr_certificate_chooser_dialog_run (GcrCertificateChooserDialog *self)
{
	gboolean ret = FALSE;

	g_return_val_if_fail (GCR_IS_CERTIFICATE_CHOOSER_DIALOG (self), FALSE);

	if (gtk_dialog_run (GTK_DIALOG (self)) == GTK_RESPONSE_OK) {
		ret = TRUE;
	}

	gtk_widget_hide (GTK_WIDGET (self));

	return ret;
}

void
gcr_certificate_chooser_dialog_run_async (GcrCertificateChooserDialog *self,
                                     GCancellable *cancellable,
                                     GAsyncReadyCallback callback,
                                     gpointer user_data)
{
	g_return_if_fail (GCR_IS_CERTIFICATE_CHOOSER_DIALOG (self));

	_gcr_dialog_util_run_async (GTK_DIALOG (self), cancellable, callback, user_data);
}

gboolean
gcr_certificate_chooser_dialog_run_finish (GcrCertificateChooserDialog *self,
                                      GAsyncResult *result)
{
	gint response;

	g_return_val_if_fail (GCR_IS_CERTIFICATE_CHOOSER_DIALOG (self), FALSE);

	response = _gcr_dialog_util_run_finish (GTK_DIALOG (self), result);

	gtk_widget_hide (GTK_WIDGET (self));

	return (response == GTK_RESPONSE_OK) ? TRUE : FALSE;
}

