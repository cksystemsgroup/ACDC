/*
 * Copyright (c) 2012, the ACDC Project Authors.
 * All rights reserved. Please see the AUTHORS file for details.
 * Use of this source code is governed by a BSD license that
 * can be found in the LICENSE file.
 */

#include "acdc.h"

LClass *allocate_heap_class(unsigned int max_lifetime) {

	LClass *ec = calloc_meta(max_lifetime, sizeof(LClass));
	//calloc creates zeroed memory, i.e., the first and last
	//pointers of each LClass are NULL
	return ec;
}

void lclass_insert_after(LClass *list, LSCNode *after, LSCNode *c) {
	c->prev = after;
	c->next = after->next;
	if (after->next == NULL)
		list->last = c;
	else
		after->next->prev = c;
	after->next = c;
}

void lclass_insert_before(LClass *list, LSCNode *before, LSCNode *c) {
	c->prev = before->prev;
	c->next = before;
	if (before->prev == NULL)
		list->first = c;
	else
		before->prev->next = c;
	before->prev = c;
}

void lclass_insert_beginning(LClass *list, LSCNode *c) {
	if (list->first == NULL) {
		list->first = c;
		list->last = c;
		c->prev = NULL;
		c->next = NULL;
	} else {
		lclass_insert_before(list, list->first, c);
	}
}

void lclass_insert_end(LClass *list, LSCNode *c) {
	if (list->last == NULL)
		lclass_insert_beginning(list, c);
	else
		lclass_insert_after(list, list->last, c);
}

void lclass_remove(LClass *list, LSCNode *c) {
	if (c->prev == NULL)
		list->first = c->next;
	else
		c->prev->next = c->next;
	if (c->next == NULL)
		list->last = c->prev;
	else
		c->next->prev = c->prev;
}

