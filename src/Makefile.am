# Automake file for FindFour

gamedatadir = $(pkgdatadir)/data

bin_PROGRAMS = findfour
findfour_SOURCES = findfour.c findfour.h \
	netplay.c netplay.h \
	stun.c stun.h \
	juego.c juego.h \
	chat.c chat.h \
	inputbox.c inputbox.h \
	utf8.c utf8.h \
	draw-text.c draw-text.h \
	message.c message.h \
	path.c path.h \
	background.c background.h \
	resolv.c resolv.h \
	ventana.c ventana.h \
	sdl2_rect.c sdl2_rect.h \
	gettext.h

if MACOSX
findfour_SOURCES += SDLMain.m SDLMain.h
endif

EXTRA_DIST = findfour.rc SDLMain.m SDLMain.h

if MINGW32
findfour_ico.o: findfour.rc ../data/findfour.ico
	$(WINDRES) $(srcdir)/findfour.rc -O coff -o findfour_ico.o

mingw_ldadd = findfour_ico.o -lws2_32 -lwsock32
else # NOT MINGW
mingw_ldadd =
endif

findfour_CPPFLAGS = -DGAMEDATA_DIR=\"$(gamedatadir)/\" -DLOCALEDIR=\"$(localedir)\" $(AM_CPPFLAGS)
findfour_CFLAGS = $(SDL_CFLAGS) $(SDL_image_CFLAGS) $(SDL_mixer_CFLAGS) $(SDL_ttf_CFLAGS) $(AM_CFLAGS)
if MACOSX
# En MAC OS X, hay que ligar/compilar contra los frameworks
findfour_LDFLAGS = -Wl,-rpath,@loader_path/../Frameworks  $(AM_LDFLAGS)
else
findfour_LDADD = $(SDL_LIBS) $(SDL_image_LIBS) $(SDL_mixer_LIBS) $(SDL_ttf_LIBS) $(mingw_ldadd)
endif
LDADD = $(LIBINTL)

#------------------ Packaging rules for Mac OSX ------------------------

bundle_root = $(top_builddir)/etc/macfiles
bundle_name = $(bundle_root)/FindFour.app
# Special target to copy all needed game files to FindFour.app/Contents/Resources.
# For Mac OS X bundle.
.PHONY: bundle
bundle:
	-rm -rf $(bundle_root)
	mkdir -p $(bundle_name)/Contents/MacOS
	mkdir -p $(bundle_name)/Contents/Resources/data/
	echo "APPL????" > $(bundle_name)/Contents/PkgInfo
	cp $(top_builddir)/etc/Info.plist $(bundle_name)/Contents/
	cp $(top_builddir)/etc/findfour.icns $(bundle_name)/Contents/Resources/
	mkdir -p $(bundle_name)/Contents/Resources/data/images
	cp -R $(top_builddir)/data/images/* $(bundle_name)/Contents/Resources/data/images
	mkdir -p $(bundle_name)/Contents/Resources/data/sounds
	cp $(top_builddir)/data/sounds/* $(bundle_name)/Contents/Resources/data/sounds
	cp $(top_builddir)/data/*.ttf $(bundle_name)/Contents/Resources/data
	for cat in $(ALL_LINGUAS); do \
	  dir=$(bundle_name)/Contents/Resources/locale/$$cat/LC_MESSAGES; \
	  mkdir -p $$dir; \
	  echo "installing $$cat as $$dir/findfour.mo"; \
	  cp $(top_builddir)/po/$$cat.gmo $$dir/findfour.mo; \
	done

# Special target to copy the binary to FindFour.app/Contents/MacOS.
.PHONY: bundle-fw
bundle-fw: bundle
	cp findfour $(bundle_name)/Contents/MacOS/findfour
	mkdir  $(bundle_name)/Contents/Frameworks
	for i in SDL SDL_mixer SDL_image SDL_ttf; do \
	  cp -a /Library/Frameworks/$${i}.framework $(bundle_name)/Contents/Frameworks/; \
	done
	strip $(bundle_name)/Contents/MacOS/findfour

# Special target to make a disk image of the bundled app
.PHONY: bundle-dmg
bundle-dmg: bundle-fw
	ln -s /Applications $(bundle_root)/Applications
	hdiutil create  -ov -srcfolder $(bundle_root) -volname FindFour -imagekey zlib-level=6 $(top_builddir)/etc/findfour.dmg

