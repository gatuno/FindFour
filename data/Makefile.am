gamedatadir = $(pkgdatadir)/data

nobase_dist_gamedata_DATA = images/lodge.png \
	images/lodge_fire.png \
	images/lodge_candle.png \
	images/ventana.png \
	images/tablero.png \
	images/coinblue.png \
	images/coinred.png \
	images/bigcoinred.png \
	images/bigcoinblue.png \
	images/button-up.png \
	images/button-over.png \
	images/button-down.png \
	images/button-arrow-1-down.png \
	images/button-arrow-1-over.png \
	images/button-arrow-1-up.png \
	images/button-arrow-2-down.png \
	images/button-arrow-2-over.png \
	images/button-arrow-2-up.png \
	images/button-list-down.png \
	images/button-list-over.png \
	images/button-list-up.png \
	images/shadow-down.png \
	images/shadow-over.png \
	images/shadow-up.png \
	images/ventana-chat.png \
	images/loading.png \
	images/list-mini.png \
	images/list-big.png \
	images/win-1.png \
	images/win-2.png \
	images/win-3.png \
	images/win-4.png \
	images/window.png \
	images/tab.png \
	images/inputbox.png \
	images/chat.png \
	images/boton-error-down.png \
	images/boton-error-over.png \
	images/boton-error-up.png \
	images/boton-normal-down.png \
	images/boton-normal-over.png \
	images/boton-normal-up.png \
	images/error.png \
	images/fuzz.png \
	images/recent-mini.png \
	images/window_shadow.png \
	images/error_shadow.png \
	images/icon.png \
	sounds/drop.wav \
	sounds/win.wav \
	ccfacefront.ttf \
	burbankbm.ttf \
	comicrazy.ttf

# Instalar los archivos .desktop e iconos
applicationsdir = $(datadir)/applications
applications_DATA = desktop/findfour.desktop

icons16dir = $(datadir)/icons/hicolor/16x16/apps
icons16_DATA = desktop/16x16/findfour.png

icons32dir = $(datadir)/icons/hicolor/32x32/apps
icons32_DATA = desktop/32x32/findfour.png

icons48dir = $(datadir)/icons/hicolor/48x48/apps
icons48_DATA = desktop/48x48/findfour.png

icons64dir = $(datadir)/icons/hicolor/64x64/apps
icons64_DATA = desktop/64x64/findfour.png

icons128dir = $(datadir)/icons/hicolor/128x128/apps
icons128_DATA = desktop/128x128/findfour.png

icons256dir = $(datadir)/icons/hicolor/256x256/apps
icons256_DATA = desktop/256x256/findfour.png

# touch the toplevel icon directory as required by the icon theme spec
install-data-hook:
	touch $(DESTDIR)$(datadir)/icons/hicolor

EXTRA_DIST = \
	desktop/findfour.desktop	\
	desktop/16x16/findfour.png		\
	desktop/32x32/findfour.png		\
	desktop/48x48/findfour.png		\
	desktop/64x64/findfour.png		\
	desktop/128x128/findfour.png		\
	desktop/256x256/findfour.png		\
	findfour.ico

