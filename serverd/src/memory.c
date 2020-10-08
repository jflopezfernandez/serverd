/**
 * serverd - Modern web server daemon
 * Copyright (C) 2020 Jose Fernando Lopez Fernandez
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the
 * implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth
 * Floor, Boston, MA 02110-1301 USA.
 *
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>

/**
 * Allow for the use of the jemalloc memory allocator instead
 * of the standard library malloc.
 *
 */
#if defined(ENABLE_JEMALLOC)
#include <jemalloc/jemalloc.h>
#endif

#include "serverd.h"
#include "error.h"
#include "memory.h"

/**
 * Allocate number of bytes specified by size argument.
 *
 * This function represents the public interface to the
 * serverd memory module. This function replaces all external
 * calls to malloc(3).
 *
 * This interface allows all error-checking to take place
 * internally, removing the burden from the caller. In
 * addition, this interface allows for the transparent
 * replacement of the memory allocator apparatus without
 * requiring any additional external refactoring.
 *
 */
void* allocate_memory(size_t size) {
    /**
     * Allocate the memory block of requested size.
     *
     * @note If jemalloc is enabled, this call to malloc is
     * transparently replaced to a call to je_malloc via a
     * preprocessor macro definition. This can be disabled
     * by defining JEMALLOC_NO_RENAME, but the default is
     * probably both simpler and preferable.
     *
     */
    void* memory_block = malloc(size);

    /**
     * Ensure that pointer returned by the call to malloc
     * points to a valid memory block segment.
     *
     * Both the C standard library and jemalloc memory
     * allocation function returns a NULL pointer if there
     * is a problem allocating the requested memory. This
     * check is therefore necessary regardless of the malloc
     * implementation we are using.
     *
     */
    if (memory_block == NULL) {
        /**
         * If the returned memory block is invalid, there
         * was a significant problem.
         *
         * At the moment, we are not going to do any complex
         * error handling; we are simply going to terminate
         * the process with an error status and go from
         * there.
         *
         */
        fatal_error("[Error] %s: %s\n", "Memory allocation failure in call to malloc()", strerror(errno));
    }

    /**
     * Return the newly-allocated memory block.
     *
     * Since we have already performed the requisite
     * validation, client code no longer needs to perform
     * this check, eliminating the need for that annoying
     * boilerplate, and thereby reducing code size.
     *
     */
    return memory_block;
}

/**
 * Prevent double-free errors
 *
 * @details This function works by taking a pointer argument
 * by reference (thus effectively requiring a double-pointer
 * argument) and checking whether the pointer has already
 * been set to NULL. If it hasn't, the function calls free()
 * on the data pointed to by ptr, and then setting *ptr to
 * NULL. This allows us to subsequently prevent a double-
 * free error by verifying the memory block pointed to by
 * the next *ptr is not equal to NULL;
 *
 */
void safe_free(void** ptr) {
    /**
     * The C standard library implementation dictates that a
     * call to free(3) with a NULL argument will do nothing,
     * but it's best to check here first and save ourselves
     * the overhead of carrying out a system call.
     *
     */
    if (*ptr == NULL) {
        /**
         * Since the pointed-to memory block is NULL, there
         * is nothing for us to do. Simply return.
         *
         */
        return;
    }

    /**
     * Free the memory pointed to by *ptr. There is no
     * return value from a call to free(3).
     *
     */
    free(*ptr);

    /**
     * Calling free(3) on the same memory block twice
     * without an intermediate allocation via malloc(3) or
     * its derivatives leads to undefined behavior, as well
     * as some nasty security bugs that collectively go by
     * the name of double-free errors.
     *
     * To prevent these errors, we take a pointer argument
     * by reference and explicitly set it to NULL here so
     * that we can prevent accidentally freeing this memory
     * block twice.
     *
     */
    *ptr = NULL;
}
