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

#ifndef PROJECT_INCLUDES_ERROR_H
#define PROJECT_INCLUDES_ERROR_H

/**
 * Exit with an error status and message.
 *
 * @details This function accepts a printf-style format
 * string to allow for the output of detailed error messages
 * and status codes. This function calls exit(3) directly
 * after printing its output to standard out.
 *
 */
__attribute__((noreturn,format(printf,1,2)))
void fatal_error(const char* format, ...);

#endif /** PROJECT_INCLUDES_ERROR_H */