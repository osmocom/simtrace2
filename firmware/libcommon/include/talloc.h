/* Memory allocation library
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307, USA
 */
#pragma once

#include <stdlib.h>
#include <stdio.h>
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
void talloc_report(const void *ptr, FILE *f);
