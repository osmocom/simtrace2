/* General utilities
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

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#ifdef __ARM
#define local_irq_save(x)					\
	({							\
		x = __get_PRIMASK();				\
	 	__disable_irq();				\
	})

#define local_irq_restore(x)					\
	 	__set_PRIMASK(x)
#else
#warning "local_irq_{save,restore}() not implemented"
#define local_irq_save(x)
#define local_irq_restore(x)
#endif
