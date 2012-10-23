AM_CFLAGS = \
$(EFL_CFLAGS) \
$(ESQL_CFLAGS) \
$(DEBUGCFLAGS) \
-I$(top_srcdir)/include  \
-I$(top_builddir) \
-DPREFIX=\"$(prefix)/\" \
-DLIBDIR=\"$(libdir)/\"
AM_LDFLAGS = -no-undefined --no-allow-shlib-undefined -export-dynamic -module -version-info $(ZRPC_VERSION) $(DEBUGLDFLAGS)

zrpcdb_la_LTLIBRARIES = zrpcdb.la

if DBCONF
cfgdir = $(prefix)/db
cfg_DATA = zrpcdb.conf
endif

zrpcdb_la_SOURCES = \
$(AZY_DIR)/Common_Esskyuehl.c \
zrpcdb.c \
zrpcdb_auth.c \
zrpcdb_common.c \
zrpcdb_console.c \
zrpcdb_disk.c \
zrpcdb_group.c \
zrpcdb_node.c \
zrpcdb_platform.c \
zrpcdb_network.c \
zrpcdb_session.c \
zrpcdb_storage.c \
zrpcdb_scheduler.c \
zrpcdb_user.c \
zrpcdb_vif.c \
zrpcdb_vm.c \
zrpcdb_zentific.c

zrpcdb_la_LIBADD = $(EFL_LIBS) $(ESQL_LIBS) $(AZY_LIBS) $(top_builddir)/src/servlets/libztype.la
zrpcdb_ladir = $(prefix)/db

MAINTAINERCLEANFILES = Makefile.in

sources:
	azy_parser $(AZY_DIR)/zentific-server.azy -Es -o $(AZY_DIR) -m common-impl
headers:
	azy_parser $(AZY_DIR)/zentific-server.azy -Es -o $(top_srcdir)/include -m common-headers
