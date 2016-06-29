/*
 * ty, a collection of GUI and command-line tools to manage Teensy devices
 *
 * Distributed under the MIT license (see LICENSE.txt or http://opensource.org/licenses/MIT)
 * Copyright (c) 2015 Niels Martignène <niels.martignene@gmail.com>
 */

#include <getopt.h>
#include <stdarg.h>
#include "main.h"

static const char *short_options = COMMON_SHORT_OPTIONS"O:vw";
static const struct option long_options[] = {
    COMMON_LONG_OPTIONS
    {"output",  required_argument, NULL, 'O'},
    {"verbose", no_argument,       NULL, 'v'},
    {"watch",   no_argument,       NULL, 'w'},
    {0}
};

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
    } else {
        print_field("action", "%s", action);
        print_field("tag", "%s", ty_board_get_tag(board));
        print_field("serial", "%"PRIu64, ty_board_get_serial_number(board));
        if (model)
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
    ty_monitor *monitor;
    int r;

    int c;
    while ((c = getopt_long(argc, argv, short_options, long_options, NULL)) != -1) {
        switch (c) {
        HANDLE_COMMON_OPTIONS(c, print_list_usage);

        case 'O':
            if (strcmp(optarg, "plain") == 0) {
                output = OUTPUT_PLAIN;
            } else if (strcmp(optarg, "json") == 0) {
                output = OUTPUT_JSON;
            } else {
                ty_log(TY_LOG_ERROR, "--output must be one off plain or json");
                print_list_usage(stderr);
                return EXIT_FAILURE;
            }
            break;
        case 'v':
            verbose = true;
            break;
        case 'w':
            watch = true;
            break;
        }
    }

    if (argc > optind) {
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