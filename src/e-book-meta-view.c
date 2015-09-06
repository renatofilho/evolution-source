#include "config.h"

#include <glib.h>
#include "e-book-meta-view.h"

struct _EBookMetaViewPrivate
{
    GCancellable *m_cancel;
    GThread *m_thread;
    GMainContext *m_context;
    GMainLoop *m_loop;
    GSList *m_clients;

    GHashTable *m_views;
};

typedef struct _JobData JobData;
struct _JobData
{
    EBookMetaView *m_self;
    EDataBookView *m_book_view;
    GSList *m_views;
    GSource *m_source;
};

typedef struct _ViewData ViewData;
struct _ViewData
{
    EBookClient *m_client;
    guint m_percent;
    EBookClientView *m_view;
    JobData *m_job;
};

G_DEFINE_TYPE_WITH_PRIVATE (EBookMetaView, e_book_meta_view, G_TYPE_OBJECT)

gpointer    e_book_meta_view_thread                 (EBookMetaView *self);
gboolean    e_book_meta_view_start_view_on_thread   (JobData *j_data);
gboolean    e_book_meta_view_stop_view_on_thread    (JobData *j_data);
guint       e_book_meta_view_percent                (GSList *views);
void        e_book_meta_view_on_objects_added       (EBookClientView *client_view,
                                                     const GSList *objects,
                                                     JobData *data);
void        e_book_meta_view_on_objects_modified    (EBookClientView *client_view,
                                                     const GSList *objects,
                                                     JobData *data);
void        e_book_meta_view_on_objects_removed     (EBookClientView *client_view,
                                                     const GSList *uids,
                                                     JobData *data);
void        e_book_meta_view_on_progress            (EBookClientView *client_view,
                                                     guint percent,
                                                     const gchar *message,
                                                     ViewData *data);
void        e_book_meta_view_on_complete            (EBookClientView *client_view,
                                                     const GError *error,
                                                     ViewData *data);
void        view_data_free (ViewData *data);
void        job_data_free (JobData *data);



// gobject related
static void
e_book_meta_view_dispose (GObject *gobject)
{
    G_OBJECT_CLASS (e_book_meta_view_parent_class)->dispose (gobject);
}

static void
e_book_meta_view_finalize (GObject *gobject)
{
    G_OBJECT_CLASS (e_book_meta_view_parent_class)->finalize (gobject);
}

static void
e_book_meta_view_class_init (EBookMetaViewClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->dispose = e_book_meta_view_dispose;
    gobject_class->finalize = e_book_meta_view_finalize;
}

static void
e_book_meta_view_init (EBookMetaView *self)
{
    self->priv = e_book_meta_view_get_instance_private (self);

    self->priv->m_views = g_hash_table_new_full (g_direct_hash,
                                                 g_direct_equal,
                                                 (GDestroyNotify) g_object_unref,
                                                 (GDestroyNotify) job_data_free);
    self->priv->m_cancel = g_cancellable_new ();
    self->priv->m_thread = g_thread_new("meta-view-thread", (GThreadFunc) e_book_meta_view_thread, self);

}

// public

EBookMetaView *e_book_meta_view_new()
{
    return g_object_new (E_TYPE_BOOK_META_VIEW, NULL);
}

void
e_book_meta_view_register_client(EBookMetaView *self, EBookClient *client)
{
    g_debug (__PRETTY_FUNCTION__);

    gint found = g_slist_index (self->priv->m_clients, client);
    if (found != -1) {
        g_warning ("Client already registered: %p", client);
    } else {
        g_object_ref (client);
        self->priv->m_clients = g_slist_append (self->priv->m_clients, client);
    }
}

