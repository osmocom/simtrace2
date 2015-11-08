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
