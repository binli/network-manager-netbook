SUBDIRS = icons marshallers libnm-gtk src gnome-bluetooth po

autostartdir = $(sysconfdir)/xdg/autostart
autostart_in_files = network-manager-netbook.desktop.in
autostart_DATA = network-manager-netbook.desktop

%.desktop: %.desktop.in
	$(AM_V_GEN) sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

servicedir = $(datadir)/dbus-1/services
service_in_files = com.meego.UX.Shell.Panels.network.service.in
service_DATA = com.meego.UX.Shell.Panels.network.service

%.service: %.service.in
	$(AM_V_GEN) sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

themedir = $(pkgdatadir)/theme
theme_in_files = network-manager-netbook.css.in
theme_DATA = network-manager-netbook.css

%.css: %.css.in
	$(AM_V_GEN) sed -e "s|\@pkgdatadir\@|$(pkgdatadir)|" $< > $@

EXTRA_DIST = \
	$(autostart_in_files) \
	$(service_in_files) \
	$(theme_in_files) \
	intltool-extract.in \
	intltool-merge.in \
	intltool-update.in

DISTCLEANFILES = \
	$(autostart_DATA) \
	$(service_DATA) \
	$(theme_DATA) \
	intltool-extract \
	intltool-merge \
	intltool-update
