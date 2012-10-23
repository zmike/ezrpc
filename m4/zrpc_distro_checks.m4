dnl Determine the distro that is building the source,
dnl then set up config file install based on it

AC_DEFUN([ZRPC_CHECK_DISTRO],
[

AC_CHECKING([distro type])
AM_CONDITIONAL([UBUNTU], [false])
AM_CONDITIONAL([DEBIAN], [false])
AM_CONDITIONAL([SUSE], [false])
AM_CONDITIONAL([CENTOS], [false])
AM_CONDITIONAL([GENTOO], [false])
if (grep -q Ubuntu /etc/lsb-release &> /dev/null);then
	AM_CONDITIONAL([UBUNTU], [true])
	DISTRO=ubuntu
elif test -f /etc/debian_version ;then
	AM_CONDITIONAL([DEBIAN], [true])
	DISTRO=debian
elif test -f /etc/SuSE-release ;then
	AM_CONDITIONAL([SUSE], [true])
	DISTRO=suse
elif (grep -qi centos /etc/redhat-release &> /dev/null);then
	AM_CONDITIONAL([CENTOS], [true])
	DISTRO=centos
elif test -f /etc/gentoo-release ;then
	AM_CONDITIONAL([GENTOO], [true])
	DISTRO=gentoo
else
	DISTRO=unknown
fi

AC_MSG_RESULT([$DISTRO])
AC_ARG_ENABLE([confs-install], AS_HELP_STRING([--enable-confs-install],[force installation of config files @<:@default=detect@:>@]))
if test "x$enable_confs-install" != "xyes";then
	AC_MSG_NOTICE([checking whether to install zentific config files in ${prefix}])
	AM_CONDITIONAL([ZCONF], [test ! -f ${prefix}/zrpc.conf])
	AM_CONDITIONAL([DBCONF], [test ! -f ${prefix}/db/zrpcdb.conf])
	if test "x$DISTRO" != "xunknown" ;then
		_conf=$(grep -h defaults_DATA init/$DISTRO/Makefile.am|cut -d/ -f2-)
		AC_MSG_NOTICE([checking whether to install ${sysconfdir}/${_conf}])
		AM_CONDITIONAL([ETCCONF], [test ! -f ${sysconfdir}/${_conf}])
	else
		AM_CONDITIONAL([ETCCONF], [false])
	fi
else
	AM_CONDITIONAL([ZCONF], [true])
	AM_CONDITIONAL([DBCONF], [true])
	AM_CONDITIONAL([ETCCONF], [true])
fi

])