void
e_book_meta_view_unregister_client(EBookMetaView *self, EBookClient *client)
{
    g_debug (__PRETTY_FUNCTION__);

    gint found = g_slist_index (self->priv->m_clients, client);
    if (found != -1) {

        // close all views
        GList *l;
        for (l = g_hash_table_get_values (self->priv->m_views); l != NULL; l = l->next) {
            GSList *v;
            GSList *view_cpy;
            JobData *j_data;

            j_data = (JobData *) l->data;
            view_cpy = g_slist_copy (j_data->m_views);

            for (v = view_cpy; v; v = v->next) {
                ViewData *v_data = (ViewData *) v->data;
                e_book_client_view_stop (v_data->m_view, NULL);
            }

            g_slist_free (view_cpy);
        }

        // remove client
        self->priv->m_clients = g_slist_remove (self->priv->m_clients, client);
        g_object_unref (client);
    }
}

void
e_book_meta_view_start_view(EBookMetaView *self, EDataBookView *book_view)
{
    g_debug (__PRETTY_FUNCTION__);

    GSList *l;
    gpointer *found;

    found = g_hash_table_lookup (self->priv->m_views, book_view);
    if (found) {
        g_warning ("View already started");
        return;
    }

    if (g_slist_length (self->priv->m_clients) == 0) {
        g_debug ("Empty clients");
        return;
    }

    g_object_ref (book_view);

    JobData *j_data = g_new0 (JobData, 1);
    j_data->m_self = self;
    j_data->m_book_view = book_view;
    j_data->m_source = g_idle_source_new();

    for (l = self->priv->m_clients; l; l = l->next) {
        ViewData *data = g_new0 (ViewData, 1);

        data->m_job = j_data;
        data->m_client = E_BOOK_CLIENT(l->data);
        j_data->m_views = g_slist_append(j_data->m_views, data);
    }

    g_hash_table_insert (self->priv->m_views, book_view, j_data);
    g_source_set_callback (j_data->m_source, (GSourceFunc) e_book_meta_view_start_view_on_thread, j_data, NULL);
    g_source_attach (j_data->m_source, self->priv->m_context);
}

void
e_book_meta_view_stop_view (EBookMetaView *self, EDataBookView *book_view)
{
    g_debug (__PRETTY_FUNCTION__);

    JobData *found = (JobData *) g_hash_table_lookup (self->priv->m_views, book_view);
    if (found) {
        g_warning ("Fail to stop view: View not found.");
        return;
    }

    g_source_set_callback (found->m_source, (GSourceFunc) e_book_meta_view_stop_view_on_thread, found, NULL);
}

// private
gpointer
e_book_meta_view_thread (EBookMetaView *self)
{
    g_debug (__PRETTY_FUNCTION__);

    self->priv->m_context = g_main_context_new ();
    self->priv->m_loop = g_main_loop_new (self->priv->m_context, FALSE);
    g_main_context_push_thread_default (self->priv->m_context);
    g_main_loop_run (self->priv->m_loop);

    return NULL;
}

gboolean
e_book_meta_view_start_view_on_thread (JobData *j_data)
{
    g_debug (__PRETTY_FUNCTION__);
    GSList *l;
    const gchar *sexp;

    sexp = e_book_backend_sexp_text (e_data_book_view_get_sexp (j_data->m_book_view));

    for (l = j_data->m_views; l != NULL; l = l->next) {
        ViewData *data;
        EBookClientView *view = NULL;
        GError *error = NULL;

        data = (ViewData*) l->data;
        e_book_client_get_view_sync (E_BOOK_CLIENT (data->m_client),
                                     sexp,
                                     &view,
                                     j_data->m_self->priv->m_cancel,
                                     &error);

        if (error) {
            g_warning ("Fail to get view: %s", error->message);
            g_error_free (error);
        } else {
            data->m_view = view;
            g_signal_connect (view,
                              "objects_added",
                              G_CALLBACK (e_book_meta_view_on_objects_added),
                              j_data);
            g_signal_connect (view,
                              "objects_modified",
                              G_CALLBACK (e_book_meta_view_on_objects_modified),
                              j_data);
            g_signal_connect (view,
                              "objects_removed",
                              G_CALLBACK (e_book_meta_view_on_objects_removed),
                              j_data);
            g_signal_connect (view,
                              "progress",
                              G_CALLBACK (e_book_meta_view_on_progress),
                              data);
            g_signal_connect (view,
                              "complete",
                              G_CALLBACK (e_book_meta_view_on_complete),
                              data);
            // FIXME: stating the view causes the server to hang and no more contacts are returned on the client
            //e_book_client_view_start (E_BOOK_CLIENT_VIEW (view), &error);
            if (error) {
                g_warning ("Fail to start view: %s", error->message);
            } else {
                g_debug ("VIEW STARTED: client: %p, view: %p", data->m_client, data->m_view);

            }
        }
    }

    return FALSE;
}

