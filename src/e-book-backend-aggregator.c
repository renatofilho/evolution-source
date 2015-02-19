#include "config.h"

#include <glib/gstdio.h>
#include <glib/gi18n-lib.h>

#include <libebook/libebook.h>

#include "e-book-backend-aggregator.h"


#define E_BOOK_BACKEND_AGGREGATOR_GET_PRIVATE(obj) \
    (G_TYPE_INSTANCE_GET_PRIVATE \
    ((obj), E_TYPE_BOOK_BACKEND_AGGREGATOR, EBookBackendAggregatorPrivate))

#define E_BOOK_BACKEND_AGGREGATOR_NAME "aggregator"


/* Forward Declarations */
static void e_book_backend_aggregator_initable_init
                        (GInitableIface *iface);

G_DEFINE_TYPE_WITH_CODE (
    EBookBackendAggregator,
    e_book_backend_aggregator,
    E_TYPE_BOOK_BACKEND,
    G_IMPLEMENT_INTERFACE (
        G_TYPE_INITABLE,
        e_book_backend_aggregator_initable_init))

struct _EBookBackendAggregatorPrivate {
    GObject *obj;
};

static gpointer
book_backend_aggregator_get_view_thread (EBookClient *client)
{
    EBookClientView *view;
    EBookQuery *query;
    gchar *sexp;
    GError *error = NULL;

    query = e_book_query_any_field_contains("");
    sexp = e_book_query_to_string(query);
    e_book_query_unref (query);
    e_book_client_get_view_sync (E_BOOK_CLIENT (client),
                                 sexp,
                                 &view,
                                 NULL,
                                 &error);
    if (error) {
        g_warning ("Fail to get view: [%s]", error->message);
        g_clear_error (&error);
    } else {
        e_book_client_view_start (E_BOOK_CLIENT_VIEW (view), &error);
        if (error) {
            g_warning ("Fail to start view: %s", error->message);
        } else {
            g_debug ("VIEW STARTED");
        }
    }
    g_free (sexp);
    return NULL;
}

static gboolean
book_backend_aggregator_register_client (EBookBackendAggregator *self,
                                         EClient *client)
{
    g_debug ("book_backend_aggregator_register_client: %d", __LINE__);
    GThread *thread;

    g_object_ref (client);
    thread = g_thread_new (NULL,
                           (GThreadFunc) book_backend_aggregator_get_view_thread,
                           client);

    g_thread_unref (thread);
    return TRUE;
}

static void
book_backend_aggregator_client_connected_cb (GObject *source_object,
                                             GAsyncResult *result,
                                             EBookBackendAggregator *self)
{
    g_debug (G_STRFUNC);

    EClient *client;
    GError *error = NULL;

    g_return_if_fail (self != NULL);

    client = e_book_client_connect_finish (result, &error);

    /* Sanity check. */
    g_return_if_fail (
        ((client != NULL) && (error == NULL)) ||
        ((client == NULL) && (error != NULL)));

    if (error != NULL) {
        g_warning ("%s: %s", G_STRFUNC, error->message);
        g_error_free (error);
        return;
    }

    book_backend_aggregator_register_client (self, client);
    g_object_unref (client);
}

static void
book_backend_aggregator_source_added (ESourceRegistry *registry,
                                      ESource *source,
                                      EBookBackendAggregator *self)
{
    ESourceBackend *extension;
    const gchar *extension_name;

    if (!e_source_get_enabled (source)) {
        g_debug ("Skip source (not enabled): %s", e_source_get_display_name (source));
        return;
    }

    /* We're only interested in address books. */
    extension_name = E_SOURCE_EXTENSION_ADDRESS_BOOK;
    if (!e_source_has_extension (source, extension_name)) {
        g_debug ("Skip source (no addressbook): %s", e_source_get_display_name (source));
        return;
    }

    extension = e_source_get_extension (source, extension_name);
    if (extension == NULL) {
        g_debug ("Skip source (extension is null): %s", e_source_get_display_name (source));
        return;
    }

    if (g_strcmp0(e_source_backend_get_backend_name (extension),
                  E_BOOK_BACKEND_AGGREGATOR_NAME) == 0)  {
        g_debug ("Skip source: %s", e_source_get_display_name (source));
        return;
    }

    e_book_client_connect (source,
                           NULL,
                           (GAsyncReadyCallback) book_backend_aggregator_client_connected_cb,
                           self);
}

