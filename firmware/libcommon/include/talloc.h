#pragma once

#include <stdlib.h>
#include <stdarg.h>

/* minimalistic emulation of core talloc API functions used by msgb.c */

#define __TALLOC_STRING_LINE1__(s)    #s
#define __TALLOC_STRING_LINE2__(s)   __TALLOC_STRING_LINE1__(s)
#define __TALLOC_STRING_LINE3__  __TALLOC_STRING_LINE2__(__LINE__)
#define __location__ __FILE__ ":" __TALLOC_STRING_LINE3__

#define talloc_zero(ctx, type) (type *)_talloc_zero(ctx, sizeof(type), #type)
#define talloc_zero_size(ctx, size) _talloc_zero(ctx, size, __location__)
void *_talloc_zero(const void *ctx, size_t size, const char *name);

#define talloc_free(ctx) _talloc_free(ctx, __location__)
int _talloc_free(void *ptr, const char *location);

/* Unsupported! */
#define talloc_size(ctx, size) talloc_named_const(ctx, size, __location__)
void *talloc_named_const(const void *context, size_t size, const char *name);
void talloc_set_name_const(const void *ptr, const char *name);
char *talloc_strdup(const void *t, const char *p);
void *talloc_pool(const void *context, size_t size);
