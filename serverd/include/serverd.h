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

#ifndef PROJECT_INCLUDES_SERVERD_H
#define PROJECT_INCLUDES_SERVERD_H

#ifndef PROJECT_INCLUDES_CONFIG_H
#include "config.h"
#endif

#if !defined(TRUE) || !defined(FALSE)
/**
 * @brief Definitions for true and false
 *
 * @details This enum contains definitions for the boolean
 * values TRUE and FALSE, which represent 1 and 0,
 * respectively.
 * 
 */
enum { FALSE = 0, TRUE = !FALSE };
#endif

#endif /** PROJECT_INCLUDES_SERVERD_H */
