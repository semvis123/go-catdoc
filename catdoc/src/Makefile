# Your C compilier and flags
SHELL = /bin/sh

srcdir = .
top_srcdir = ..

prefix = /usr/local
exec_prefix = ${prefix}
installroot = 

bindir = ${exec_prefix}/bin
libdir = ${exec_prefix}/lib
confdir = ${prefix}/etc
datadir = ${datarootdir}
datarootdir = ${prefix}/share
mandir = ${datarootdir}/man
INSTALL = /usr/local/bin/ginstall -c
WISH = /usr/local/bin/wish
CC = gcc
COMPAT_OBJ=
build_targets = catdoc xls2csv catppt
install_targets = install-catdoc
CHARSETPATH=${datadir}/catdoc
# Flags to pass to your C compilier
# remove -O2 on HP/UX 9. It is known to cause problems
FLAGS=-I. -g -O3 -DHAVE_CONFIG_H -DCATDOC_VERSION=\"0.95\" -DCHARSETPATH=\"${CHARSETPATH}\"

#Compile-time configuration of catdoc itself. Edit according to taste

#Directory, where all system-wide stuff resides
DATA_DIR=$(datadir)/catdoc
#directory, where catdoc binary would be placed
#directory where to put man page

#System-wide configuration file
SYSTEMRC=$(confdir)/catdocrc

#Per-user configration file (in user's home directory
USERRC=.catdocrc

# path where to search for charsets. may be colon-separated list of dirs

# Extension for map of special character
SPEC_EXT=.specchars

# Extension for map of substitutes (chars which are missing from target
# charset)
REPL_EXT=.replchars

# Target charset - one used on your system console or xterm
TARGET_CHARSET=koi8-r

# Source charset - one which your Word files are usially in unless they
# are UNICODE
SOURCE_CHARSET=cp1251

# Character which would be printed instead of ones, not found in charset
# or replacement map
UNKNOWN_CHAR=?

# END OF COMPILE-TIME CONFIGURATION

DEFINES=-DSYSTEMRC=\"$(SYSTEMRC)\" -DUSERRC=\"$(USERRC)\"

CFLAGS=$(FLAGS) $(DEFINES) -DHAVE_CONFIG_H

COMMONOBJ=charsets.o substmap.o fileutil.o confutil.o numutils.o ole.o 

OBJ=catdoc.o reader.o writer.o analyze.o rtfread.o $(COMMONOBJ)

OBJXLS=xls2csv.o sheet.o xlsparse.o $(COMMONOBJ)

OBJPPT=catppt.o pptparse.o $(COMMONOBJ)
#.c.o:
#	$(CC) -c $(CFLAGS) $*
all: catdoc xls2csv catppt
catdoc: $(OBJ)
	$(CC) -o catdoc  $(OBJ) $(LDFLAGS)

third_party/emsdk:
	mkdir -p third_party/emsdk
	test -d $@/.git || git clone --depth 1 https://github.com/emscripten-core/emsdk.git $@
	cd $@ && git fetch origin $(EMSDK_COMMIT) && git checkout $(EMSDK_COMMIT)
	touch $@

emsdk.uptodate: third_party/emsdk
	third_party/emsdk/emsdk install 3.1.37
	third_party/emsdk/emsdk activate 3.1.37
	rm -Rf third_party/emsdk/upstream/emscripten
	git clone $(EMSCRIPTEN_FORK) third_party/emsdk/upstream/emscripten
	cd third_party/emsdk/upstream/emscripten/ && git checkout $(EMSCRIPTEN_BRANCH)
	touch emsdk.uptodate

EMSDK_DIR=$(PWD)/third_party/emsdk/upstream/emscripten
EMSDK_COMMIT=ef2a8e929d5337755e9b1d1e1d4ad859dc694eee
EMSCRIPTEN_BRANCH=implement-more-syscalls-for-standalone
EMSCRIPTEN_FORK=https://github.com/jerbob92/emscripten

catdoc-wasm: emsdk.uptodate
	$(EMSDK_DIR)/emcc -o ../../catdoc.wasm \
		catdoc.c reader.c writer.c \
		analyze.c rtfread.c charsets.c \
		substmap.c fileutil.c confutil.c \
		numutils.c ole.c \
		-I. -O3 -DHAVE_CONFIG_H \
		-DCATDOC_VERSION=\"0.95\" \
		-DCHARSETPATH=\"charsets\" \
		-sEXPORTED_FUNCTIONS=_get_text,_get_author,_get_last_author,_get_version,_get_title,_get_subject,_get_keywords,_get_comments\
		-sSTANDALONE_WASM -sWARN_ON_UNDEFINED_SYMBOLS=0 \
		--no-entry -sFILESYSTEM=1 -sALLOW_MEMORY_GROWTH -sMAXIMUM_MEMORY=1GB

xls2csv: $(OBJXLS)
	$(CC) -o xls2csv $(OBJXLS) -lm $(LDFLAGS)

catppt: $(OBJPPT)
	$(CC) -o catppt $(OBJPPT) -lm $(LDFLAGS)

install: install-catdoc
install-catdoc:catdoc xls2csv catppt 
	../mkinstalldirs $(installroot)$(bindir)
	$(INSTALL) -m 755 catdoc $(installroot)$(bindir)/catdoc
	$(INSTALL) -m 755 xls2csv $(installroot)$(bindir)/xls2csv
	$(INSTALL) -m 755 catppt $(installroot)$(bindir)/catppt
install-wordview: wordview 
	../mkinstalldirs $(installroot)$(bindir)
	$(INSTALL) -m 755 wordview $(installroot)$(bindir)/wordview
wordview: wordview.tcl
	echo "#! $(WISH)" >wordview
	echo set charset_lib "\"$(CHARSETPATH)\"">>wordview
	cat wordview.tcl >>wordview
	chmod 0755 wordview
strftime.o: ../compat/strftime.c
	$(CC) -c $(FLAGS) -o $@ $*
clean:
	rm -f *.o catdoc wordview xls2csv catppt
distclean: clean
	rm Makefile
