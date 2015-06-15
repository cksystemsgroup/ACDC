#!/bin/bash

# This scripts downloads and builds the allocators under test
# in the allocators subdirectory
#
# Feel free to adapt this scipts to your needs

ALL_ALLOCATORS="jemalloc llalloc ptmalloc3 tbb tcmalloc streamflow hoard scalloc"
WD=`pwd`

function install_allocator {
	ALLOCATOR=$1

	if [[ $ALLOCATOR == "jemalloc" ]]; then
		# jemalloc
		rm -rf jemalloc*
		wget http://ftp.de.debian.org/debian/pool/main/j/jemalloc/jemalloc_3.6.0.orig.tar.bz2
		tar -xvjf jemalloc_3.6.0.orig.tar.bz2
		mv jemalloc-3.6.0 jemalloc
		cd jemalloc
		./autogen.sh
		make -j 2
		cd $WD/allocators
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
		cd $WD/allocators
		ln -s ptmalloc3/libptmalloc3.so libptmalloc3.so
	fi
	
	if [[ $ALLOCATOR == "tbb" ]]; then
		# tbb
		rm -rf tbb*
		rm -rf libtbb*
                wget https://www.threadingbuildingblocks.org/sites/default/files/software_releases/linux/tbb43_20150209oss_lin.tgz
		tar -xzf tbb43_20150209oss_lin.tgz
		ln -s tbb43_20150209oss/lib/intel64/gcc4.4/libtbbmalloc.so.2 libtbbmalloc.so.2
		ln -s tbb43_20150209oss/lib/intel64/gcc4.4/libtbbmalloc.so.2 libtbbmalloc.so
		ln -s tbb43_20150209oss/lib/intel64/gcc4.4/libtbbmalloc_proxy.so.2 libtbbmalloc_proxy.so.2
		ln -s tbb43_20150209oss/lib/intel64/gcc4.4/libtbbmalloc_proxy.so.2 libtbbmalloc_proxy.so
	fi
	
	if [[ $ALLOCATOR == "tcmalloc" ]]; then
		# tcmalloc
                rm -rf libtcmalloc*
		sudo apt-get install libtcmalloc-minimal4
		ln -s /usr/lib/libtcmalloc_minimal.so.4 libtcmalloc.so
	fi


        if [[ $ALLOCATOR == "michael" ]]; then
                sudo apt-get install libc6-dev-i386 gcc-multilib g++-multilib
                # Re-implementation of Michael's allocator done by the Streamflow authors.
                rm -rf michael*
                rm -rf libmichael*
                wget http://people.cs.vt.edu/~scschnei/streamflow/michael.tar.gz
                tar -xzf michael.tar.gz
                cd michael
                make
                cd $WD/allocators
                ln -s michael/libmichael.so libmichael.so
        fi
	
	if [[ $ALLOCATOR == "streamflow" ]]; then
		#streamflow
		sudo apt-get install libnuma-dev
		rm -rf *streamflow*
		git clone git://github.com/scotts/streamflow.git
		cd streamflow/
		sed -i '1s/^/#include <unistd.h>\n/' malloc_new.cpp
		make
		cd $WD/allocators
		ln -s streamflow/libstreamflow.so libstreamflow.so
	fi
	
	if [[ $ALLOCATOR == "hoard" ]]; then
		#hoard
		rm -rf Hoard
		rm -rf libhoard*
		git clone https://github.com/emeryberger/Hoard
		cd Hoard
		#git checkout 604d959
                git submodule init
                git submodule update
		cd src
		make linux-gcc-x86-64
		cd $WD/allocators
		ln -s Hoard/src/libhoard.so libhoard.so
	fi

	if [[ $ALLOCATOR == "supermalloc" ]]; then
		#hoard
		rm -rf SuperMalloc
		rm -rf libsupermalloc*

                git clone https://github.com/kuszmaul/SuperMalloc.git

		cd SuperMalloc/release
                make
		cd $WD/allocators
		ln -s SuperMalloc/release/lib/libsupermalloc_pthread.so libsupermalloc.so
	fi


	if [[ $ALLOCATOR == "scalloc" ]]; then
		#scalloc
            if [ -f ../install_scalloc.sh ]; then
                    ../install_scalloc.sh
            else
                  rm -rf scalloc
                  rm -rf libscalloc.so
                  git clone https://github.com/cksystemsgroup/scalloc.git
                  cd scalloc
                  git checkout 0.9.0
                  tools/make_deps.sh
                  build/gyp/gyp --depth=. scalloc.gyp
                  BUILDTYPE=Release make
                  cd $WD/allocators
                  ln -s scalloc/out/Release/lib.target/libscalloc.so libscalloc.so
            fi
        fi
}


mkdir -p $WD/allocators
cd $WD/allocators

if [[ $# -ge 1 ]]; then
	ALL_ALLOCATORS=$@
fi

for ARG in $ALL_ALLOCATORS; do
        cd $WD/allocators
	echo "Installing $ARG..."
	install_allocator $ARG
done

#done
cd $WD