static void
book_backend_aggregator_source_removed (ESourceRegistry *registry,
                                        ESource *source,
                                        EBookBackendAggregator *self)
{
    // TODO
    return;
}

static gboolean
book_backend_aggregator_load_external_sources (gpointer user_data)
{
    EBookBackendAggregator *self;
    ESourceRegistry *registry;
    GList *sources;
    GList *link;

    self = E_BOOK_BACKEND_AGGREGATOR (user_data);
    registry = e_book_backend_get_registry(E_BOOK_BACKEND (self));
    sources = e_source_registry_list_sources (registry,
                                              E_SOURCE_EXTENSION_ADDRESS_BOOK);

    for(link = sources; link != NULL; link = link->next) {
        book_backend_aggregator_source_added (registry,
                                              E_SOURCE (link->data),
                                              self);
    }

    g_list_free_full (sources, (GDestroyNotify) g_object_unref);
    return FALSE;
}

static void
book_backend_aggregator_dispose (GObject *object)
{
    G_OBJECT_CLASS (e_book_backend_aggregator_parent_class)->dispose (object);
}

static void
book_backend_aggregator_finalize (GObject *object)
{
    G_OBJECT_CLASS (e_book_backend_aggregator_parent_class)->finalize (object);
}

static void
book_backend_aggregator_constructed (GObject *object)
{
    /* Load address book sources from an idle callback
     * to avoid deadlocking e_data_factory_ref_backend(). */
    g_idle_add_full(
        G_PRIORITY_DEFAULT_IDLE,
        book_backend_aggregator_load_external_sources,
        g_object_ref (object),
        (GDestroyNotify) g_object_unref);

    /* Chain up to parent's constructed() method. */
    G_OBJECT_CLASS (e_book_backend_aggregator_parent_class)->constructed (object);
}

static gchar *
book_backend_aggregator_get_backend_property (EBookBackend *backend,
                                        const gchar *prop_name)
{
    g_return_val_if_fail (prop_name != NULL, NULL);

    if (g_str_equal (prop_name, CLIENT_BACKEND_PROPERTY_CAPABILITIES)) {
        return g_strdup ("local,do-initial-query,bulk-adds,bulk-modifies,bulk-removes,contact-lists");

    } else if (g_str_equal (prop_name, BOOK_BACKEND_PROPERTY_REQUIRED_FIELDS)) {
        return g_strdup (e_contact_field_name (E_CONTACT_FILE_AS));

    } else if (g_str_equal (prop_name, BOOK_BACKEND_PROPERTY_SUPPORTED_FIELDS)) {
        GString *fields;
        gint ii;

        fields = g_string_sized_new (1024);
        for (ii = 1; ii < E_CONTACT_FIELD_LAST; ii++) {
            if (fields->len > 0)
                g_string_append_c (fields, ',');
            g_string_append (fields, e_contact_field_name (ii));
        }

        return g_string_free (fields, FALSE);

    }

    /* Chain up to parent's get_backend_property() method. */
    return E_BOOK_BACKEND_CLASS (e_book_backend_aggregator_parent_class)->
        get_backend_property (backend, prop_name);
}


static gboolean
book_backend_aggregator_open_sync (EBookBackend *backend,
                             GCancellable *cancellable,
                             GError **error)
{
    return TRUE;
}

static gboolean
book_backend_aggregator_create_contacts_sync (EBookBackend *backend,
                                        const gchar * const *vcards,
                                        GQueue *out_contacts,
                                        GCancellable *cancellable,
                                        GError **error)
{
    return TRUE;
}

static gboolean
book_backend_aggregator_modify_contacts_sync (EBookBackend *backend,
                                        const gchar * const *vcards,
                                        GQueue *out_contacts,
                                        GCancellable *cancellable,
                                        GError **error)
{
    return TRUE;
}

