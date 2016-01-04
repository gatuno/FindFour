# Automake file for FindFour

gamedatadir = $(pkgdatadir)/data

bin_PROGRAMS = findfour
findfour_SOURCES = findfour.c findfour.h netplay.c netplay.h cp-button.c cp-button.h stun.c stun.h juego.c juego.h chat.c chat.h inputbox.c inputbox.h utf8.c utf8.h draw-text.c draw-text.h message.c message.h

EXTRA_DIST = findfour.rc

if MINGW32
findfour_ico.o: findfour.rc ../data/findfour.ico
	$(WINDRES) $(srcdir)/findfour.rc -O coff -o findfour_ico.o

mingw_ldadd = findfour_ico.o
else # NOT MINGW
mingw_ldadd =
endif

findfour_CPPFLAGS = -DGAMEDATA_DIR=\"$(gamedatadir)/\" -DLOCALEDIR=\"$(localedir)\" $(AM_CPPFLAGS)
findfour_CFLAGS = $(SDL_CFLAGS) $(SDL_image_CFLAGS) $(SDL_mixer_CFLAGS) $(SDL_ttf_LIBS) $(AM_CFLAGS)
findfour_LDADD = $(SDL_LIBS) $(SDL_image_LIBS) $(SDL_mixer_LIBS) $(SDL_ttf_LIBS) $(mingw_ldadd)

if MINGW32
findfour_LDADD += -lws2_32 -lwsock32
endif
