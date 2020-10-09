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
#include <signal.h>
#include <errno.h>
#include <time.h>

#include <unistd.h>

#include <sys/epoll.h>
#include <sys/resource.h>
#include <sys/select.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <fcntl.h>
#include <getopt.h>
#include <netdb.h>
#include <syslog.h>

#include <arpa/inet.h>

#include <netinet/in.h>
#include <netinet/tcp.h>

#include "serverd.h"
#include "configuration.h"
#include "error.h"
#include "memory.h"

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
#else
    #error "socket() macro already defined"
#endif

void bind_(socket_t socket, const struct sockaddr* socket_address, socklen_t address_length) {
    int result = bind(socket, socket_address, address_length);

    if (result == -1) {
        fatal_error("[Error] %s\n", strerror(errno));
    }
}

#ifndef bind
    #define bind bind_
#else
    #error "bind() macro already defined"
#endif

void listen_(socket_t socket, size_t backlog) {
    /**
     * If the number of conections to backlog is larger than
     * the value in /proc/sys/net/core/somaxconn, then it is
     * silently truncated to that value.
     *
     * The default value is 4096.
     *
     */
    if (listen(socket, backlog) == -1) {
        fatal_error("[Error] %s\n", strerror(errno));
    }
}

#ifndef listen
    #define listen listen_
#else
    #error "listen() macro already defined"
#endif

