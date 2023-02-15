#!/bin/sh
# This script is used to build NDS32 burner for Linux and Windows platform.

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
rm -f PAR_burn PAR_burn.exe
rm -f platform.c spiflash-MXIC.c
rm -f Makefile_SPIburn_linux Makefile_SPIburn_win
rm -f Makefile_linux Makefile_win
rm -f build_IntelJ3.sh build_SPIburn.sh

# For Windows platform
rm -f *.o
rm -f ./telnet/*.o
if type i386-mingw32-gcc 2> /dev/null; then
    export PATH=/home/users/sqa/mingw/bin:$PATH
    make --makefile=Makefile_PARburn_win clean
    make --makefile=Makefile_PARburn_win
    i386-mingw32-strip PAR_burn.exe
fi

# For Linux platform
rm -f *.o
rm -f ./telnet/*.o
if [[ "$OSTYPE" == "darwin"* ]]; then
	make --makefile=Makefile_PARburn_darwin clean
	make --makefile=Makefile_PARburn_darwin
else
	make --makefile=Makefile_PARburn_linux clean
	make --makefile=Makefile_PARburn_linux
fi
strip PAR_burn

cd -
