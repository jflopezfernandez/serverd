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

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include <unistd.h>

#include <sys/epoll.h>
#include <sys/select.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <fcntl.h>
#include <getopt.h>
#include <netdb.h>

#include <arpa/inet.h>

#include <netinet/in.h>
#include <netinet/tcp.h>

#include "memory.h"

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

/**
 * @def EPOLL_MAX_EVENTS
 * @brief Maximum events returned per call to epoll_wait(2).
 * 
 */
#ifndef EPOLL_MAX_EVENTS
#define EPOLL_MAX_EVENTS (10)
#endif

/**
 * @def DEFAULT_HOSTNAME
 * @brief The server's default hostname.
 * 
 */
#ifndef DEFAULT_HOSTNAME
#define DEFAULT_HOSTNAME "localhost"
#endif

/**
 * @def DEFAULT_PORT
 * @brief The default port the server should listen on.
 *
 * @details Superuser privileges are required in order for
 * any application to bind to a port with a number in the
 * range 1-1024. Common port numbers for development or
 * testing web servers to use are 3000 and 8080. If started
 * on port 80, the server will respond to and service client
 * requests like any normal server would, although it is
 * also possible to use a reverse proxy for TLS termination
 * while native serverd TLS support is implemented.
 *
 * @author Jose Fernando Lopez Fernandez
 * @date October 7, 2020 [08:13:05 AM EDT]
 * 
 */
#ifndef DEFAULT_PORT
#define DEFAULT_PORT "8080"
#endif

/**
 * Program Options
 *
 * @details This array contains the list of valid long
 * options recognized by the server, as well as whether each
 * option accepts (or sometimes even requires) an additional
 * value when specified. If the third argument is NULL, the
 * call to getopt_long returns the fourth argument.
 * Otherwise, the call to getopt_long will return the value
 * specified by that fourth argument.
 * 
 */
static struct option long_options[] = {
    { "help",     no_argument,       0, 'h' },
    { "version",  no_argument,       0,  0  },
    { "hostname", required_argument, 0, 'H' },
    { "port",     required_argument, 0, 'p' },
    { 0, 0, 0, 0 }
};

/**
 * Program Help Menu
 *
 * @details This is the menu display to the user when the -h
 * or --help command-line arguments are passed in. The
 * option list is current as of October 7, 2020 [11:13 AM EDT],
 * but the project version information still needs to be
 * sorted out.
 *
 * @author Jose Fernando Lopez Fernandez
 * @date October 7, 2020 [11:13:52 AM EDT]
 *
 * @todo Establish project versioning system
 *
 */
static const char* help_menu = 
    "serverd version: 0.0.1\n"
    "Usage: serverd [options]\n"
    "\n"
    "Configuration Options:\n"
    "  -H, --hostname <str>     Server hostname\n"
    "  -p, --port <int>         Port number to bind to\n"
    "\n"
    "Generic Options:\n"
    "  -h, --help               Display this help menu and exit\n"
    "      --version            Display server version information\n"
    "\n";

/**
 * This is the entry point of the server.
 * 
 * @param argc 
 * @param argv
 *
 * @return int 
 */
