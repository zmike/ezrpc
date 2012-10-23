AM_CFLAGS = \
$(AZY_CFLAGS) \
$(EFL_CFLAGS) \
$(OPENSSL_CLFAGS) \
$(DEBUGCFLAGS) \
-I$(top_srcdir)/include \
-I$(top_builddir) \
-DPREFIX=\"$(prefix)/\" \
-DLIBDIR=\"$(libdir)/\"

AM_LDFLAGS = -export-dynamic -version-info $(ZRPC_VERSION) $(DEBUGLDFLAGS)

SUBDIRS = azy

BUILT_SOURCES = sources headers

zrpc_files = \
$(ZRPC_DIR)/sha1wrap.c \
$(ZRPC_DIR)/sha1.c \
$(ZRPC_DIR)/utility.c

lib_LTLIBRARIES = libztype.la

libztype_la_SOURCES = azy/Common.c
libztype_la_CFLAGS = $(EFL_CFLAGS) -I$(top_srcdir)/include -I$(top_builddir)
libztype_la_LIBADD = $(EFL_LIBS)

if ZRPC
lib_LTLIBRARIES += zrpc_base.la

if ZDEV_ENABLE_DEVEL_SERVLET_DO_NOT_BUILD_FOR_PRODUCTION
DEBUG_sources = \
azy/DEBUG.azy_server.c \
functions/functions-debug.c
else
DEBUG_sources =
endif

Network_sources = \
azy/Network.azy_server.c \
functions/functions-network.c

Storage_sources = \
azy/Storage.azy_server.c \
functions/functions-storage.c

Node_sources = \
azy/Node.azy_server.c \
functions/functions-node.c

Scheduler_sources = \
azy/Scheduler.azy_server.c \
functions/functions-jobs.c

Session_sources = \
azy/Session.azy_server.c \
functions/functions-session.c

User_sources = \
azy/User.azy_server.c \
functions/functions-auth.c \
functions/functions-group.c \
functions/functions-user.c

VM_sources = \
azy/VM.azy_server.c \
functions/functions-vm.c \
functions/functions-vif.c \
functions/functions-console.c \
functions/functions-disk.c \
functions/functions-log.c

Zentific_sources = \
azy/Zentific.azy_server.c \
functions/functions-zentific.c

Platform_sources = \
azy/Platform.azy_server.c \
functions/functions-platform.c

#system_sources = \
#azy/system.azy_server.c

zrpc_base_la_LIBADD = $(AZY_LIBS) $(EFL_LIBS) $(OPENSSL_LIBS) libztype.la
zrpc_base_la_LDFLAGS = $(AM_LDFLAGS) -module
zrpc_base_la_SOURCES = \
init.c \
servlet-functions.c \
functions/functions_cb.c \
azy/Common_Azy.c \
$(zrpc_files) \
$(Platform_sources) \
$(Zentific_sources) \
$(VM_sources) \
$(User_sources) \
$(Storage_sources) \
$(Session_sources) \
$(Scheduler_sources) \
$(Node_sources) \
$(Network_sources) \
$(DEBUG_sources)
endif

MAINTAINERCLEANFILES = Makefile.in
CLEANFILES = azy/*.c

sources:
	azy_parser -Es $(AZY_DIR)/zentific-server.azy -o $(AZY_DIR) -m common-impl,server-impl

headers:
	azy_parser -Es $(AZY_DIR)/zentific-server.azy -o $(top_srcdir)/include -m common-headers,server-headers
