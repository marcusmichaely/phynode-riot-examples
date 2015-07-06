/*
 * Copyright (C) 2014 INRIA
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License. See the file LICENSE in the top level directory for more
 * details.
 */

/**
 * @file
 * @brief       6LoWPAN example application helper functions
 *
 * @author      Oliver Hahm <oliver.hahm@inria.fr>
 */

#include "msg.h"
#include "sixlowpan/ip.h"

#include "sixlowapp.h"

#define ENABLE_DEBUG            (0)
#include "debug.h"

uint64_t sixlowapp_wait_for_msg_type(msg_t *m, timex_t timeout, uint16_t mtype)
{
    timex_t t1, t2, delta;
    delta = timex_set(0, 0);
    vtimer_now(&t1);
    while (timex_cmp(delta, timeout) < 0) {
        if (vtimer_msg_receive_timeout(m, timeout) < 0) {
            return 0;
        }
        vtimer_now(&t2);
        delta = timex_sub(t2, t1);
        if (m->type == mtype) {
            return timex_uint64(delta);
        }
        timeout = timex_sub(timeout, delta);
    }
    return 0;
}
