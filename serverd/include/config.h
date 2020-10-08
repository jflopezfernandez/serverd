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

#ifndef PROJECT_INCLUDES_CONFIG_H
#define PROJECT_INCLUDES_CONFIG_H

/**
 * Make sure we are compiling on Linux.
 *
 * It will be great to be able to support other platforms
 * eventually, particularly the BSD's, and even Windows, but
 * we're first going to limit ourselves to Linux to ensure
 * we can take advantage of all of the kernel-specific
 * optimization tools first. In particular, the epoll(7) API
 * is Linux-specific.
 *
 */
#ifndef __linux__
#error "This platform is not supported."
#endif

#endif /** PROJECT_INCLUDES_CONFIG_H */