static gboolean
book_backend_aggregator_remove_contacts_sync (EBookBackend *backend,
                                        const gchar * const *uids,
                                        GCancellable *cancellable,
                                        GError **error)
{
    return TRUE;
}


static EContact *
book_backend_aggregator_get_contact_sync (EBookBackend *backend,
                                    const gchar *uid,
                                    GCancellable *cancellable,
                                    GError **error)
{
    return NULL;
}

static gboolean
book_backend_aggregator_get_contact_list_sync (EBookBackend *backend,
                                         const gchar *query,
                                         GQueue *out_contacts,
                                         GCancellable *cancellable,
                                         GError **error)
{
    return FALSE;
}


static void
book_backend_aggregator_start_view (EBookBackend *backend,
                                    EDataBookView *book_view)
{
    g_debug ("book_backend_aggregator_start_view\n");
    e_data_book_view_notify_complete (book_view, NULL /* Success */);
}

static void
book_backend_aggregator_stop_view (EBookBackend *backend,
                                   EDataBookView *book_view)
{
    g_debug ("book_backend_aggregator_stop_view");
}

static void
book_backend_aggregator_sync (EBookBackend *backend)
{
}

static gboolean
book_backend_aggregator_set_locale (EBookBackend *backend,
                              const gchar *locale,
                              GCancellable *cancellable,
                              GError **error)
{
    return TRUE;
}

static gchar *
book_backend_aggregator_dup_locale (EBookBackend *backend)
{
    return NULL;
}

static EDataBookCursor *
book_backend_aggregator_create_cursor (EBookBackend *backend,
                                 EContactField *sort_fields,
                                 EBookCursorSortType *sort_types,
                                 guint n_fields,
                                 GError **error)
{
    return NULL;
}

static gboolean
book_backend_aggregator_delete_cursor (EBookBackend *backend,
                                 EDataBookCursor *cursor,
                                 GError **error)
{
    return TRUE;
}

static gboolean
book_backend_aggregator_initable_init (GInitable *initable,
                                 GCancellable *cancellable,
                                 GError **error)
{
    return TRUE;
}

static void
e_book_backend_aggregator_class_init (EBookBackendAggregatorClass *klass)
{
    g_debug("e_book_backend_aggregator_class_init");
    GObjectClass *object_class;
    EBookBackendClass *backend_class;

    g_type_class_add_private (klass, sizeof (EBookBackendAggregatorPrivate));

    object_class = G_OBJECT_CLASS (klass);
    object_class->dispose = book_backend_aggregator_dispose;
    object_class->finalize = book_backend_aggregator_finalize;
    object_class->constructed = book_backend_aggregator_constructed;

    backend_class = E_BOOK_BACKEND_CLASS (klass);
    backend_class->use_serial_dispatch_queue = TRUE;

    backend_class->get_backend_property = book_backend_aggregator_get_backend_property;

    backend_class->open_sync = book_backend_aggregator_open_sync;
    backend_class->create_contacts_sync = book_backend_aggregator_create_contacts_sync;
    backend_class->modify_contacts_sync = book_backend_aggregator_modify_contacts_sync;
    backend_class->remove_contacts_sync = book_backend_aggregator_remove_contacts_sync;
    backend_class->get_contact_sync = book_backend_aggregator_get_contact_sync;
    backend_class->get_contact_list_sync = book_backend_aggregator_get_contact_list_sync;
    backend_class->start_view = book_backend_aggregator_start_view;
    backend_class->stop_view = book_backend_aggregator_stop_view;
    backend_class->sync = book_backend_aggregator_sync;
    backend_class->set_locale = book_backend_aggregator_set_locale;
    backend_class->dup_locale = book_backend_aggregator_dup_locale;
    backend_class->create_cursor = book_backend_aggregator_create_cursor;
    backend_class->delete_cursor = book_backend_aggregator_delete_cursor;
}

static void
e_book_backend_aggregator_initable_init (GInitableIface *iface)
{
    iface->init = book_backend_aggregator_initable_init;
}

static void
e_book_backend_aggregator_init (EBookBackendAggregator *backend)
{
    backend->priv = E_BOOK_BACKEND_AGGREGATOR_GET_PRIVATE (backend);
}

