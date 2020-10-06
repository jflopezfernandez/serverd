/**
 * serverd - Modern web server daemon
 * Copyright (C) 2020 Jose Fernando Lopez Fernandez
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
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

#include <netdb.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <netinet/in.h>

int main(void)
{
    printf("%s\n", "serverd starting...");

    struct addrinfo hints;
    memset(&hints, 0, sizeof (hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo* bind_address;
    getaddrinfo(0, "8081", &hints, &bind_address);
    
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

    struct sockaddr_storage client_address;
    socklen_t client_len = sizeof (client_address);

    int socket_client = accept(socket_listen, (struct sockaddr *) &client_address, &client_len);

    if (socket_client == -1) {
        fprintf(stderr, "[Error] %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    char address_buffer[100];
    getnameinfo((struct sockaddr *) &client_address, client_len, address_buffer, sizeof (address_buffer), 0, 0, NI_NUMERICHOST);
    printf("New connection from %s\n", address_buffer);

    char request[1024];
    ssize_t bytes_received = recv(socket_client, request, 1024, 0);
    printf("%s\n", request);

    const char* response =
        "HTTP/1.1 200 OK\r\n"
        "Connection: Close\r\n"
        "Content-Type: text/html\r\n"
        "\r\n"
        "<!DOCTYPE html>\n"
        "<html lang='en-US'>\n"
        "<head>\n"
        "    <meta charset='UTF-8'>\n"
        "    <meta name='viewport' content='width=device-width, initial-scale=1.0, shrink-to-fit=no'>\n"
        "\n"
        "    <title>serverd</title>\n"
        "\n"
        "    <!-- Stylesheets -->\n"
        "</head>\n"
        "<body>\n"
        "    <main role='main'>\n"
        "        <p>serverd home</p>\n"
        "    </main>\n"
        "</body>\n"
        "</html>\n";
    
    ssize_t bytes_sent = send(socket_client, response, strlen(response), 0);

    close(socket_client);
    close(socket_listen);

    return EXIT_SUCCESS;
}
