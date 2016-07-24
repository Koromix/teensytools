/*
 * ty, a collection of GUI and command-line tools to manage Teensy devices
 *
 * Distributed under the MIT license (see LICENSE.txt or http://opensource.org/licenses/MIT)
 * Copyright (c) 2015 Niels Martignène <niels.martignene@gmail.com>
 */

#include <stdarg.h>
#include "main.h"

enum output_format {
    OUTPUT_PLAIN,
    OUTPUT_JSON
};

enum collection_type {
    COLLECTION_LIST = '[',
    COLLECTION_OBJECT = '{'
};

static enum output_format output = OUTPUT_PLAIN;
static bool verbose = false;
static bool watch = false;

static enum collection_type collections[8];
static unsigned int collection_depth;
static bool collection_started;

static void print_list_usage(FILE *f)
{
    fprintf(f, "usage: %s list [options]\n\n", executable_name);

    print_common_options(f);
    fprintf(f, "\n");

    fprintf(f, "List options:\n"
               "   -O, --output <format>    Output format, must be plain (default) or json\n"
               "   -v, --verbose            Print detailed information about devices\n\n"
               "   -w, --watch              Watch devices dynamically\n");
}

TY_PRINTF_FORMAT(2, 3)
static void print_field(const char *key, const char *format, ...)
{
    char value[256];
    bool numeric;

    numeric = false;
    if (format) {
        va_list ap;
        int dummy;
        char dummy2;

        va_start(ap, format);
        vsnprintf(value, sizeof(value), format, ap);
        va_end(ap);

        if (sscanf(value, "%d%c", &dummy, &dummy2) == 1)
            numeric = true;
    } else {
        value[0] = 0;
    }

    switch (output) {
    case OUTPUT_PLAIN:
        if (key || format)
            printf("\n%*s%c ", collection_depth * 2, "", collection_depth % 2 ? '+' : '-');
        if (key)
            printf("%s: ", key);
        printf("%s", value);
        break;

    case OUTPUT_JSON:
        if (collection_started)
            printf(", ");
        if (collection_depth && collections[collection_depth - 1] == COLLECTION_LIST && key && format) {
            if (numeric) {
                printf("[\"%s\", %s]", key, value);
            } else {
                printf("[\"%s\", \"%s\"]", key, value);
            }
        } else {
            if (key)
                printf("\"%s\": ", key);
            if (numeric) {
                printf("%s", value);
            } else if (format) {
                printf("\"%s\"", value);
            }
        }
        break;
    }

    collection_started = true;
}

static void start_collection(const char *key, enum collection_type type)
{
    print_field(key, NULL);
    if (output == OUTPUT_JSON)
        printf("%c", type);

    assert(collection_depth < TY_COUNTOF(collections));
    collections[collection_depth++] = type;

    collection_started = false;
}

static void end_collection(void)
{
    assert(collection_depth);
    collection_depth--;

    switch (output) {
    case OUTPUT_PLAIN:
        if (!collection_started && collections[collection_depth] == COLLECTION_LIST)
            printf("(none)");
        break;
    case OUTPUT_JSON:
        printf("%c", collections[collection_depth] + 2);
        break;
    }

    collection_started = !!collection_depth;
}

static int print_interface_info(ty_board_interface *iface, void *udata)
{
    TY_UNUSED(udata);

    print_field(ty_board_interface_get_name(iface), "%s", ty_board_interface_get_path(iface));

    return 0;
}

static int list_callback(ty_board *board, ty_monitor_event event, void *udata)
{
    TY_UNUSED(event);
    TY_UNUSED(udata);

    const ty_board_model *model = ty_board_get_model(board);
    const char *action = "";

    switch (event) {
    case TY_MONITOR_EVENT_ADDED:
        action = "add";
        break;
    case TY_MONITOR_EVENT_CHANGED:
        action = "change";
        break;
    case TY_MONITOR_EVENT_DISAPPEARED:
        action = "miss";
        break;
    case TY_MONITOR_EVENT_DROPPED:
        action = "remove";
        break;
    }

    start_collection(NULL, COLLECTION_OBJECT);

    if (output == OUTPUT_PLAIN) {
        printf("%s %s %s", action, ty_board_get_tag(board),
               model ? ty_board_model_get_name(model) : "(unknown)");
        if (ty_board_get_description(board))
            printf(" (%s)", ty_board_get_description(board));
    } else {
        print_field("action", "%s", action);
        print_field("tag", "%s", ty_board_get_tag(board));
        print_field("serial", "%"PRIu64, ty_board_get_serial_number(board));
        if (ty_board_get_description(board))
            print_field("description", "%s", ty_board_get_description(board));
        print_field("model", "%s", ty_board_model_get_name(model));
    }

    if (verbose && ((event != TY_MONITOR_EVENT_DROPPED && event != TY_MONITOR_EVENT_DISAPPEARED) || output != OUTPUT_PLAIN)) {
        print_field("location", "%s", ty_board_get_location(board));

        int capabilities = ty_board_get_capabilities(board);

        start_collection("capabilities", COLLECTION_LIST);
        for (unsigned int i = 0; i < TY_BOARD_CAPABILITY_COUNT; i++) {
            if (capabilities & (1 << i))
                print_field(NULL, "%s", ty_board_capability_get_name(i));
        }
        end_collection();

        start_collection("interfaces", COLLECTION_LIST);
        ty_board_list_interfaces(board, print_interface_info, NULL);
        end_collection();
    }

    end_collection();
    printf("\n");
    fflush(stdout);

    return 0;
}

int list(int argc, char *argv[])
{
    ty_optline_context optl;
    char *opt;
    ty_monitor *monitor;
    int r;

    ty_optline_init_argv(&optl, argc, argv);
    while ((opt = ty_optline_next_option(&optl))) {
        if (strcmp(opt, "--help") == 0) {
            print_list_usage(stdout);
            return EXIT_SUCCESS;
        } else if (strcmp(opt, "--output") == 0 || strcmp(opt, "-O") == 0) {
            char *value = ty_optline_get_value(&optl);
            if (!value) {
                ty_log(TY_LOG_ERROR, "Option '--output' takes an argument");
                print_list_usage(stderr);
                return EXIT_FAILURE;
            }

            if (strcmp(value, "plain") == 0) {
                output = OUTPUT_PLAIN;
            } else if (strcmp(value, "json") == 0) {
                output = OUTPUT_JSON;
            } else {
                ty_log(TY_LOG_ERROR, "--output must be one off plain or json");
                print_list_usage(stderr);
                return EXIT_FAILURE;
            }
        } else if (strcmp(opt, "--verbose") == 0 || strcmp(opt, "-v") == 0) {
            verbose = true;
        } else if (strcmp(opt, "--watch") == 0 || strcmp(opt, "-w") == 0) {
            watch = true;
        } else if (!parse_common_option(&optl, opt)) {
            print_list_usage(stderr);
            return EXIT_FAILURE;
        }
    }
    if (ty_optline_consume_non_option(&optl)) {
        ty_log(TY_LOG_ERROR, "No positional argument is allowed");
        print_list_usage(stderr);
        return EXIT_FAILURE;
    }

    r = get_monitor(&monitor);
    if (r < 0)
        return EXIT_FAILURE;

    r = ty_monitor_list(monitor, list_callback, NULL);
    if (r < 0)
        return EXIT_FAILURE;

    if (watch) {
        r = ty_monitor_register_callback(monitor, list_callback, NULL);
        if (r < 0)
            return EXIT_FAILURE;

        r = ty_monitor_wait(monitor, NULL, NULL, -1);
        if (r < 0)
            return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
