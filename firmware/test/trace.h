#pragma once

#include <stdio.h>

#define TRACE_DEBUG(x, args ...)	printf(x, ## args)
