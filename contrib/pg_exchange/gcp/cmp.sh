export CFLAGS="-O0" 

./configure --prefix=`pwd`/tmp_install --enable-tap-tests --enable-debug \
 --enable-cassert --enable-nls --with-openssl --with-perl --with-tcl --with-python \
 --with-tclconfig=/usr/lib/x86_64-linux-gnu/tcl8.6

# make

