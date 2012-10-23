#!/bin/bash
bs_dir="$(dirname $(readlink -f $0))"
rm -rf "${bs_dir}"/autom4te.cache
rm -f "${bs_dir}"/aclocal.m4 "${bs_dir}"/ltmain.sh

echo "Running autoreconf..." ; cd "${bs_dir}" && autoreconf -fi && cd - || exit 1

echo 'Configuring...'
#cd "${bs_dir}" && CFLAGS="-Os -pipe -Wall -march=native" LDFLAGS="-Wl,--sort-common -Wl,-O1" ./configure --prefix=/opt/zentific --libdir=/opt/zentific/zrpc --datadir=/etc/pam.d --sysconfdir=/etc --disable-debug $@ && cd - &> /dev/null || exit 1
cd "${bs_dir}" && CFLAGS="-Wall -Wextra -O0 -pipe -g -DZDEV_DEBUG -DSHUTUP_DB" ./configure --prefix=/opt/zentific --libdir=/opt/zentific/zrpc --datadir=/etc/pam.d --sysconfdir=/etc --disable-debug $@ && cd - &> /dev/null || exit 1
echo -e "\nRunning make..."
make -C "${bs_dir}" -j2 --no-print-directory V=0 || exit 1
echo -e "\n\nDone compiling!\n"
echo -e "\nYou must now run 'make install' as a privileged user."

echo -e "\nFinal steps required before running zrpc for the first time:\n"

[[ ! -d /var/run/zentific ]] && echo -e "\nRequired directory missing! Execute the following command or zrpc will not run!\n\tmkdir /var/run/zentific"
[[ ! -d /opt/zentific/logs ]] && mkdir -p /opt/zentific/logs

echo -e "Be sure to run update-rc.d (debian et al), chkconfig (Red Hat, etc),
or equivalent to enable the init script, which is /etc/init.d/zrpc by default.

Finally, make a zentific database (plus user/password) with the schema provided,
then configure /opt/zentific/db/zrpc.conf so zrpc knows how to talk to your
database, and run zrpcdb_init to set up an initial admin user.
For more information, check out the DATABASE_README file.
"