socket_t initialize_listener_socket(const char* hostname, const char* port) {
    struct addrinfo hints;
    memset(&hints, 0, sizeof (hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo* bind_address;
    getaddrinfo(hostname, port, &hints, &bind_address);
    
    socket_t listener_socket = socket(bind_address->ai_family, bind_address->ai_socktype, bind_address->ai_protocol);
    bind(listener_socket, bind_address->ai_addr, bind_address->ai_addrlen);
    
    // Free the server address info structure.
    freeaddrinfo(bind_address);

    /** @todo Set backlog parameter */
    listen(listener_socket, SOMAXCONN);

    return listener_socket;
}

/**
 * HTTP request methods as specified in RFC 7231
 *
 * See: https://tools.ietf.org/html/rfc7231#section-4
 *
 */
enum request_method_t {
    REQUEST_METHOD_UNKNOWN,
    REQUEST_METHOD_GET,
    REQUEST_METHOD_POST,
    REQUEST_METHOD_HEAD,
    REQUEST_METHOD_PUT,
    REQUEST_METHOD_DELETE,
    REQUEST_METHOD_CONNECT,
    REQUEST_METHOD_OPTIONS,
    REQUEST_METHOD_TRACE
};

struct http_request_t {
    enum request_method_t request_method;
    const char* request_uri;
};

enum http_status_code_t {
    HTTP_STATUS_CODE_NULL = 0,
    HTTP_STATUS_CODE_CONTINUE = 100,
    HTTP_STATUS_CODE_SWITCHING_PROTOCOL = 101,
    HTTP_STATUS_CODE_PROCESSING = 102,
    HTTP_STATUS_CODE_EARLY_HINTS = 103.
    HTTP_STATUS_CODE_OK = 200,
    HTTP_STATUS_CODE_CREATED = 201,
    HTTP_STATUS_CODE_ACCEPTED = 202,
    HTTP_STATUS_CODE_NON_AUTHORITATIVE_INFORMATION = 203,
    HTTP_STATUS_CODE_NO_CONTENT = 204,
    HTTP_STATUS_CODE_RESET_CONTENT = 205,
    HTTP_STATUS_CODE_PARTIAL_CONTENT = 206,
    HTTP_STATUS_CODE_MULTIPLE_CHOICE = 300,
    HTTP_STATUS_CODE_MOVED_PERMANENTLY = 301,
    HTTP_STATUS_CODE_FOUND = 302,
    HTTP_STATUS_CODE_NOT_MODIFIED = 304,
    HTTP_STATUS_CODE_TEMPORARY_REDIRECT = 307,
    HTTP_STATUS_CODE_PERMANENT_REDIRECT = 308,
    HTTP_STATUS_CODE_BAD_REQUEST = 400,
    HTTP_STATUS_CODE_UNAUTHORIZED = 401,
    HTTP_STATUS_CODE_FORBIDDEN = 403,
    HTTP_STATUS_CODE_NOT_FOUND = 404,
    HTTP_STATUS_CODE_METHOD_NOT_ALLOWED = 405,
    HTTP_STATUS_CODE_NOT_ACCEPTABLE = 406,
    HTTP_STATUS_CODE_PROXY_AUTHENTICATION_REQUIRED = 407,
    HTTP_STATUS_CODE_REQUEST_TIMEOUT = 408,
    HTTP_STATUS_CODE_CONFLICT = 409,
    HTTP_STATUS_CODE_GONE = 410,
    HTTP_STATUS_CODE_LENGTH_REQUIRED = 411,
    HTTP_STATUS_CODE_PRECONDITION_FAILED = 412,
    HTTP_STATUS_CODE_PAYLOAD_TOO_LARGE = 413,
    HTTP_STATUS_CODE_URI_TOO_LONG = 414,
    HTTP_STATUS_CODE_UNSUPPORTED_MEDIA_TYPE = 415,
    HTTP_STATUS_CODE_EXPECTATION_FAILED = 417,
    HTTP_STATUS_CODE_IM_A_LITTLE_TEAPOT = 418,
    HTTP_STATUS_CODE_UPGRADE_REQUIRED = 426,
    HTTP_STATUS_CODE_PRECONDITION_REQUIRED = 428,
    HTTP_STATUS_CODE_TOO_MANY_REQUESTS = 429,
    HTTP_STATUS_CODE_REQUEST_HEADER_FIELDS_TOO_LARGE = 431,
    HTTP_STATUS_CODE_UNAVAILABLE_FOR_LEGAL_REASONS = 451,
    HTTP_STATUS_CODE_INTERNAL_SERVER_ERROR = 500,
    HTTP_STATUS_CODE_NOT_IMPLEMENTED = 501,
    HTTP_STATUS_CODE_BAD_GATEWAY = 502,
    HTTP_STATUS_CODE_SERVICE_UNAVAILABLE = 503,
    HTTP_STATUS_CODE_GATEWAY_TIMEOUT = 504,
    HTTP_STATUS_CODE_HTTP_VERSION_NOT_SUPPORTED = 505,
    HTTP_STATUS_CODE_VARIANT_ALSO_NEGOTIATES = 506,
    HTTP_STATUS_CODE_NOT_EXTENDED = 510,
    HTTP_STATUS_CODE_NETWORK_AUTHENTICATION_REQUIRED = 511
};

struct uri_t {
    char* protocol;
    char* hostname;
    char* port_num;
    char* doc_path;
};

struct uri_t* parse_uri(char* uri_string) {
    char* uri_buffer = malloc(strlen(uri_string));
    strcpy(uri_buffer, uri_string);

    struct uri_t* uri = malloc(sizeof(struct uri_t));

    char* p = strstr(uri_buffer, "://");
    

    return uri;
}

void free_uri(struct uri_t* uri) {
    free(uri->protocol);
    free(uri->hostname);
    free(uri->port_num);
    free(uri->doc_path);
    free(uri);
}

void print_uri(struct uri_t* uri) {
    printf("URL: %s://%s%s%s\n", uri->protocol, uri->hostname, uri->portnum, uri->doc_path);
    printf("  - Protocol: %s\n", uri->protocol);
    printf("  - Hostname: %s\n", uri->hostname);
    printf("  - Port Num: %s\n", uri->port_num);
    printf("  - Doc Path: %s\n", uri->doc_path);
    printf("\n");
}

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
    if (argc == 1) {
        fprintf(stderr, "No input(s).\n");
        return EXIT_FAILURE;
    }

    while (*++argv) {
        printf("%s\n", *argv);
        struct uri_t* uri = parse_uri(*argv);
        print_uri(uri);
        free_uri(uri);
    }

    return EXIT_SUCCESS;

    /**
     * Initialize the configuration options container.
     *
     * The configuration module handles all of the command-
     * line and configuration file parsing, so once we call
     * this function, the server is ready to rock and roll.
     *
     */
    struct configuration_options_t* configuration_options = initialize_server_configuration(argc, argv);

    /**
     * Call umask to set the file mode creation mask to a
     * known mode.
     *
     */
    umask(007);

    /**
     * In order for the server process to daemonize, we need
     * to first call fork.
     *
     */
    pid_t pid;

    if ((pid = fork()) == -1) {
        /**
         * If something went wrong during the call to fork(2),
         * log, this represents a fatal error and we cannot
         * proceed with the server initialization procedure.
         *
         */
        fatal_error("[Error] %s\n", strerror(errno));
    } else if (pid != 0) {
        /**
         * If nothing went wrong with the call to fork(2),
         * the parent thread we started from the shell will
         * exit. The child process will continue with no
         * controlling terminal.
         *
         */
        return EXIT_SUCCESS;
    }

    /**
     * Having previously called fork(2) and terminated the
     * parent process, the current process is not a process
     * group leader, and therefore the call to setsid(2)
     * creates a new session.
     *
     */
    setsid();

    struct sigaction signal_action;
    signal_action.sa_handler = SIG_IGN;
    sigemptyset(&signal_action.sa_mask);
    signal_action.sa_flags = 0;

    if (sigaction(SIGHUP, &signal_action, NULL) == -1) {
        fatal_error("[Error] %s\n", strerror(errno));
    }

    // if ((pid = fork()) == -1) {
    //     fatal_error("[Error] %s\n", strerror(errno));
    // } else if (pid != 0) {
    //     return EXIT_SUCCESS;
    // }

    printf("%u\n", getpid());

    // if (chdir("/") == -1) {
    //    fatal_error("[Error] %s\n", strerror(errno));
    // }

    struct rlimit resource_limit;
    getrlimit(RLIMIT_NOFILE, &resource_limit);

    if (resource_limit.rlim_max == RLIM_INFINITY) {
        resource_limit.rlim_max = 1024;
    }

    for (rlim_t i = 0; i < resource_limit.rlim_max; ++i) {
        close(i);
    }

    openlog("serverd", LOG_CONS, LOG_DAEMON);

    // printf("Configuration filename: %s\n", configuration_options->configuration_filename);
    // printf("Hostname: %s\n", configuration_options->hostname);
    // printf("Sever port: %s\n", configuration_options->port);
    
    // printf("%s\n", "serverd starting...");

    socket_t socket_listen = initialize_listener_socket(configuration_options->hostname, configuration_options->port);

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

    syslog(LOG_NOTICE, "Listening for new connections on port %s...", configuration_options->port);

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
                    fatal_error("[Error] %s\n", strerror(errno));
                }

                // setnonblocking(conn_sock)
                ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP | EPOLLOUT | EPOLLERR;
                ev.data.fd = new_connection_socket;

                if (epoll_ctl(epfd, EPOLL_CTL_ADD, new_connection_socket, &ev) == -1) {
                    fatal_error("[Error] %s\n", strerror(errno));
                }

                char address_buffer[128];
                int get_name_info_error_code = getnameinfo((struct sockaddr *) &client_address, client_len, address_buffer, sizeof (address_buffer), NULL, 0, NI_NUMERICHOST);

                if (get_name_info_error_code) {
                    /** @todo Implement more robust error-handling */
                    fatal_error("[Error] %s\n", strerror(errno));
                }

                /** Log the client request to the console */
                //printf("New connection from %s\n", address_buffer);

                /** Log the new connection request */
                syslog(LOG_INFO, "New connection from %s...", address_buffer);
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
                    //printf("%s\n", request);

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
                        fatal_error("[Error] %s\n", "Invalid request method.");
                    }

                    char* request_uri = strtok(NULL, " \r\n");

                    if (request_uri == NULL) {
                        /** @todo This also needs to actually be handled */
                        fatal_error("[Error] %s\n", "No request URI found.");
                    }

                    char* request_version = strtok(NULL, " \r\n");

                    if (request_version == NULL) {
                        /** @todo Probably send an invalid request error */
                        fatal_error("[Error] %s\n", "Invalid request version.");
                    }

                    const char* response =
                        "HTTP/1.1 200 OK\r\n"
                        "Connection: Close\r\n"
                        "Content-Type: text/html\r\n"
                        "\r\n";
                    
                    char filename_buffer[1024] = { 0 };
                    snprintf(filename_buffer, 1024, "%s%s", configuration_options->document_root_directory, "index.html");
                    syslog(LOG_DEBUG, "Filename buffer: %s", filename_buffer);


                    send(events[i].data.fd, response, strlen(response), 0);
                    //int f = open(filename_buffer, O_RDONLY | O_NONBLOCK);
                    //int f = open("samples/site/index.html", O_RDONLY | O_NONBLOCK);

                    //const char* filename = "/home/jflopezfernandez/projects/c/serverd/serverd/samples/site/index.html";

                    int f = open(filename_buffer, O_RDONLY | O_NONBLOCK);

                    if (f == -1) {
                        syslog(LOG_ERR, "[Error] Could not open file: %s (%s)", filename_buffer, strerror(errno));
                        return EXIT_FAILURE;
                    }

                    struct stat status;

                    if (fstat(f, &status) == -1) {
                        syslog(LOG_ERR, "%s: %s", "Could not check file status", strerror(errno));
                        return EXIT_FAILURE;
                    }

                    ssize_t bytes_sent = sendfile(events[i].data.fd, f, NULL, status.st_size);

                    if (bytes_sent == -1) {
                        syslog(LOG_ERR, "[Error] sendfile(2) failed: %s", strerror(errno));
                        return EXIT_FAILURE;
                    }

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
