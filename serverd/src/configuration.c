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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>

#include "serverd.h"
#include "configuration.h"
#include "error.h"
#include "memory.h"

/**
 * @def DEFAULT_CONFIGURATION_FILENAME
 * @brief The default configuration filename for the server.
 *
 * @details This filename and path corresponds to the sample
 * configuration file included in the server source directory.
 *
 */
#ifndef DEFAULT_CONFIGURATION_FILENAME
#define DEFAULT_CONFIGURATION_FILENAME "samples/conf/serverd.conf"
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
    { "help",                   no_argument,       0, 'h' },
    { "version",                no_argument,       0,  0  },
    { "configuration-filename", required_argument, 0, 'f' },
    { "hostname",               required_argument, 0, 'H' },
    { "port",                   required_argument, 0, 'p' },
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
    "  -f, --configuration-filename <str>    Path to alternative configuration file\n"
    "  -H, --hostname <str>                  Server hostname\n"
    "  -p, --port <int>                      Port number to bind to\n"
    "\n"
    "Generic Options:\n"
    "  -h, --help                            Display this help menu and exit\n"
    "      --version                         Display server version information\n"
    "\n";

/**
 * Allocate memory required by the configuration options.
 *
 * This function ensures that the memory allocation succeeded,
 * calling exit(3) if malloc returns NULL.
 *
 */
static struct configuration_options_t* allocate_configuration_options_object(void) {
    /**
     * Allocate the memory block for the configuration
     * options object and immediately return it.
     *
     * Due to some nifty refactoring, this function is
     * simply a convenience method to make the allocation of
     * the configuration options object invisible to the
     * calling function within the module.
     *
     * Previously, this function contained error-checking
     * code designed to verify that the call to malloc
     * succeeded. This code is no longer necessary, as the
     * allocate_memory function is the public interface to
     * the memory module, which now handles all of that
     * error-checking and bookkeeping behind the scenes.
     *
     */
    return allocate_memory(sizeof (struct configuration_options_t));
}

/**
 * Initialize the default configuration options.
 *
 * This function takes care of allocating the memory required
 * by the configuration options object by calling the requisite
 * allocation function, and then taking the newly-allocated
 * configuration options object and initializing all of its
 * properties to their respective defaults.
 *
 * This function returns a fully-initialized configuration
 * options object.
 *
 */
static struct configuration_options_t* initialize_default_configuration(void) {
    /**
     * Call the function responsible for ensuring the
     * configuration options object is allocated successfully.
     *
     */
    struct configuration_options_t* configuration_options = allocate_configuration_options_object();

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
    configuration_options->configuration_filename = DEFAULT_CONFIGURATION_FILENAME;

    /**
     * @brief The hostname the server will use.
     *
     * @todo The user should be able to configure the
     * hostname for the server on a per-virtual host basis.
     * 
     */
    configuration_options->hostname = DEFAULT_HOSTNAME;

    /**
     * @brief The port the server will use to listen for incoming
     * client connections on.
     * 
     */
    configuration_options->port = DEFAULT_PORT;
    
    /**
     * Return the initialized configuration options object.
     *
     */
    return configuration_options;
}

/**
 * Set configuration options from command-line arguments.
 *
 * This function handles the setting of server configuration
 * options by calling the getopt(3) and getopt_long(3) standard
 * library functions.
 *
 * This routine must be called before any configuration file
 * parsing, as it would otherwise not be possible to specify
 * a configuration file path from the command-line.
 *
 */
static void parse_command_line_configuration_options(struct configuration_options_t* configuration_options, int argc, char *argv[]) {
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
                    exit(EXIT_SUCCESS);
                }

                /** @todo What happens if we've gotten to this point? */
            } break;

            case 'f': {
                configuration_options->configuration_filename = optarg;
            } break;

            /**
             * Configure Server Hostname
             */
            case 'H': {
                configuration_options->hostname = optarg;
            } break;

            /**
             * Display Help Menu
             */
            case 'h': {
                printf("%s", help_menu);
                exit(EXIT_SUCCESS);
            } break;

            /**
             * Configure Server Port
             */
            case 'p': {
                configuration_options->port = optarg;
            } break;

            /** @todo What conditions cause this case, and how likely are they? */
            case '?': {
                break;
            } break;

            /** @todo What conditions trigger this case, and how likely are they? */
            default: {
                fatal_error("?? getopt returned character code 0%o ?? \n", c);
            } break;
        }
    }
}

/**
 * Parse server configuration file
 *
 * This function handles the setting of server configuration
 * options by parsing the configuration file specified
 * either via the command-line or using the default value
 * set in the DEFAULT_CONFIGURATION_FILENAME directive.
 *
 * @note The function allows for the bypassing of config
 * file parsing altogether by specifying a value of NULL for
 * the configuration_filename parameter.
 *
 */
