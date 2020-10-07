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
enum { FALSE = 0, TRUE = !FALSE };
#endif

#ifndef EPOLL_MAX_EVENTS
#define EPOLL_MAX_EVENTS (10)
#endif

#ifndef DEFAULT_HOSTNAME
#define DEFAULT_HOSTNAME "localhost"
#endif

#ifndef DEFAULT_PORT
#define DEFAULT_PORT "8080"
#endif

int main(int argc, char *argv[])
{
    int verbose = FALSE;
    const char* hostname = DEFAULT_HOSTNAME;
    const char* port = DEFAULT_PORT;

    static struct option long_options[] = {
        { "help", no_argument, 0, 'h' },
        { "version", no_argument, 0, 0 },
        { "verbose", no_argument, 0, 'v' },
        { "hostname", required_argument, 0, 'H' },
        { "port", required_argument, 0, 'p' },
        { 0, 0, 0, 0 }
    };

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

            case 'v': {
                printf("[Info] %s\n", "Verbose output enabled.");
                verbose = TRUE;
            } break;

            case '?': {
                break;
            } break;

            default: {
                printf("?? getopt returned character code 0%o ?? \n", c);
            } break;
        }
    }

    if (verbose) {
        printf("Configured hostname: %s\n", hostname);
        printf("Configured server port: %s\n", port);
    }
    
    printf("%s\n", "serverd starting...");

    struct addrinfo hints;
    memset(&hints, 0, sizeof (hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo* bind_address;
    getaddrinfo(0, port, &hints, &bind_address);
    
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
                getnameinfo((struct sockaddr *) &client_address, client_len, address_buffer, sizeof (address_buffer), 0, 0, NI_NUMERICHOST);
                printf("New connection from %s\n", address_buffer);
            } else {
                if (events[i].events & EPOLLIN) {
                    char request[1024];
                    read(events[i].data.fd, request, 1024);
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
