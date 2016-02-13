ACLOCAL_AMFLAGS = -I m4 --install
dist_doc_DATA = README

bin_PROGRAMS = patts-upg

patts_upg_SOURCES = patts-upg.c
patts_upg_CFLAGS = -Wall -Wextra -Wunreachable-code -ftrapv -std=c11
patts_upg_LDADD = $(patts_LIBS)

AM_CFLAGS = $(DEPS_CFLAGS)
AM_LIBS = $(DEPS_LIBS)
