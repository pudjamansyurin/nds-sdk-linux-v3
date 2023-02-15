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
rm -f SPI_burn SPI_burn.exe
rm -f platform-ag101p.c smcflash-IntelJ3.c smcflash-Micron.c
rm -f Makefile_PARburn_linux Makefile_PARburn_win
rm -f Makefile_linux Makefile_win
rm -f build_PARburn.sh build_IntelJ3.sh

# For Windows platform
rm -f *.o
rm -f ./telnet/*.o
if type i386-mingw32-gcc 2> /dev/null; then
    export PATH=/home/users/sqa/mingw/bin:$PATH
    make --makefile=Makefile_SPIburn_win clean
    make --makefile=Makefile_SPIburn_win
    i386-mingw32-strip SPI_burn.exe
fi

# For Linux platform
rm -f *.o
rm -f ./telnet/*.o
if [[ "$OSTYPE" == "darwin"* ]]; then
	make --makefile=Makefile_SPIburn_darwin clean
	make --makefile=Makefile_SPIburn_darwin
else
	make --makefile=Makefile_SPIburn_linux clean
	make --makefile=Makefile_SPIburn_linux
fi
strip SPI_burn

cd -
