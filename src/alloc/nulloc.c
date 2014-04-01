/*
 * Copyright (c) 2013, the ACDC Project Authors.
 * All rights reserved. Please see the AUTHORS file for details.
 * Use of this source code is governed by a BSD license that
 * can be found in the LICENSE file.
 */

#include "nulloc.h"
#include "../caches.h"

__thread long long dummy __attribute__((aligned(L1_LINE_SZ)));

void *nulloc_alloc(size_t size) {
        dummy = 0;
        return (void*)&dummy;
}

void nulloc_free(void* ptr) {
        return;
}