int main(int argc, char *argv[])
{
    /**
     * @brief The hostname the server will use.
     *
     * @todo The user should be able to configure the
     * hostname for the server on a per-virtual host basis.
     * 
     */
    const char* hostname = DEFAULT_HOSTNAME;

    /**
     * @brief The port the server will use to listen for incoming
     * client connections on.
     * 
     */
    const char* port = DEFAULT_PORT;

    /**
     * Enter command-line argument processing loop.
     *
     * The option string dictates what short options are
     * valid when starting the server with command-line
     * arguments. Some of these short options interact with
     * the long options array defined above.
     *
     * An option that must be specified with an argument has
     * a colon after its letter. If the argument for a
     * particular option is optional, that option's letter
     * will be followed by two colons.
     *
     */
    while (TRUE) {

        /**
         * @details This variable is set by getopt_long(3)
         * on each iteration of the option-parsing loop. It
         * is used as the index to the long options array
         * when a long option is detected in the input.
         *
         */
        int option_index = 0;

        /**
         * Option Char
         *
         * @details Each call to getopt_long(3) returns a
         * sentinel character that either uniquely identifies
         * a command-line option or can be used to do so in
         * the case where the return value is zero (0).
         *
         * This latter case is useful because it allows for
         * the use of both long and short option forms, both
         * of which this server uses.
         *
         */
        int c = getopt_long(argc, argv, "hvH:p:", long_options, &option_index);

        /**
         * When getopt(3), et. al, are done parsing all of
         * the passed-in command-line arguments, they return
         * a value of -1. We can therefore simply and safely
         * break out of this loop if we detect this here.
         *
         */
        if (c == -1) {
            /**
             * Any additional command-line arguments that
             * were not deemed to be valid command-line
             * options are considered to be positional
             * arguments, and they can be iterated over
             * using the optind variable provided by the
             * getopt*(3) function(s).
             *
             * All of these positional arguments, if they
             * exist, will be located at argv[optind] until
             * argv[argc - 1].
             *
             */
            break;
        }

        switch (c) {
            case 0: {
                if (strcmp(long_options[option_index].name, "version") == 0) {
                    /** @todo Configure project versioning info */
                    printf("Version Info\n");
                    return EXIT_SUCCESS;
                }

                /** @todo What happens if we've gotten to this point? */
            } break;

            /**
             * Configure Server Hostname
             */
            case 'H': {
                hostname = optarg;
            } break;

            /**
             * Display Help Menu
             */
            case 'h': {
                printf("%s", help_menu);
                return EXIT_SUCCESS;
            } break;

            /**
             * Configure Server Port
             */
            case 'p': {
                port = optarg;
            } break;

            /** @todo What conditions cause this case, and how likely are they? */
            case '?': {
                break;
            } break;

            /** @todo What conditions trigger this case, and how likely are they? */
            default: {
                printf("?? getopt returned character code 0%o ?? \n", c);
            } break;
        }
    }

    printf("Configured hostname: %s\n", hostname);
    printf("Configured server port: %s\n", port);
    
    printf("%s\n", "serverd starting...");

    struct addrinfo hints;
    memset(&hints, 0, sizeof (hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo* bind_address;
    getaddrinfo(NULL, port, &hints, &bind_address);
    
    int socket_listen = socket(bind_address->ai_family, bind_address->ai_socktype, bind_address->ai_protocol);

    if (socket_listen == -1) {
        fprintf(stderr, "[Error] %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    if (bind(socket_listen, bind_address->ai_addr, bind_address->ai_addrlen) == -1) {
        fprintf(stderr, "[Error] %s\n", strerror(errno));
        return EXIT_FAILURE;
    }
    
    // Free the server address info structure.
    freeaddrinfo(bind_address);

    /** @todo Set backlog parameter */
    if (listen(socket_listen, 10) == -1) {
        fprintf(stderr, "[Error] %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    int epfd = epoll_create(TRUE);

    if (epfd == -1) {
        fprintf(stderr, "[Error] %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = socket_listen;

    if (epoll_ctl(epfd, EPOLL_CTL_ADD, socket_listen, &ev) == -1) {
        fprintf(stderr, "[Error] %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    struct epoll_event events[EPOLL_MAX_EVENTS];

    while (TRUE) {
        int nfds = epoll_wait(epfd, events, EPOLL_MAX_EVENTS, -1);

        if (nfds == -1) {
            fprintf(stderr, "[Error] %s\n", strerror(errno));
            return EXIT_FAILURE;
        }

        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd == socket_listen) {
                struct sockaddr_storage client_address;
                socklen_t client_len = sizeof (client_address);

                int new_connection_socket = accept(socket_listen, (struct sockaddr *) &client_address, &client_len);

                if (new_connection_socket == -1) {
                    fprintf(stderr, "[Error] %s\n", strerror(errno));
                    return EXIT_FAILURE;
                }

                // setnonblocking(conn_sock)
                ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP | EPOLLOUT | EPOLLERR;
                ev.data.fd = new_connection_socket;

                if (epoll_ctl(epfd, EPOLL_CTL_ADD, new_connection_socket, &ev) == -1) {
                    fprintf(stderr, "[Error] %s\n", strerror(errno));
                    return EXIT_FAILURE;
                }

                char address_buffer[128];
                int get_name_info_error_code = getnameinfo((struct sockaddr *) &client_address, client_len, address_buffer, sizeof (address_buffer), NULL, 0, NI_NUMERICHOST);

                if (get_name_info_error_code) {
                    /** @todo Implement more robust error-handling */
                    fprintf(stderr, "[Error] %s\n", gai_strerror(get_name_info_error_code));
                    return EXIT_FAILURE;
                }

                /** Log the client request to the console */
                printf("New connection from %s\n", address_buffer);
            } else {
                if (events[i].events & EPOLLIN) {
                    
                    /** Allocate a stack buffer for the request data */
                    char request[1024] = { 0 };

                    /** Read the client request into the buffer */
                    ssize_t bytes_received = read(events[i].data.fd, request, 1024);

                    if (bytes_received == -1) {
                        /** @todo Actually handle this error */
                        fprintf(stderr, "[Error] %s\n", strerror(errno));
                        return EXIT_FAILURE;
                    }

                    /** Log the buffer to stdout for now */
                    printf("%s\n", request);

                    const char* response =
                        "HTTP/1.1 200 OK\r\n"
                        "Connection: Close\r\n"
                        "Content-Type: text/html\r\n"
                        "\r\n";

                    send(events[i].data.fd, response, strlen(response), 0);
                    int f = open("samples/site/index.html", O_RDONLY | O_NONBLOCK);
                    sendfile(events[i].data.fd, f, NULL, 311);
                    close(f);

                    // DEBUG: close socket
                    int current_socket = events[i].data.fd;
                    epoll_ctl(epfd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
                    close(current_socket);
                }
            }
        }
    }

    close(socket_listen);

    return EXIT_SUCCESS;
}
