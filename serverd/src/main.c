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

    while (TRUE) {
        int option_index = 0;
        int c = getopt_long(argc, argv, "hvH:p:", long_options, &option_index);

        if (c == -1) {
            break;
        }

        switch (c) {
            case 0: {
                if (strcmp(long_options[option_index].name, "version") == 0) {
                    /** @todo Configure project versioning info */
                    printf("Version Info\n");
                    return EXIT_SUCCESS;
                }
            } break;

            case 'p': {
                port = optarg;
            } break;

            case 'H': {
                hostname = optarg;
            } break;

            case 'h': {
                /** @todo Create help menu */
                printf("Help Menu\n");
                return EXIT_SUCCESS;
            } break;

            case '?': {
                break;
            } break;

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
                getnameinfo((struct sockaddr *) &client_address, client_len, address_buffer, sizeof (address_buffer), NULL, 0, NI_NUMERICHOST);
                printf("New connection from %s\n", address_buffer);
            } else {
                if (events[i].events & EPOLLIN) {
                    
                    /** Allocate a stack buffer for the request data */
                    char request[1024] = { 0 };

                    /** Read the client request into the buffer */
                    read(events[i].data.fd, request, 1024);

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
