SUBDIRS = . test

AM_YFLAGS = -d -t
AM_LFLAGS = -i
AM_CPPFLAGS = -DIJVM_DATADIR="\"$(datadir)\"" \
	-DCOMPILE_HOST="\"$(shell hostname)\"" \
	-DCOMPILE_DATE="\"$(shell date '+%a %b %e %Y')\""

ijvm-lex.o : ijvm-parse.h
mic1-lex.o : mic1-parse.h

bin_PROGRAMS   = ijvm-asm ijvm mic1-asm mic1

DISTCLEANFILES = ijvm-lex.c ijvm-parse.c ijvm-parse.h \
	mic1-lex.c mic1-parse.c mic1-parse.h

CLEANFILES = mini-ijvm.tar.gz

ijvm_asm_SOURCES = ijvm-asm.c ijvm-asm.h ijvm-cons.c \
	ijvm-parse.y ijvm-parse.h ijvm-lex.l ijvm-emit.c \
	ijvm-spec.c ijvm-spec.h ijvm-util.c ijvm-util.h types.h

ijvm_SOURCES  = ijvm.c ijvm-util.c ijvm-util.h \
	ijvm-spec.c ijvm-spec.h types.h

mic1_asm_SOURCES = mic1-asm.c mic1-asm.h mic1-cons.c \
	mic1-parse.y mic1-parse.h mic1-lex.l \
	mic1-layout.c mic1-check.c \
	mic1-util.c mic1-util.h types.h

mic1_SOURCES = mic1.c mic1-util.c mic1-util.h ijvm-spec.c ijvm-spec.h \
	ijvm-util.c ijvm-util.h types.h

data_DATA = ijvm.spec

EXTRA_DIST = $(data_DATA) Makefile.mini.in

mini_ijvm = ijvm.spec ijvm.c ijvm-util.c ijvm-util.h \
	ijvm-spec.c ijvm-spec.h types.h

mini-ijvm.tar.gz : $(mini_ijvm) Makefile.mini.in
	-rm -rf mini-ijvm
	mkdir mini-ijvm
	cp $(addprefix $(srcdir)/,$(mini_ijvm)) mini-ijvm
	sed -e 's:\@datadir\@:$(datadir):g' \
	  -e 's:\@VERSION\@:$(VERSION):g' \
	  $(srcdir)/Makefile.mini.in > mini-ijvm/Makefile
	tar cfz $@ mini-ijvm
	-rm -rf mini-ijvm

test : test-ijvm-asm

test-ijvm-asm:
	(for f in test/*.j; do ./ijvm-asm $$f; done) > test/output 2>&1
	diff test/output test/ijvm-asm.output >/dev/null

daimi-install:
	./daimi-install.sh $(VERSION)
