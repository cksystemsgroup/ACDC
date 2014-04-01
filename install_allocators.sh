#!/bin/bash

# This scripts downloads and builds the allocators under test
# in the allocators subdirectory
#
# Feel free to adapt this scipts to your needs

ALL_ALLOCATORS="jemalloc llalloc ptmalloc3 tbb tcmalloc streamflow hoard scalloc"

function install_allocator {
	ALLOCATOR=$1

	if [[ $ALLOCATOR == "jemalloc" ]]; then
		# jemalloc
		rm -rf jemalloc*
		wget http://ftp.de.debian.org/debian/pool/main/j/jemalloc/jemalloc_3.5.1.orig.tar.bz2
		tar -xvjf jemalloc_3.5.1.orig.tar.bz2
		mv jemalloc-3.5.1 jemalloc
		cd jemalloc
		./autogen.sh
		make -j 2
		cd ..
		ln -s jemalloc/lib/libjemalloc.so.1 libjemalloc.so.1
		ln -s jemalloc/lib/libjemalloc.so.1 libjemalloc.so
	fi
	
	if [[ $ALLOCATOR == "llalloc" ]]; then
		# llalloc
		rm -rf lockless_allocator_linux*
		wget http://locklessinc.com/downloads/lockless_allocator_linux.tgz
		tar -xzf lockless_allocator_linux.tgz
		ln -s lockless_allocator_linux/64bit/libllalloc.so.1.4 libllalloc.so.1.4
		ln -s lockless_allocator_linux/64bit/libllalloc.so.1.4 libllalloc.so
	fi

	if [[ $ALLOCATOR == "ptmalloc3" ]]; then
		# ptmalloc3
		rm -rf ptmalloc*
		wget http://www.malloc.de/malloc/ptmalloc3-current.tar.gz
		tar -xzf ptmalloc3-current.tar.gz
		cd ptmalloc3/
		make linux-shared OPT_FLAGS='-O2 -pthread'
		cd ..
		ln -s ptmalloc3/libptmalloc3.so libptmalloc3.so
	fi
	
	if [[ $ALLOCATOR == "tbb" ]]; then
		# tbb
		rm -rf tbb*
		rm -rf libtbb*
		wget https://www.threadingbuildingblocks.org/sites/default/files/software_releases/linux/tbb42_20131003oss_lin.tgz
		tar -xzf tbb42_20131003oss_lin.tgz
		ln -s tbb42_20131003oss/lib/intel64/gcc4.4/libtbbmalloc.so.2 libtbbmalloc.so.2
		ln -s tbb42_20131003oss/lib/intel64/gcc4.4/libtbbmalloc.so.2 libtbbmalloc.so
		ln -s tbb42_20131003oss/lib/intel64/gcc4.4/libtbbmalloc_proxy.so.2 libtbbmalloc_proxy.so.2
		ln -s tbb42_20131003oss/lib/intel64/gcc4.4/libtbbmalloc_proxy.so.2 libtbbmalloc_proxy.so
	fi
	
	if [[ $ALLOCATOR == "tcmalloc" ]]; then
		# tcmalloc
		sudo apt-get install libgoogle-perftools-dev
		ln -s /usr/lib/libtcmalloc_minimal.so.0 libtcmalloc.so
	fi
	
	if [[ $ALLOCATOR == "streamflow" ]]; then
		#streamflow
		sudo apt-get install libnuma-dev
		rm -rf streamflow
		git clone git://github.com/scotts/streamflow.git
		cd streamflow/
		sed -i '1s/^/#include <unistd.h>\n/' malloc_new.cpp
		make
		cd ..
		ln -s streamflow/libstreamflow.so libstreamflow.so
	fi
	
	if [[ $ALLOCATOR == "hoard" ]]; then
		#hoard
		rm -rf Hoard
		rm -rf libhoard*
		git clone --recursive https://github.com/emeryberger/Hoard
		cd Hoard
		#git checkout 3.10
		cd src
		make linux-gcc-x86-64
		cd ../../
		ln -s Hoard/src/libhoard.so libhoard.so
	fi

	if [[ $ALLOCATOR == "scalloc" ]]; then
		#scalloc
    if [ -f ../install_scalloc.sh ]; then 
      ../install_scalloc.sh
    else
      rm -rf scalloc
      git clone https://github.com/cksystemsgroup/scalloc.git
      cd scalloc/
		  git checkout release
		  tools/make_deps.sh

		  ./build/gyp/gyp --depth=. scalloc.gyp
		  BUILDTYPE=Release make
		  cp out/Release/lib.target/libscalloc.so out/Release/libscalloc.so
      cd ..
      rm -rf libscalloc*
      ln -s scalloc/out/Release/libscalloc.so libscalloc.so
      ln -s scalloc/out/Release/libscalloc.so libscalloc.so.0
    fi
	fi
}


mkdir -p allocators
cd allocators

if [[ $# -ge 1 ]]; then
	ALL_ALLOCATORS=$@
fi

for ARG in $ALL_ALLOCATORS; do
	echo "Installing $ARG..."
	install_allocator $ARG
done

#done
cd ..
