#ifndef E_BOOK_META_VIEW_H
#define E_BOOK_META_VIEW_H

#include <glib.h>
#include <libebook/libebook.h>
#include <libedata-book/libedata-book.h>

/* Standard GObject macros */
#define E_TYPE_BOOK_META_VIEW                  (e_book_meta_view_get_type ())
#define E_BOOK_META_VIEW(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), E_TYPE_BOOK_META_VIEW, EBookMetaView))
#define E_IS_BOOK_META_VIEW(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), E_TYPE_BOOK_META_VIEW))
#define E_BOOK_META_VIEW_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), E_TYPE_BOOK_META_VIEW, EBookMetaViewClass))
#define E_IS_BOOK_META_VIEW_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), E_TYPE_BOOK_META_VIEW))
#define E_BOOK_META_VIEW_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), E_TYPE_BOOK_META_VIEW, EBookMetaViewClass))

G_BEGIN_DECLS

typedef struct _EBookMetaView EBookMetaView;
typedef struct _EBookMetaViewClass EBookMetaViewClass;
typedef struct _EBookMetaViewPrivate EBookMetaViewPrivate;

struct _EBookMetaView {
    GObject parent;
    EBookMetaViewPrivate *priv;
};

struct _EBookMetaViewClass {
    GObjectClass parent_class;
};

GType   e_book_meta_view_get_type             (void);
EBookMetaView*  e_book_meta_view_new          ();
void    e_book_meta_view_register_client      (EBookMetaView *self, EBookClient *client);
void    e_book_meta_view_unregister_client    (EBookMetaView *self, EBookClient *client);
void    e_book_meta_view_start_view           (EBookMetaView *self, EDataBookView *book_view);
void    e_book_meta_view_stop_view            (EBookMetaView *self, EDataBookView *book_view);

G_END_DECLS

#endif /* E_BOOK_META_VIEW_H */
