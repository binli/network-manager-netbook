noinst_LTLIBRARIES = libwireless-security.la

libwireless_security_la_SOURCES = \
	wireless-security.h \
	wireless-security.c \
	ws-wep-key.h \
	ws-wep-key.c \
	ws-wpa-psk.h \
	ws-wpa-psk.c \
	ws-leap.h \
	ws-leap.c \
	ws-wpa-eap.h \
	ws-wpa-eap.c \
	ws-dynamic-wep.h \
	ws-dynamic-wep.c \
	eap-method.h \
	eap-method.c \
	eap-method-tls.h \
	eap-method-tls.c \
	eap-method-leap.h \
	eap-method-leap.c \
	eap-method-ttls.h \
	eap-method-ttls.c \
	eap-method-peap.h \
	eap-method-peap.c \
	eap-method-simple.h \
	eap-method-simple.c \
	helpers.h \
	helpers.c

libwireless_security_la_CPPFLAGS = \
	$(NMN_CFLAGS) \
	-I${top_srcdir}/libnm-gtk \
	-I${top_srcdir}/src/ \
	-DUIDIR=\""$(uidir)"\"

libwireless_security_la_LIBADD = \
	$(NMN_LIBS)

uidir = $(datadir)/network-manager-netbook
ui_DATA = \
	ca-nag-dialog.ui \
	dynamic-wep.ui \
	eap-leap.ui \
	eap-peap.ui \
	eap-simple.ui \
	eap-tls.ui \
	eap-ttls.ui \
	leap.ui \
	wep-key.ui \
	wpa-eap.ui \
	wpa-psk.ui

EXTRA_DIST = $(ui_DATA)
