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

#include "memory.h"

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
