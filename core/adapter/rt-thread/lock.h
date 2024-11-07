/*
 * Copyright (C) 2024 Rockchip Electronics Co., Ltd.
 * Authors:
 *  Cerf Yu <cerf.yu@rock-chips.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __RTT_ADAPTER_LINUX_LOCK_H__
#define __RTT_ADAPTER_LINUX_LOCK_H__

#include <rtdevice.h>

#include <assert.h>
#include <errno.h>

#if 0
/* mutex */
#define mutex rt_mutex

#define mutex_lock rt_mutex_take
#define mutex_unlock rt_mutex_release
#define mutex_trylock rt_mutex_trytake

static inline rt_err_t mutex_is_locked(struct rt_mutex *m)
{
    rt_err_t rc = mutex_trylock(m);
    if (rc == RT_EOK) {
        // Locked, so was not locked; unlock and return not locked
        mutex_unlock(m);
        return 0;
    } else if (rc == -RT_ETIMEOUT) {
        // Already locked
        return 1;
    } else {
        // Error occurred, so we do not really know, but be conversative
        return 1;
    }
}

#define mutex_init(m) ({ \
    int __rc = rt_mutex_init(m, "rga_mutex", RT_IPC_FLAG_FIFO); \
    assert(__rc == RT_EOK); \
    __rc; \
})

#define mutex_destroy(m) ({ \
    int __rc = rt_mutex_detach(m); \
    assert(__rc == RT_EOK); \
    __rc; \
})

/* spinlock */
typedef struct rt_spinlock spinlock_t;

#define spin_lock_irqsave(lock, flag) ({ \
    flag = rt_spin_lock_irqsave(lock); \
})
#define spin_unlock_irqrestore rt_spin_unlock_irqrestore
#define spin_lock_init rt_spin_lock_init
#endif

#ifndef RT_USING_PTHREADS

/* pthread_mutex */
#define pthread_mutex_t struct rt_mutex
// typedef struct rt_mutex pthread_mutex_t;

#define pthread_mutex_lock(m) rt_mutex_take(m, RT_WAITING_FOREVER)
#define pthread_mutex_unlock rt_mutex_release

#define pthread_mutex_init(m, attr) ({ \
    int __rc = rt_mutex_init(m, "rga_mutex", RT_IPC_FLAG_FIFO); \
    assert(__rc == RT_EOK); \
    __rc; \
})

#define pthread_mutex_destroy(m) ({ \
    int __rc = rt_mutex_detach(m); \
    assert(__rc == RT_EOK); \
    __rc; \
})

/* pthread_rwlock */
typedef struct rt_mutex pthread_rwlock_t;

#define pthread_rwlock_rdlock(m) rt_mutex_take(m, RT_WAITING_FOREVER)
#define pthread_rwlock_wrlock(m) rt_mutex_take(m, RT_WAITING_FOREVER)
#define pthread_rwlock_unlock rt_mutex_release

#define pthread_rwlock_init(m, attr) ({ \
    int __rc = rt_mutex_init(m, "rga_mutex_rwlock", RT_IPC_FLAG_FIFO); \
    assert(__rc == RT_EOK); \
    __rc; \
})

#define pthread_rwlock_destroy (m) ({ \
    int __rc = rt_mutex_detach(m); \
    assert(__rc == RT_EOK); \
    __rc; \
})

#endif /* #ifndef RT_USING_PTHREADS */

#endif /* #ifndef __RTT_ADAPTER_LINUX_LOCK_H__ */
