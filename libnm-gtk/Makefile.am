SUBDIRS = wireless-security

NULL=

noinst_LTLIBRARIES = libnm-gtk.la

libnm_gtk_la_CPPFLAGS = \
	-DUIDIR=\""$(uidir)"\" \
	-DICONDIR=\""$(pkgdatadir)/icons"\" \
	$(LIBNM_GTK_CFLAGS) \
	-I${top_builddir}/marshallers \
	-I${top_srcdir}/libnm-gtk/wireless-security \
	$(NULL)

libnm_gtk_la_LIBADD = \
	$(LIBNM_GTK_LIBS) \
	${top_builddir}/marshallers/libmarshallers.la \
	${top_builddir}/libnm-gtk/wireless-security/libwireless-security.la \
	$(NULL)

# libnmincludedir = $(includedir)/libnm-gtk

noinst_HEADERS = \
	nm-bt-item.h \
	nm-bt-provider.h \
	nm-cdma-item.h \
	nm-cdma-provider.h \
	nm-connection-item.h \
	nm-connection-list.h \
	nm-connection-model.h \
	nm-device-handler.h \
	nm-device-item.h \
	nm-device-model.h \
	nm-device-provider.h \
	nm-ethernet-item.h \
	nm-ethernet-provider.h \
	nm-gconf-connection.h \
	nm-gconf-settings.h \
	nm-gsm-item.h \
	nm-gsm-pin-request-item.h \
	nm-gsm-provider.h \
	nm-item-provider.h \
	nm-list-item.h \
	nm-list-model.h \
	nm-mobile-providers.h \
	nm-status-icon.h \
	nm-status-model.h \
	nm-wifi-item.h \
	nm-wifi-provider.h \
	$(NULL)

libnm_gtk_la_SOURCES = \
	gconf-helpers.c \
	gconf-helpers.h \
	gconf-upgrade.c	\
	gconf-upgrade.h \
	libnm-gtk-gsm-device.c \
	libnm-gtk-gsm-device.h \
	nm-bt-item.c \
	nm-bt-provider.c \
	nm-cdma-item.c \
	nm-cdma-provider.c \
	nm-connection-item.c \
	nm-connection-list.c \
	nm-connection-model.c \
	nm-device-handler.c \
	nm-device-item.c \
	nm-device-model.c \
	nm-device-provider.c \
	nm-ethernet-item.c \
	nm-ethernet-provider.c \
	nm-gconf-connection.c \
	nm-gconf-settings.c \
	nm-gsm-item.c \
	nm-gsm-pin-request-item.c \
	nm-gsm-provider.c \
	nm-icon-cache.c \
	nm-icon-cache.h \
	nm-item-provider.c \
	nm-list-item.c \
	nm-list-model.c \
	nm-mobile-providers.c \
	nm-status-icon.c \
	nm-status-model.c \
	nm-wifi-item.c \
	nm-wifi-provider.c \
	utils.c \
	utils.h \
	wireless-dialog.c \
	wireless-dialog.h \
	wireless-helper.h \
	$(NULL)

noinst_PROGRAMS = libnm-gtk-test

libnm_gtk_test_SOURCES = test.c
libnm_gtk_test_CFLAGS = $(LIBNM_GTK_CFLAGS)
libnm_gtk_test_LDADD = libnm-gtk.la $(LIBNM_GTK_LIBS)

uidir = $(datadir)/network-manager-netbook
ui_DATA = wireless-security.ui

EXTRA_DIST = \
	$(ui_DATA) \
	$(NULL)
