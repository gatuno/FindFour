# Automake file for FindFour

gamedatadir = $(pkgdatadir)/data

bin_PROGRAMS = findfour
findfour_SOURCES = findfour.c findfour.h netplay.c netplay.h cp-button.c cp-button.h
#EXTRA_DIST =

findfour_CPPFLAGS = -DGAMEDATA_DIR=\"$(gamedatadir)/\" -DLOCALEDIR=\"$(localedir)\" $(AM_CPPFLAGS)
findfour_CFLAGS = $(SDL_CFLAGS) $(SDL_image_CFLAGS) $(SDL_mixer_CFLAGS) $(SDL_ttf_LIBS) $(AM_CFLAGS)
findfour_LDADD = $(SDL_LIBS) $(SDL_image_LIBS) $(SDL_mixer_LIBS) $(SDL_ttf_LIBS)

