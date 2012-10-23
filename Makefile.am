ACLOCAL_AMFLAGS = -I m4
INCLUDES = $(INCLTDL)

SUBDIRS = include src db_sources

if ZRPC

SUBDIRS += init

cfgdir = $(sysconfdir)/pam.d
cfg_DATA = zentific-xmlrpc

if ZCONF
confdir = $(prefix)
conf_DATA = zrpc.conf
endif

endif

sqldir = $(prefix)/utils
sql_DATA = zentific.sql

MAINTAINERCLEANFILES = Makefile.in aclocal.m4 config.guess config.h* config.sub \
	configure depcomp install-sh ltmain.sh missing compile m4/l*

maintainer-clean-local: 
	rm -rf autom4te.cache
	rm -rf debian/zrpc
