# Makefile for OpenChangeSim
# Written by Julien Kerihuel <j.kerihuel@openchange.org>, 2010.

default: all

config.mk: config.status config.mk.in
	./config.status

config.status: configure
	./configure

configure: configure.ac
	./autogen.sh

include config.mk

all:		openchangesim

install:	all			\
		openchangesim-install

uninstall::	openchangesim-uninstall

dist:: 		distclean
	./autogen.sh

distclean:: clean
	rm -rf autom4te.cache
	rm -f config.status config.log 
	rm -f config.h config.h.in
	rm -f stamp-h1
	rm -f intltool-extract intltool-merge intltool-update

clean::
	rm -f *~
	rm -f */*~
	rm -f */*/*~
	rm -f openchangesim

re:: clean install

#################################################################
# Suffixes compilation rules
#################################################################

.SUFFIXES: .c .o .h .po

.c.o:
	@echo "Compiling $<"
	$(CC) $(CFLAGS) -c $< -o $@ $(LIBS)

.c.po:
	@echo "Compiling $< with -fPIC"
	@$(CC) $(CFLAGS) -fPIC -c $< -o $@

#################################################################
# OpenChangeSim compilation rules
#################################################################

src/version.h: VERSION
	@./script/mkversion.sh VERSION src/version.h $(PACKAGE_VERSION) $(top_builddir)/

openchangesim:	bin/openchangesim

openchangesim-install:
	@echo "[*] install: openchangesim"
	$(INSTALL) -d $(DESTDIR)$(bindir)
	$(INSTALL) -m 0755 bin/openchangesim $(DESTDIR)$(bindir)

openchangesim-uninstall:
	rm -f $(DESTDIR)$(bindir)/openchangesim

openchangesim-clean::
	rm -f bin/openchangesim
	rm -f src/*.o src/*.po
	rm -f src/version.h
	rm -f src/configuration.yy.c
	rm -f src/configuration.tab.c src/configuration.tab.h

clean:: openchangesim-clean

bin/openchangesim:	src/version.h					\
			src/configuration.tab.po			\
			src/configuration.yy.po				\
			src/configuration_api.po			\
			src/configuration_dump.po			\
			src/openchangesim_public.po			\
			src/openchangesim.o
	@echo "Linking $@"
	@$(CC) $(CFLAGS) -o $@ $^ $(LIBS) $(LDFLAGS) -lpopt

src/configuration.yy.c:	src/configuration.l
	@echo "Generating $@"
	@$(FLEX) -t $< > $@

src/configuration.tab.c: src/configuration.y
	@echo "Generating $@"
	@$(BISON) -d $< -o $@

# Avoid warnings
src/configuration.yy.o: CFLAGS=
src/configuration.tab.o: CFLAGS=

# This should be the last line in the makefile since other distclean rules may 
# need config.mk
distclean::
	rm -f config.mk