INSTDIR=`pwd`/tmp_install
GCPSCRIPTS=`pwd`/contrib/pg_exchange/gcp/

export LD_LIBRARY_PATH=$INSTDIR/lib:$LD_LIBRARY_PATH
export PATH=$INSTDIR/bin:$PATH
export PATH=$GCPSCRIPTS:$PATH

