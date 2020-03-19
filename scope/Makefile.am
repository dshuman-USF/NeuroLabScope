
bin_PROGRAMS = scope edt2ps

scope_SOURCES = AddTree.c FilterTree.c WritePop.c blockList.c		\
blockList.h dispUnit64.c error.h filesup.h filesup64.c ltoa.c		\
options.c std.c std.h FilterTree.h options.h WritePop.h AddTree.h	\
defines.h dispUnit64.h color_table.c color_table.h fpu_check.h		\
gammafit_search.h gen_control.h unpin.h fpu_check.c gammafit_search.c	\
gen_control.c unpin.c bandpass.c bandpass.h

edt2ps_SOURCES = edt2ps.c util.c util.h

#AM_CFLAGS = -mgas -g
AM_CFLAGS = -Wall --std=c99

LDADD = $(X_LIBS) -lXm -lXmu -lXt $(X_PRE_LIBS) -lX11	\
$(X_EXTRA_LIBS) $(LIBOBJS) -lJudy -lgsl -lgslcblas

AM_CPPFLAGS = $(X_CFLAGS)

scopefiles: $(scope_SOURCES) Makefile.am
	ls $(scope_SOURCES) > scopefiles

EXTRA_DIST = getline.h bandpass.pdf scopescreen.pdf debian

dist_doc_DATA = scope.pdf ChangeLog README COPYING LICENSE

info_TEXINFOS = scope.texi

checkin_files = $(scope_SOURCES) $(edt2ps_SOURCES) $(dist_DOC_DATA) $(EXTRA_DIST) $(info_TEXINFOS) Makefile.am configure.ac ChangeLog

deb:
	@echo 'Making debian packages'
	make distdir &&\
	cd $(distdir) &&\
	dpkg-buildpackage -Zgzip -uc -us && \
	cd .. && \
	rm -rf $(distdir)

