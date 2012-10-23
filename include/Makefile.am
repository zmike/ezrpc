ACLOCAL_AMFLAGS = -I m4
INCLUDES = $(INCLTDL)

includedir = $(prefix)/include
include_HEADERS = Common.h

BUILT_SOURCES = headers

CLEANFILES = $(h)
MAINTAINERCLEANFILES = Makefile.in

headers:
	azy_parser -E $(AZY_DIR)/zentific-server.azy -o $(top_srcdir)/include -m common-headers,server-headers

h = \
DEBUG.azy_server.h \
DEBUG.azy_server_stubs.h \
Network.azy_server.h \
Network.azy_server_stubs.h \
Node.azy_server.h \
Node.azy_server_stubs.h \
Platform.azy_server.h \
Platform.azy_server_stubs.h \
Scheduler.azy_server.h \
Scheduler.azy_server_stubs.h \
Session.azy_server.h \
Session.azy_server_stubs.h \
Storage.azy_server.h \
Storage.azy_server_stubs.h \
system.azy_server.h \
system.azy_server_stubs.h \
User.azy_server.h \
User.azy_server_stubs.h \
VM.azy_server.h \
VM.azy_server_stubs.h \
Zentific.azy_server.h \
Zentific.azy_server_stubs.h \
Common.h \
Common_Azy.h \
Common_Esskyuehl.h
