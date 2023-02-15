#!/bin/bash
# This script is used to build IntelJ3 burner for Linux and Windows platform.

BASEDIR=$(dirname "$0")
echo "$BASEDIR"
cd $BASEDIR

for ARG in "$@"; do
	case "$ARG" in
		--bin=*)
			ARG_PATH=${ARG#*=}
			;;
	esac
done

export PATH=/home/users/sqa/mingw/bin:$PATH
#BUILD_DIR=`pwd`
#rm -rf IntelJ3
#git clone atclnx01:/home/project/git_repo/IntelJ3.git
#cd $BUILD_DIR/IntelJ3

# Remove all executable file
rm -f IntelJ3 IntelJ3.exe
rm -f platform.c spiflash-MXIC.c
rm -f Makefile_PARburn_linux Makefile_PARburn_win
rm -f Makefile_SPIburn_linux Makefile_SPIburn_win
rm -f build_PARburn.sh build_SPIburn.sh

# For Windows platform
rm -f *.o
rm -f ./telnet/*.o
if type i386-mingw32-gcc 2> /dev/null; then
    export PATH=/home/users/sqa/mingw/bin:$PATH
    make --makefile=Makefile_win clean
    make --makefile=Makefile_win
    i386-mingw32-strip IntelJ3.exe
fi

# For Linux platform
rm -f *.o
rm -f ./telnet/*.o
make --makefile=Makefile_linux clean
make --makefile=Makefile_linux
strip IntelJ3 

cd -