gboolean
e_book_meta_view_stop_view_on_thread (JobData *j_data)
{
    g_debug (__PRETTY_FUNCTION__);

    GSList *l;
    GSList *views_cpy;

    views_cpy = g_slist_copy (j_data->m_views);
    for(l = views_cpy; l; l = l->next) {
        GError *error;
        ViewData *data;

        error = NULL;
        data = (ViewData *) l->data;
        e_book_client_view_stop (data->m_view, &error);
        if (error) {
            g_warning ("Fail to stop view: %s", error->message);
            g_error_free (error);
        }
    }

    g_hash_table_remove (j_data->m_self->priv->m_views, j_data->m_book_view);

    return FALSE;
}

guint
e_book_meta_view_percent (GSList *views)
{
    GSList *view;
    guint total_percent = 0;

    for (view = views; view; view = view->next) {
         total_percent += ((ViewData *) view->data)->m_percent;
    }

    if (total_percent > 0) {
        return (total_percent / g_slist_length (views));
    }

    return 0;
}

void
e_book_meta_view_on_objects_added (EBookClientView *client_view,
                                   const GSList *objects,
                                   JobData *data)
{
    g_debug (__PRETTY_FUNCTION__);

    const GSList *c;
    for (c = objects; c; c = c->next) {
        EContact *contact = c->data;
        g_object_ref (contact);
        g_debug ("Contact added: %s", (const gchar*) e_contact_get_const (E_CONTACT (c->data), E_CONTACT_UID));
        e_data_book_view_notify_update (data->m_book_view, E_CONTACT (c->data));
    }
}

void
e_book_meta_view_on_objects_modified (EBookClientView *client_view,
                                      const GSList *objects,
                                      JobData *data)
{
    g_debug (__PRETTY_FUNCTION__);

    const GSList *c;
    for (c = objects; c; c = c->next) {
        e_data_book_view_notify_update (data->m_book_view, E_CONTACT(c->data));
    }
}

void
e_book_meta_view_on_objects_removed (EBookClientView *client_view,
                                     const GSList *uids,
                                     JobData *data)
{
    g_debug (__PRETTY_FUNCTION__);

    const GSList *c;
    for (c = uids; c; c = c->next) {
        e_data_book_view_notify_remove (data->m_book_view, c->data);
    }
}

void
e_book_meta_view_on_progress (EBookClientView *client_view,
                              guint percent,
                              const gchar *message,
                              ViewData *data)
{
    g_debug (__PRETTY_FUNCTION__);
    g_debug ("PROGRESS: %p, %d, %s", client_view, percent, message);

    data->m_percent = percent;
    e_data_book_view_notify_progress (data->m_job->m_book_view,
                                      e_book_meta_view_percent (data->m_job->m_views),
                                      message);
}

void
e_book_meta_view_on_complete (EBookClientView *client_view,
                              const GError *error,
                              ViewData *data)
{
    g_debug (__PRETTY_FUNCTION__);
    g_debug ("COMPLETE: %p", client_view);

    JobData *j_data;

    j_data = data->m_job;
    j_data->m_views = g_slist_remove (j_data->m_views, data);

    g_debug ("view size: %d", g_slist_length (j_data->m_views));
    if (g_slist_length (j_data->m_views) == 0) {
        g_debug ("notify DONE");
        e_data_book_view_notify_complete (j_data->m_book_view, NULL);
        g_debug ("notify DONE: END");
    }

    view_data_free (data);
}

void
view_data_free (ViewData *data)
{
    g_free (data);
}

void
job_data_free (JobData *data)
{
    g_object_unref (data->m_source);
    g_slist_free_full (data->m_views, (GDestroyNotify) view_data_free);

    g_free (data);
}

