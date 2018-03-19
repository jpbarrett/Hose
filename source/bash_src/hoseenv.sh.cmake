#!/bin/sh

if [ -n "$HOSE_SYS" ]
    then
        export OLD_HOSE_SYS=$HOSE_SYS
        OLD_PATH=$OLD_HOSE_SYS/bin:
        OLD_LDLIBPATH=$OLD_HOSE_SYS/lib:
        OLD_CMAKE_PREF=$OLD_HOSE_SYS:
fi

if [ -n "$HOSE_SYSPY" ]
    then
        export OLD_HOSE_SYSPY=$HOSE_SYSPY
fi

if [ -n "$HOSE_INSTALL" ]
    then
        export OLD_HOSE_INSTALL=$HOSE_INSTALL
        OLD_PATH=$OLD_HOSE_INSTALL/bin:
        OLD_LDLIBPATH=$OLD_HOSE_INSTALL/lib:
        OLD_CMAKE_PREF=$OLD_HOSE_INSTALL:
fi

export HOSE_INSTALL=@CMAKE_INSTALL_PREFIX@

if [ $# -eq 0 ]
  then
    HOSE_SYS=$HOSE_INSTALL
  else
    HOSE_SYS=`readlink -f $1`
fi

export HOSE_SYS

export PATH=$HOSE_INSTALL/bin:${PATH//${OLD_PATH}/}
export LD_LIBRARY_PATH=$HOSE_INSTALL/lib:${LD_LIBRARY_PATH//${OLD_LDLIBPATH}/}
export CMAKE_PREFIX_PATH=$HOSE_INSTALL:${CMAKE_PREFIX_PATH//${OLD_CMAKE_PREF}/}

echo -e "Hose install directory set to ${HOSE_INSTALL}"

return 0
