/*
 * ty, a collection of GUI and command-line tools to manage Teensy devices
 *
 * Distributed under the MIT license (see LICENSE.txt or http://opensource.org/licenses/MIT)
 * Copyright (c) 2015 Niels Martignène <niels.martignene@gmail.com>
 */

#ifndef TY_COMMON_PRIV_H
#define TY_COMMON_PRIV_H

#include "common.h"
#include "compat_priv.h"

void _ty_refcount_increase(unsigned int *rrefcount);
unsigned int _ty_refcount_decrease(unsigned int *rrefcount);

#endif
