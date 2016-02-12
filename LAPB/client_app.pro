TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

DESTDIR += "../bin"

INCLUDEPATH += ./include ./lapb_lib

SOURCES += ./src/client_main.c \
    ./lapb_lib/lapb_iface.c \
    ./lapb_lib/lapb_in.c \
    ./lapb_lib/lapb_out.c \
    ./lapb_lib/lapb_subr.c \
    ./lapb_lib/lapb_timer.c \
    ./src/tcp_client.c \
    ./src/my_timer.c \
    ./src/common.c \
    lapb_lib/lapb_queue.c

HEADERS += \
    ./lapb_lib/net_lapb.h \
    ./include/tcp_client.h \
    ./include/my_timer.h \
    ./include/common.h \
    lapb_lib/lapb_queue.h

LIBS += -pthread
