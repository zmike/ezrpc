Name: zrpc
Summary: Zentific XMLRPC server
Version: 0.9.5
Release: 0
License: LGPL
Group: Development
Source: %{name}-%{version}.tar.gz
URL: http://www.zentific.com/
BuildRoot: %{_tmppath}/%{name}-%{version}-root
BuildRequires: libzxr mysql-devel pam-devel libzshare
Requires: libzxr libmysqlclient15 pam libzshare

%description
Zentific XMLRPC server

%prep
#echo %_target
#echo %_target_alias
#echo %_target_cpu
#echo %_target_os
#echo %_target_vendor
echo Building %{name}-%{version}-%{release}

%setup

%build
./bootstrap.sh

%install
make DESTDIR=%buildroot install

%post
chkconfig -a %{name}

%clean
rm -rf $RPM_BUILD_ROOT

%postun
/sbin/ldconfig


%files
%defattr(-,root,root)
/opt/zentific/bin/zrpc
/etc/init.d/zrpc

/opt/zentific/db/zrpcdb*
/opt/zentific/zrpc/*
/opt/zentific/zrpc.conf
/etc/pam.d/zrpc
/etc/sysconfig/SuSEfirewall2.d/services/zrpc
/etc/sysconfig/zrpc
/etc/init.d/zrpc
/opt/zentific/utils/zrpcdb_init
/opt/zentific/utils/zentific.sql
