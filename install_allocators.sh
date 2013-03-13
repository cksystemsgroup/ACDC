#!/bin/bash

# This scripts downloads and builds the allocators under test
# in the allocators subdirectory
#
# Feel free to adapt this scipts to your needs

mkdir -p allocators
cd allocators

# jemalloc
git clone git://canonware.com/jemalloc.git
cd jemalloc
./autogen.sh
make -j 2
cd ..
ln -s jemalloc/lib/libjemalloc.so.1 libjemalloc.so.1
ln -s jemalloc/lib/libjemalloc.so.1 libjemalloc.so

# llalloc
wget http://locklessinc.com/downloads/lockless_allocator_linux.tgz
tar -xzf lockless_allocator_linux.tgz
ln -s lockless_allocator_linux/64bit/libllalloc.so.1.4 libllalloc.so.1.4
ln -s lockless_allocator_linux/64bit/libllalloc.so.1.4 libllalloc.so

# ptmalloc3
wget http://www.malloc.de/malloc/ptmalloc3-current.tar.gz
tar -xzf ptmalloc3-current.tar.gz
cd ptmalloc3/
make linux-shared OPT_FLAGS='-O2 -pthread'
cd ..
ln -s ptmalloc3/libptmalloc3.so libptmalloc3.so

# tbb
wget http://threadingbuildingblocks.org/sites/default/files/software_releases/linux/tbb41_20130116oss_lin.tgz
tar -xzf tbb41_20130116oss_lin.tgz
ln -s tbb41_20130116oss/lib/intel64/cc4.1.0_libc2.4_kernel2.6.16.21/libtbbmalloc.so.2 libtbbmalloc.so.2
ln -s tbb41_20130116oss/lib/intel64/cc4.1.0_libc2.4_kernel2.6.16.21/libtbbmalloc.so.2 libtbbmalloc.so
ln -s tbb41_20130116oss/lib/intel64/cc4.1.0_libc2.4_kernel2.6.16.21/libtbbmalloc_proxy.so.2 libtbbmalloc_proxy.so.2
ln -s tbb41_20130116oss/lib/intel64/cc4.1.0_libc2.4_kernel2.6.16.21/libtbbmalloc_proxy.so.2 libtbbmalloc_proxy.so

# tcmalloc
sudo apt-get install libgoogle-perftools-dev

#streamflow
git clone git://github.com/scotts/streamflow.git
cd streamflow/
make
cd ..
ln -s streamflow/libstreamflow.so libstreamflow.so

#hoard
git clone --recursive https://github.com/emeryberger/Hoard
cd Hoard/src
make linux-gcc-x86-64
cd ../../
ln -s Hoard/src/libhoard.so libhoard.so