__attribute__((nonnull(1)))
static void parse_configuration_file_options(struct configuration_options_t* configuration_options) {
    /**
     * If we have configuration filename argument, we simply
     * short-circuit execution (and prevent a segmentation
     * fault) by simply returning right away.
     *
     */
    if (configuration_options->configuration_filename == NULL) {
        /**
         * There is no value to return, as this function has
         * a return type of void, so an empty return statement
         * is good enough.
         *
         */
        return;
    }

    /**
     * Open the configuration file.
     *
     */
    FILE* configuration_file = fopen(configuration_options->configuration_filename, "r");

    /**
     * Verify that we were actually able to open the
     * configuration file.
     *
     * The call to fopen(3) could have failed for a number
     * of reasons, including the file not existing, the path
     * being incorrect, or the server not having the
     * permission required to read the file.
     *
     */
    if (configuration_file == NULL) {
        /**
         * If we are not able to read the configuration file,
         * this represents a fatal error. No further
         * execution should be carried out.
         *
         */
        fatal_error("[Error] %s: %s (%s)\n", "Could not open configuration file", configuration_options->configuration_filename, strerror(errno));
    }

    /**
     * Define the size of the line buffer to use while
     * parsing the configuration file.
     *
     * Honestly, 512 bytes is ridiculously long for what we
     * need, but it's the block size on solid-state drives,
     * so it's probably the smallest we can make without
     * impacting performance too much.
     *
     * @todo It may be well-worth our time to determine
     * whether reading into a much larger input buffer is
     * significantly faster.
     *
     */
    size_t buffer_size = 512;
    
    /**
     * Heap-allocate the line buffer.
     *
     * The getline(3) function that we are using specifically
     * requires that the line buffer argument to be
     * heap-allocated.
     *
     */
    char* line_buffer = allocate_memory(sizeof(char) * buffer_size);

    /**
     * Iterate over the configuration file stream, reading
     * each line into the line buffer.
     *
     */
    while ((getline(&line_buffer, &buffer_size, configuration_file)) > 0) {
        /**
         * In other to use a more simplistic, hand-made
         * configuration file parser instead of something
         * more complicated, like PCRE, we use a basic state
         * machine to eliminate all of the comments in the
         * current line.
         *
         * Once we detect a comment, we begin to "zero out"
         * the line content after that point. We're not
         * actually zeroing anything out, though, we're
         * simply replacing the character at each location
         * after a comment is found with a space, which then
         * strtok(3) skips over once we start tokenizing the
         * input line.
         *
         */
        int comment = FALSE;

        /**
         * Iterate over each character of the input line.
         *
         */
        for (size_t i = 0; i < strlen(line_buffer); ++i) {
            /**
             * If we have already detected a comment on this
             * line, replace the current character with a
             * space.
             *
             */
            if (comment) {
                line_buffer[i] = ' ';
            }

            /**
             * If we find a '#' symbol, interpret it as the
             * start of a comment.
             *
             * @note Since we are explicitly setting comment
             * to TRUE, rather than negating it (!comment),
             * this has the effect of making the comment
             * state-transition idempotent, so we don't have
             * to worry about multiple pound signs per line
             * switching us in and out of the comment mode.
             *
             */
            if (line_buffer[i] == '#') {
                comment = TRUE;
            }
        }

        /**
         * Tokenize the input line.
         *
         * Having cleared the input line of all comments, we
         * can begin the tokenization process by splitting
         * the input line based on equal signs, spaces, or
         * new line characters.
         *
         * The configuration file parser expects configuration
         * options to be defined as follows:
         *
         *  Option=Value
         *
         * The equivalent regular expression for parsing this
         * grammar looks like this:
         *
         *  ^([A-Za-z]+)\s*=(.*)$
         *
         * While we are not actually using regular expressions
         * at the moment, it may eventually become necessary,
         * depending on the complexity of the configuration
         * language we deem necessary.
         *
         */
        char* option = strtok(line_buffer, " =#\r\n");

        /**
         * It's possible that there are blank lines in the
         * configuration file, both before and after we've
         * gone through and cleared all the comments, so
         * if strtok(3) doesn't find anything on this line,
         * just move on.
         *
         */
        if ((option != NULL) && (strcmp(option, "") != 0)) {
            /** Get value */
            char* value = strtok(NULL, "\r\n");

            /**
             * Since we did find an option token on this
             * line, a value token is necessary.
             *
             */
            if ((value == NULL) || (strcmp(value,"") == 0)) {
                /**
                 * If no value is found for the current
                 * configuration option, we need to exit
                 * right away with an error message indicating
                 * the problem.
                 *
                 */
                fatal_error("[Error] Invalid configuration setting for option: %s\n", option);
            }

            char* value_string = allocate_memory(strlen(value));
            strcpy(value_string, value);

            /** @todo Validate configuration options */
            if (strcmp(option, "hostname") == 0) {
                configuration_options->hostname = value_string;
            } else if (strcmp(option, "port") == 0) {
                configuration_options->port = value_string;
            } else if (strcmp(option, "docroot") == 0) {
                configuration_options->document_root_directory = value_string;
            } else {
                free(value_string);
                fatal_error("[Error] %s: %s\n", "Unrecognized option", option);
            }
        }
    }

    /**
     * Free the line buffer memory.
     *
     */
    free(line_buffer);

    /**
     * Close the configuration file stream.
     *
     */
    fclose(configuration_file);
}

/**
 * This function is the public interface for the configuration
 * module, which handles all of the necessary configuration
 * parsing required to configure the server.
 *
 */
struct configuration_options_t* initialize_server_configuration(int argc, char *argv[]) {
    /**
     * Initialize the server configuration options by setting
     * all options to their defined defaults.
     *
     */
    struct configuration_options_t* configuration_options = initialize_default_configuration();

    /**
     * Begin by getting all user options passed in from the
     * command-line, if any.
     *
     * This must be done prior to parsing the configuration
     * file because there will be command-line options to
     * specify using a configuration file specified via the
     * command-line, as well as the option to not use any
     * configuration file and simply run right from the
     * defaults.
     *
     */
    parse_command_line_configuration_options(configuration_options, argc, argv);

    /**
     * Parse server configuration file.
     *
     * Once the previous step of parsing the command-line
     * for user options has been completed, we continue by
     * reading the configuration file to continue the
     * configuration process.
     *
     */
    parse_configuration_file_options(configuration_options);

    /**
     * Return the configured server options.
     *
     */
    return configuration_options;
}
