AM_CFLAGS = \
$(ZSHARE_CFLAGS) \
$(AZY_CFLAGS) \
$(EFL_CFLAGS) \
$(OPENSSL_CFLAGS) \
$(DEBUGCFLAGS) \
-I$(top_srcdir)/include \
-DPREFIX=\"$(prefix)/\" \
-DLIBDIR=\"$(libdir)/\"
AM_LDFLAGS = -no-undefined -export-dynamic $(DEBUGLDFLAGS)

BUILT_SOURCES = sources headers

bin_PROGRAMS = zrpc

zrpc_SOURCES = \
$(AZY_DIR)/Common.c \
$(AZY_DIR)/Common_Azy.c \
$(top_srcdir)/include/Common.h \
zconf.c \
db_sources.c \
loop.c \
server.c \
sha1.c \
sha1wrap.c \
sigsegv.c \
utility.c \
xen-detect.c

zrpc_LDADD = @ZSHARE_LIBS@ $(AZY_LIBS) $(EFL_LIBS) $(OPENSSL_LIBS) $(PAM_LIBS)

MAINTAINERCLEANFILES = Makefile.in

sources:
	azy_parser -Es $(AZY_DIR)/zentific-server.azy -o $(AZY_DIR) -m common-impl,server-impl
headers:
	azy_parser -Es $(AZY_DIR)/zentific-server.azy -o $(top_srcdir)/include -m common-headers,server-headers
