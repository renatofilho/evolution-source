#include "config.h"
#include "e-book-backend-aggregator.h"

#define FACTORY_NAME "aggregator"

typedef EBookBackendFactory EBookBackendAggregatorFactory;
typedef EBookBackendFactoryClass EBookBackendAggregatorFactoryClass;

/* Module Entry Points */
void e_module_load (GTypeModule *type_module);
void e_module_unload (GTypeModule *type_module);

/* Forward Declarations */
GType e_book_backend_aggregator_factory_get_type (void);

G_DEFINE_DYNAMIC_TYPE (
    EBookBackendAggregatorFactory,
    e_book_backend_aggregator_factory,
    E_TYPE_BOOK_BACKEND_FACTORY)

static void
e_book_backend_aggregator_factory_class_init (EBookBackendFactoryClass *klass)
{
    g_debug("Aggregator factory init: %s", FACTORY_NAME);
    klass->factory_name = FACTORY_NAME;
    klass->backend_type = E_TYPE_BOOK_BACKEND_AGGREGATOR;
}

static void
e_book_backend_aggregator_factory_class_finalize (EBookBackendFactoryClass *klass)
{
}

static void
e_book_backend_aggregator_factory_init (EBookBackendFactory *factory)
{
}

G_MODULE_EXPORT void
e_module_load (GTypeModule *type_module)
{
    e_book_backend_aggregator_factory_register_type (type_module);
}

G_MODULE_EXPORT void
e_module_unload (GTypeModule *type_module)
{
}

