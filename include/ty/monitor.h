/*
 * ty, a collection of GUI and command-line tools to manage Teensy devices
 *
 * Distributed under the MIT license (see LICENSE.txt or http://opensource.org/licenses/MIT)
 * Copyright (c) 2015 Niels Martignène <niels.martignene@gmail.com>
 */

#ifndef TY_MONITOR_H
#define TY_MONITOR_H

#include "common.h"

TY_C_BEGIN

struct ty_board;

typedef struct ty_monitor ty_monitor;

enum {
    TY_MONITOR_PARALLEL_WAIT = 1
};

typedef enum ty_monitor_event {
    TY_MONITOR_EVENT_ADDED,
    TY_MONITOR_EVENT_CHANGED,
    TY_MONITOR_EVENT_DISAPPEARED,
    TY_MONITOR_EVENT_DROPPED
} ty_monitor_event;

typedef int ty_monitor_callback_func(struct ty_board *board, ty_monitor_event event, void *udata);
typedef int ty_monitor_wait_func(ty_monitor *monitor, void *udata);

TY_PUBLIC int ty_monitor_new(int flags, ty_monitor **rmonitor);
TY_PUBLIC void ty_monitor_free(ty_monitor *monitor);

TY_PUBLIC int ty_monitor_start(ty_monitor *monitor);
TY_PUBLIC void ty_monitor_stop(ty_monitor *monitor);

TY_PUBLIC void ty_monitor_set_udata(ty_monitor *monitor, void *udata);
TY_PUBLIC void *ty_monitor_get_udata(const ty_monitor *monitor);

TY_PUBLIC void ty_monitor_get_descriptors(const ty_monitor *monitor, struct ty_descriptor_set *set, int id);

TY_PUBLIC int ty_monitor_register_callback(ty_monitor *monitor, ty_monitor_callback_func *f, void *udata);
TY_PUBLIC void ty_monitor_deregister_callback(ty_monitor *monitor, int id);

TY_PUBLIC int ty_monitor_refresh(ty_monitor *monitor);
TY_PUBLIC int ty_monitor_wait(ty_monitor *monitor, ty_monitor_wait_func *f, void *udata, int timeout);

TY_PUBLIC int ty_monitor_list(ty_monitor *monitor, ty_monitor_callback_func *f, void *udata);

TY_C_END

#endif
