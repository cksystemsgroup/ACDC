/*
 * Copyright (c) 2013, the ACDC Project Authors.
 * All rights reserved. Please see the AUTHORS file for details.
 * Use of this source code is governed by a BSD license that
 * can be found in the LICENSE file.
 */

#ifndef NULLOC_H
#define NULLOC_H

#include <stddef.h>

void *nulloc_alloc(size_t size);
void nulloc_free(void* ptr);

#endif // NULLOC_H
