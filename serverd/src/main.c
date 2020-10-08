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

#include "error.h"
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
 * Functions that handle socket initialization, binding, and
 * other related tasks return an integer that represents a
 * file descriptor. Since it makes no sense to allow the
 * same operations on file descriptors as you would on any
 * arbitrary integers (for instance, what does it mean to
 * add two file descriptors?), we declare a typedef to more
 * accurately reflect our intent to represent a socket file
 * descriptor specifically.
 *
 */
typedef int socket_t;

socket_t socket_(int domain, int type, int protocol) {
    socket_t new_socket = socket(domain, type, protocol);

    if (new_socket == -1) {
        fatal_error("[Error] %s\n", strerror(errno));
    }

    return new_socket;
}

#ifndef socket
#define socket socket_
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
        fatal_error("[Error] %s\n", strerror(errno));
    }

    if (bind(socket_listen, bind_address->ai_addr, bind_address->ai_addrlen) == -1) {
        fatal_error("[Error] %s\n", strerror(errno));
    }
    
    // Free the server address info structure.
    freeaddrinfo(bind_address);

    /** @todo Set backlog parameter */
    if (listen(socket_listen, 10) == -1) {
        fatal_error("[Error] %s\n", strerror(errno));
    }

    int epfd = epoll_create(TRUE);

    if (epfd == -1) {
        fatal_error("[Error] %s\n", strerror(errno));
    }

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = socket_listen;

    if (epoll_ctl(epfd, EPOLL_CTL_ADD, socket_listen, &ev) == -1) {
        fatal_error("[Error] %s\n", strerror(errno));
    }

    struct epoll_event events[EPOLL_MAX_EVENTS];

    while (TRUE) {
        int nfds = epoll_wait(epfd, events, EPOLL_MAX_EVENTS, -1);

        if (nfds == -1) {
            fatal_error("[Error] %s\n", strerror(errno));
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
                    fprintf(stderr, "[Error] %s\n", strerror(errno));
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
                        fatal_error("[Error] %s\n", strerror(errno));
                    }

                    /** Log the buffer to stdout for now */
                    printf("%s\n", request);

                    /**
                     * Stack-allocate a buffer to preserve
                     * the contents of the original request.
                     *
                     */
                    char original_request[1024] = { 0 };

                    /**
                     * If the source string is less than n
                     * bytes long, strncpy(3) pads the destination
                     * string with NUL bytes to ensure that
                     * n bytes are always written. It might
                     * be a good idea to either use a macro
                     * to specify a size of the minimum
                     * between n or strlen(request), or just
                     * not bother zeroing the original_request
                     * buffer when stack-allocating it.
                     *
                     */
                    strncpy(original_request, request, 1024);

                    /**
                     * Begin tokenizing the HTTP request
                     * received from the client.
                     *
                     */
                    char* request_method = strtok(request, " \r\n");

                    if (request_method == NULL) {
                        /** @todo This error needs to actually be handled. */
                        fatal_error(stderr, "[Error] %s\n", "Invalid request method.");
                    }

                    char* request_uri = strtok(NULL, " \r\n");

                    if (request_uri == NULL) {
                        /** @todo This also needs to actually be handled */
                        fatal_error(stderr, "[Error] %s\n", "No request URI found.");
                    }

                    char* request_version = strtok(NULL, " \r\n");

                    if (request_version == NULL) {
                        /** @todo Probably send an invalid request error */
                        fatal_error(stderr, "[Error] %s\n", "Invalid request version.");
                    }

                    const char* response =
                        "HTTP/1.1 200 OK\r\n"
                        "Connection: Close\r\n"
                        "Content-Type: text/html\r\n"
                        "\r\n";

                    send(events[i].data.fd, response, strlen(response), 0);
                    int f = open("samples/site/index.html", O_RDONLY | O_NONBLOCK);
                    sendfile(events[i].data.fd, f, NULL, 311);
                    close(f);

                    /**
                     * At the moment, the server listens for
                     * incoming connections, accepts them,
                     * and returns a 200 OK status with the
                     * default page regardless of the HTTP
                     * request type or options. For this
                     * reason, we are simply closing all
                     * client connections after the initial
                     * response is sent, while including the
                     * appropriate 'Connection: Close' HTTP
                     * header in the response, as well.
                     *
                     */
                    int current_socket = events[i].data.fd;
                    epoll_ctl(epfd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
                    close(current_socket);
                }
            }
        }
    }

    /**
     * Close the server's incoming connection listener
     * socket.
     *
     * At the moment, there is no way to get to this point,
     * as the server simply continues listening for incoming
     * connections indefinitely. However, this is included
     * for good practice, if for no other reason.
     *
     */
    close(socket_listen);

    return EXIT_SUCCESS;
}
