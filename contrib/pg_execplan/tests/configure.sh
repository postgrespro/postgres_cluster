export CFLAGS="-O0"
INSTPATH=`pwd`/tmp_install

./configure --prefix=$INSTPATH --enable-debug --enable-cassert --enable-tap-tests --enable-depend && make clean && make -j4

#./configure --prefix=`pwd`/tmp_install --enable-tap-tests --enable-debug --enable-cassert --enable-nls --with-openssl --with-perl --with-tcl --with-python --with-tclconfig=/usr/lib/x86_64-linux-gnu/tcl8.6 --with-gssapi --with-libxml --with-libxslt --with-ldap --with-icu CFLAGS="-ggdb -Og -fno-stack-protector -DUSE_VALGRIND"

