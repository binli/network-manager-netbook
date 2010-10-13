NULL=

libexec_PROGRAMS = network-manager-netbook

network_manager_netbook_CPPFLAGS = \
	$(NMN_CFLAGS) \
	-I${top_builddir}/marshallers \
	-I${top_srcdir}/libnm-gtk \
	-I${top_srcdir}/libnm-gtk/wireless-security \
	-DUIDIR=\""$(uidir)"\" \
	-DICON_PATH=\""$(pkgdatadir)/icons/"\" \
	-DNMNLOCALEDIR=\"$(datadir)/locale\" \
	-DTHEME_PATH=\""$(pkgdatadir)/theme/"\" \
	$(NULL)

network_manager_netbook_LDADD = \
	$(NMN_LIBS) \
	${top_builddir}/marshallers/libmarshallers.la \
	${top_builddir}/libnm-gtk/libnm-gtk.la \
	${top_builddir}/libnm-gtk/wireless-security/libwireless-security.la \
	$(NULL)

network_manager_netbook_SOURCES = \
	gtkinfobar.c \
	gtkinfobar.h \
	nmn-applet.c \
	nmn-applet.h \
	nmn-connection-details.c \
	nmn-connection-details.h \
	nmn-gsm-pin-request-renderer.c \
	nmn-gsm-pin-request-renderer.h \
	nmn-item-renderer.c \
	nmn-item-renderer.h \
	nmn-list.c \
	nmn-list.h \
	nmn-model.c \
	nmn-model.h \
	nmn-network-renderer.c \
	nmn-network-renderer.h \
	nmn-new-connection.c \
	nmn-new-connection.h \
	nmn-panel-client.c \
	nmn-panel-client.h \
	main.c \
	$(NULL)
