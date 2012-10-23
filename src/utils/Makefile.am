AM_CFLAGS = \
$(ZSHARE_CFLAGS) \
$(AZY_CFLAGS) \
$(EFL_CFLAGS) \
$(OPENSSL_CLFAGS) \
$(DEBUGCFLAGS) \
-I$(top_srcdir)/include \
-I$(top_builddir) \
-DPREFIX=\"$(prefix)/\" \
-DLIBDIR=\"$(libdir)/\"

AM_LDFLAGS = -no-undefined -export-dynamic $(DEBUGLDFLAGS)

zrpcdb_init_PROGRAMS = zrpcdb_init
zrpcdb_init_SOURCES = \
dbinit.c \
readpassphrase.c \
$(ZRPC_DIR)/zconf.c \
$(ZRPC_DIR)/db_sources.c \
$(ZRPC_DIR)/sha1.c \
$(ZRPC_DIR)/sha1wrap.c \
$(ZRPC_DIR)/utility.c \
$(AZY_DIR)/Common_Azy.c \
$(top_srcdir)/src/servlets/functions/functions-auth.c \
$(top_srcdir)/src/servlets/functions/functions_cb.c \
$(top_srcdir)/src/servlets/functions/functions-session.c \
$(top_srcdir)/src/servlets/functions/functions-user.c \
$(top_srcdir)/src/servlets/functions/functions-zentific.c

zrpcdb_initdir = $(prefix)/utils
zrpcdb_init_LDADD = @EFL_LIBS@ @AZY_LIBS@ @OPENSSL_LIBS@ @ZSHARE_LIBS@ @top_builddir@/src/servlets/libztype.la

#zproxy_PROGRAMS = zproxy
#zproxy_SOURCES = zproxy.c
#zproxydir = $(prefix)/utils
#zproxy_LDADD = $(LIBZXR_LIBS) @ZSHARE_LIBS@

MAINTAINERCLEANFILES = Makefile.in
