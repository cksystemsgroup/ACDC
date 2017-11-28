# ACDC Benchmark

We present ACDC, an open-source benchmark that may be configured to emulate
explicit single- and multi-threaded memory allocation, sharing, access, and
deallocation behavior that exposes virtually any relevant allocator performance
differences. ACDC mimics periodic memory allocation and deallocation (AC) as
well as persistent memory (DC). Memory may be allocated thread-locally and
shared among multiple threads to study multi-core scalability and even false
sharing. Memory may be deallocated by threads other than the allocating threads
to study blowup memory fragmentation. Memory may be accessed and deallocated
sequentially in allocation order or in tree-like traversals to expose allocator
deficiencies in exploiting spatial locality. We demonstrate ACDC’s capabilities
with six state-of-the-art allocators for C/C++ in an empirical study which also
reveals interesting performance differences between the allocators.

## Compiling ACDC
* Use a recent version of `gcc`. ACDC is known to work with gcc 4.6.3 on Linux x86-64
* You need a recent version of `gnuplot` and `epstopdf` to create the plots
* Run `./install-deps.sh` to install Gyp
* You need to put the allocator libraries in the allocators sub directory of ACDC. Check and run the `install-allocators.sh` scripts for some default allocators
* To create the Makefiles run: `./build/gyp/gyp --depth=. acdc.gyp`
* Build ACDC with: `BUILDTYPE=Release make`
* Add the allocators directory to your library path, e.g., `export LD_LIBRARY_PATH=./allocators`
* Run e.g. `LD_PRELOAD=./allocators/some-allocator-lib.so ./out/Release/acdc -h` for usage information
* Copy and edit the experiment definitions in `sample.sh` in the experiments directory.
* Run an experiment using: `./scripts/run_acdc.sh experiments/[your-config].sh`

## License

Copyright (c) 2012-2013, the ACDC Project Authors.
All rights reserved. Please see the AUTHORS file for details.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met: 

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer. 
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution. 

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR 
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND 
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those
of the authors and should not be interpreted as representing official policies, 
either expressed or implied, of the ACDC Project.
