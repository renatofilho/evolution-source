project(agregator-eds-src)

set(AGGREGATOR_BACKEND ebookbackendaggregator)

set(AGGREGATOR_BACKEND_SRCS
    e-book-backend-aggregator.c
    e-book-backend-aggregator-factory.c
    e-book-backend-aggregator.h
)

add_library(${AGGREGATOR_BACKEND} MODULE
    ${AGGREGATOR_BACKEND_SRCS}
)

include_directories(
    ${CMAKE_BINARY_DIR}
    ${GLIB_INCLUDE_DIRS}
    ${GIO_INCLUDE_DIRS}
    ${EBOOK_INCLUDE_DIRS}
    ${EDATASERVER_INCLUDE_DIRS}
)

target_link_libraries(${AGGREGATOR_BACKEND}
    ${GLIB_LIBRARIES}
    ${EBOOK_LIBRARIES}
    ${EDATASERVER_LIBRARIES}
)

execute_process(
    COMMAND pkg-config --variable=backenddir libedata-book-1.2
    OUTPUT_VARIABLE EDS_BACKEND_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

install(TARGETS ${AGGREGATOR_BACKEND}
    LIBRARY DESTINATION ${EDS_BACKEND_DIR}
)
