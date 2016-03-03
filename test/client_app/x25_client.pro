TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

DESTDIR += "../../../bin"

INCLUDEPATH += ../include ../../lib/include ../../lib/export
INCLUDEPATH += ../../lib/src

SOURCES += main.c \
    ../../lib/src/af_x25.c \
    ../../lib/src/sysctl_net_x25.c \
    ../../lib/src/x25_dev.c \
    ../../lib/src/x25_facilities.c \
    ../../lib/src/x25_forward.c \
    ../../lib/src/x25_in.c \
    ../../lib/src/x25_link.c \
    ../../lib/src/x25_out.c \
    ../../lib/src/x25_proc.c \
    ../../lib/src/x25_route.c \
    ../../lib/src/x25_subr.c \
    ../../lib/src/x25_timer.c

HEADERS += \
    ../../lib/include/x25.h
