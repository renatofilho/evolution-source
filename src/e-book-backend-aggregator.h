#ifndef E_BOOK_BACKEND_AGGREGATOR_H
#define E_BOOK_BACKEND_AGGREGATOR_H

#include <libedata-book/libedata-book.h>

/* Standard GObject macros */
#define E_TYPE_BOOK_BACKEND_AGGREGATOR \
    (e_book_backend_aggregator_get_type ())
#define E_BOOK_BACKEND_AGGREGATOR(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST \
    ((obj), E_TYPE_BOOK_BACKEND_AGGREGATOR, EBookBackendAggregator))
#define E_BOOK_BACKEND_AGGREGATOR_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_CAST \
    ((cls), E_TYPE_BOOK_BACKEND_AGGREGATOR, EBookBackendAggregatorClass))
#define E_IS_BOOK_BACKEND_AGGREGATOR(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE \
    ((obj), E_TYPE_BOOK_BACKEND_AGGREGATOR))
#define E_IS_BOOK_BACKEND_AGGREGATOR_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_TYPE \
    ((cls), E_TYPE_BOOK_BACKEND_AGGREGATOR))
#define E_BOOK_BACKEND_AGGREGATOR_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS \
    ((obj), E_TYPE_BOOK_BACKEND_AGGREGATOR, EBookBackendAggregatorClass))

G_BEGIN_DECLS

typedef struct _EBookBackendAggregator EBookBackendAggregator;
typedef struct _EBookBackendAggregatorClass EBookBackendAggregatorClass;
typedef struct _EBookBackendAggregatorPrivate EBookBackendAggregatorPrivate;

struct _EBookBackendAggregator {
    EBookBackend parent;
    EBookBackendAggregatorPrivate *priv;
};

struct _EBookBackendAggregatorClass {
    EBookBackendClass parent_class;
};

GType   e_book_backend_aggregator_get_type  (void);

G_END_DECLS

#endif /* E_BOOK_BACKEND_AGGREGATOR_H */

