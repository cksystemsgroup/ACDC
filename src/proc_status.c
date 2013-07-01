 /*
 * Copyright (c) 2012, the ACDC Project Authors.
 * All rights reserved. Please see the AUTHORS file for details.
 * Use of this source code is governed by a BSD license that
 * can be found in the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include "proc_status.h"

#ifdef __linux__

#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>

#ifndef LINE_MAX
#define LINE_MAX 200
#endif

struct proc_status {
	long vm_peak;
	long vm_size;
	long vm_hwm;
	long vm_rss;
	long vm_data;
};

volatile struct proc_status stat;

static inline void fgets_nn(char *str, int size, FILE *stream) {
	char *s = str;
	int c;
	while (--size > 0 && (c = fgetc(stream)) != EOF)
		*s++ = c;
	*s = '\0';
}

static long get_long_from_line(char *str, int index) {
	char *token;
	int i = 0;
	const char *delimiter = " ";
	char *sp = str;
	while (*sp != '\0' && i < LINE_MAX) {
		if (*sp == '\t') *sp = ' ';
		++i;
		++sp;
	}
	
	for (sp = str, i = 0; ; sp = NULL) {
		token = strtok(sp, delimiter);
		if (token == NULL) break;
		if (i++ == index) return atol(token);
	}
	return 0;
}

void update_proc_status(pid_t pid) {
	
	char filename[50]; //more than enough for /proc/[pid]/status
	snprintf(filename, 50, "/proc/%d/status", pid);
	FILE *stream;
	char buf[LINE_MAX] = {0};
	//size_t len;

	stream = fopen(filename, "r");
	if (!stream) {
		perror("fopen");
		exit(EXIT_FAILURE);
	}

	//reset
	stat.vm_data = 0;
	stat.vm_hwm = 0;
	stat.vm_peak = 0;
	stat.vm_rss = 0;
	stat.vm_size = 0;

	while (fgets(buf, LINE_MAX, stream)) {
		if (strncmp(buf, "VmPeak", 6) == 0) {
			stat.vm_peak = get_long_from_line(buf, 1);
		}
		if (strncmp(buf, "VmSize", 6) == 0) {
			stat.vm_size = get_long_from_line(buf, 1);
		}
		if (strncmp(buf, "VmHWM", 5) == 0) {
			stat.vm_hwm = get_long_from_line(buf, 1);
		}
		if (strncmp(buf, "VmRSS", 5) == 0) {
			stat.vm_rss = get_long_from_line(buf, 1);
		}
		if (strncmp(buf, "VmData", 6) == 0) {
			stat.vm_data = get_long_from_line(buf, 1);
		}
	}

	if (fclose(stream)) {
		perror("fclose");
		exit(EXIT_FAILURE);
	}
}

long get_vm_peak() {
	return stat.vm_peak;
}
long get_vm_size() {
	return stat.vm_size;
}
long get_resident_set_size() {
	return stat.vm_rss;
}
long get_high_water_mark() {
	return stat.vm_hwm;
}
long get_data_segment_size() {
	return stat.vm_data;
}


#else // everyting but linux not supported yet

long get_resident_set_size() {
	printf("WARNING: resource information not supported for this platform\n");
	return 0;
}
long get_high_water_mark() {
	printf("WARNING: resource information not supported for this platform\n");
	return 0;
}
long get_data_segment_size() {
	printf("WARNING: resource information not supported for this platform\n");
	return 0;
}

#endif // __linux__
