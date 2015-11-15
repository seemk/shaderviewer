/*
 * Copyright 2010-2015 Branimir Karadzic. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef BX_CONFIG_H_HEADER_GUARD
#define BX_CONFIG_H_HEADER_GUARD

#include "platform.h"

#ifndef BX_CONFIG_ALLOCATOR_DEBUG
#	define BX_CONFIG_ALLOCATOR_DEBUG 0
#endif // BX_CONFIG_DEBUG_ALLOC

#ifndef BX_CONFIG_ALLOCATOR_CRT
#	define BX_CONFIG_ALLOCATOR_CRT 1
#endif // BX_CONFIG_ALLOCATOR_CRT

#ifndef  BX_CONFIG_SPSCQUEUE_USE_MUTEX
#	define BX_CONFIG_SPSCQUEUE_USE_MUTEX 0
#endif // BX_CONFIG_SPSCQUEUE_USE_MUTEX

#ifndef BX_CONFIG_CRT_FILE_READER_WRITER
#	define BX_CONFIG_CRT_FILE_READER_WRITER (0 \
				|| BX_PLATFORM_ANDROID \
				|| BX_PLATFORM_FREEBSD \
				|| BX_PLATFORM_EMSCRIPTEN \
				|| BX_PLATFORM_IOS \
				|| BX_PLATFORM_LINUX \
				|| BX_PLATFORM_OSX \
				|| BX_PLATFORM_QNX \
				|| BX_PLATFORM_RPI \
				|| BX_PLATFORM_WINDOWS \
				|| BX_PLATFORM_WINRT \
				? 1 : 0)
#endif // BX_CONFIG_CRT_FILE_READER_WRITER

#ifndef BX_CONFIG_SEMAPHORE_PTHREAD
#	define BX_CONFIG_SEMAPHORE_PTHREAD (BX_PLATFORM_OSX || BX_PLATFORM_IOS)
#endif // BX_CONFIG_SEMAPHORE_PTHREAD

#ifndef BX_CONFIG_SUPPORTS_THREADING
#	define BX_CONFIG_SUPPORTS_THREADING !(BX_PLATFORM_EMSCRIPTEN)
#endif // BX_CONFIG_SUPPORTS_THREADING

#endif // BX_CONFIG_H_HEADER_GUARD
