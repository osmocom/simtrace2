#pragma once

#include <stdio.h>

#define TRACE_DEBUG(x, args ...)	printf(x, ## args)
#define TRACE_INFO	TRACE_DEBUG
#define TRACE_ERROR	TRACE_DEBUG
