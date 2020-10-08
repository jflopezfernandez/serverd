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

#ifndef PROJECT_INCLUDES_MEMORY_H
#define PROJECT_INCLUDES_MEMORY_H

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
__attribute__((malloc,returns_nonnull))
void* allocate_memory(size_t size);

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
__attribute__((nonnull(1)))
void safe_free(void** ptr);

/**
 * @def FREE
 * @brief Convenience macro for calling safe_free()
 *
 * @details This macro is a more convenient way of calling
 * safe_free(), especially since it takes care of casting
 * the pointer to void**.
 *
 */
#ifndef FREE
#define FREE(ptr) ((void **) &ptr)
#endif

#endif /** PROJECT_INCLUDES_MEMORY */
