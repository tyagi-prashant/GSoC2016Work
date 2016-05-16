/*
 * gnome-keyring
 *
 * Copyright (C) 2008 Stefan Walter
 * Copyright (C) 2011 Collabora Ltd.
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
 *
 * Author: Stef Walter <stefw@collabora.co.uk>
 */

#ifndef __GCR_CERTIFICATE_CHOOSER_DIALOG_H__
#define __GCR_CERTIFICATE_CHOOSER_DIALOG_H__

#include <gck/gck.h>

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GCR_TYPE_CERTIFICATE_CHOOSER_DIALOG               (gcr_certificate_chooser_dialog_get_type ())
#define GCR_CERTIFICATE_CHOOSER_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_CAST ((obj), GCR_TYPE_CERTIFICATE_CHOOSER_DIALOG, GcrCertificateChooserDialog))
#define GCR_IS_CERTIFICATE_CHOOSER_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GCR_TYPE_CERTIFICATE_CHOOSER_DIALOG))

typedef struct _GcrCertificateChooserDialog GcrCertificateChooserDialog;

GType                   gcr_certificate_chooser_dialog_get_type     (void) G_GNUC_CONST;

GcrCertificateChooserDialog * gcr_certificate_chooser_dialog_new    (GtkWindow *parent);

gboolean                gcr_certificate_chooser_dialog_run               (GcrCertificateChooserDialog *self);

void                    gcr_certificate_chooser_dialog_run_async         (GcrCertificateChooserDialog *self,
                                                                     GCancellable *cancellable,
                                                                     GAsyncReadyCallback callback,
                                                                     gpointer user_data);

gboolean                gcr_certificate_chooser_dialog_run_finish        (GcrCertificateChooserDialog *self,
                                                                     GAsyncResult *result);

G_END_DECLS

#endif /* __GCR_CERTIFICATE_CHOOSER_DIALOG_H__ */
