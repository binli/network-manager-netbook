plugindir = $(libdir)/gnome-bluetooth/plugins

if HAVE_GBT
plugin_LTLIBRARIES = libnma.la

libnma_la_CPPFLAGS = \
	-I${top_srcdir}/libnm-gtk \
	$(GNOME_BLUETOOTH_CFLAGS)

libnma_la_SOURCES = network-manager-applet.c
libnma_la_LDFLAGS = -module -avoid-version
libnma_la_LIBADD = $(top_builddir)/libnm-gtk/libnm-gtk.la $(GNOME_BLUETOOTH_LIBS)
endif

EXTRA_DIST = network-manager-applet.c
