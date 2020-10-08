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

#ifndef PROJECT_INCLUDES_CONFIGURATION_H
#define PROJECT_INCLUDES_CONFIGURATION_H

/**
 * This object contains all valid server configuration
 * options.
 *
 */
struct configuration_options_t {

    /**
     * The server configuration file name.
     *
     * A different configuration filename can be passed in
     * via the command-line, and users can also request the
     * server bypass all configuration file parsing and
     * use simply configuration directives passed in via the
     * command-line options.
     *
     */
    const char* configuration_filename;
    
    /**
     * The hostname for the server.
     *
     * @todo The user should be able to configure the
     * hostname for the server on a per-virtual host basis.
     * 
     */
    const char* hostname;

    /**
     * The port the server will use to listen for incoming
     * client connections on.
     * 
     */
    const char* port;

    const char* document_root_directory;
};

/**
 * This function is the public interface for the configuration
 * module, which handles all of the necessary configuration
 * parsing required to configure the server.
 *
 */
struct configuration_options_t* initialize_server_configuration(int argc, char *argv[]);

#endif /** PROJECT_INCLUDES_CONFIGURATION_H */
