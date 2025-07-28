/*
 * Copyright (C) 2025 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "events.h"

#include <kernel/event.h>
#include <kernel/mutex.h>
#include <kernel/thread.h>
#include <lk/err.h>
#include <lk/trace.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uefi/types.h>

#define LOCAL_TRACE 0

namespace {
constexpr size_t kMaxEventCount = 16;
struct {
 public:
  template <typename Func>
  void read_access(Func f) const {
    AutoLock a(&m);
    f(arr, events);
  }

  template <typename Func>
  void write_access(Func f) {
    AutoLock a(&m);
    f(arr, events);
  }
  size_t get_events_count() const { return events; }

 private:
  // Cache up to 16 completed events
  mutable Mutex m;
  EfiEventImpl *volatile arr[kMaxEventCount];
  volatile size_t events = 0;
} completed_events;

lk_time_t backoff_wait_time = 1;

}  // namespace

EfiStatus wait_for_event(size_t num_events, EfiEvent *event, size_t *index) {
  LTRACEF("waiting for %zu events\n", num_events);
  for (size_t i = 0; i < num_events; i++) {
    EfiEventImpl *ev = reinterpret_cast<EfiEventImpl *>(event[i]);
    if (ev->ready()) {
      *index = i;
      return SUCCESS;
    }
  }
  while (true) {
    for (size_t i = 0; i < num_events; i++) {
      EfiEventImpl *ev = reinterpret_cast<EfiEventImpl *>(event[i]);
      if (ev->ready()) {
        *index = i;
        return SUCCESS;
      }
      auto status = event_wait_timeout(&ev->ev, 200);
      if (status == ERR_TIMED_OUT) {
        continue;
      }
      return SUCCESS;
    }
  }
  return NOT_READY;
}

EfiStatus signal_event(EfiEvent event) {
  LTRACEF("%s(type=0x%x, ready=%d)\n", __FUNCTION__, event->type,
          event->ready());
  if (event->ready()) {
    printf("Event %p already signaled\n", event);
    return SUCCESS;
  }
  event_signal(&event->ev, true);
  if ((event->type & NOTIFY_SIGNAL) && event->notify_fn != nullptr) {
    // If this event is signaled on a different thread,  defer
    // calling callbacks until the next check_event call. As UEFI apps
    // are single threaded, we don't want to call event callbacks from another
    // thread
    if (event->creator_thread != get_current_thread()) {
      LTRACEF(
          "Event %p of type 0x%x is signaled from thread %s, defer notify_fn "
          "because event is created on thread %s\n",
          event, event->type, get_current_thread()->name,
          event->creator_thread->name);
      bool success = false;
      while (!success) {
        completed_events.write_access(
            [event, &success](auto &&events, auto &i) {
              if (i >= kMaxEventCount) {
                return;
              }
              events[i++] = event;
              success = true;
            });
        if (!success) {
          thread_yield();
        }
      }
      return SUCCESS;
    }
    event->notify_fn(event, event->notify_ctx);
    event->callback_called = true;
  }
  return SUCCESS;
}

EfiStatus create_event(EfiEventType type, EfiTpl notify_tpl,
                       EfiEventNotify notify_fn, void *notify_ctx,
                       EfiEvent *event) {
  if ((type & TIMER) != 0) {
    printf("Creating timer event is not supported yet\n");
    return UNSUPPORTED;
  }
  if ((type & SIGNAL_EXIT_BOOT_SERVICES) == SIGNAL_EXIT_BOOT_SERVICES ||
      (type & SIGNAL_VIRTUAL_ADDRESS_CHANGE) == SIGNAL_VIRTUAL_ADDRESS_CHANGE) {
    printf(
        "Creating SIGNAL_EXIT_BOOT_SERVICES or SIGNAL_VIRTUAL_ADDRESS_CHANGE "
        "event is not supported yet 0x%x\n",
        type);
    return UNSUPPORTED;
  }
  if ((type & NOTIFY_WAIT)) {
    printf("Creating NOTIFY_WAIT event is not supported yet\n");
  }
  auto ev = reinterpret_cast<EfiEventImpl *>(malloc(sizeof(EfiEventImpl)));
  memset(ev, 0, sizeof(EfiEventImpl));
  ev->type = type;
  event_init(&ev->ev, false, 0);
  ev->notify_ctx = notify_ctx;
  ev->notify_fn = notify_fn;
  ev->creator_thread = get_current_thread();
  LTRACEF("Created event 0x%x callback %p %p on thread %s\n", type, notify_fn,
          notify_ctx, get_current_thread()->name);
  *event = ev;
  return SUCCESS;
}

EfiStatus check_event(EfiEvent event) {
  while (completed_events.get_events_count() > 0) {
    completed_events.write_access([](auto &&events, auto &count) {
      for (size_t i = 0; i < count; i++) {
        EfiEventImpl *ev = events[i];
        if (!ev->callback_called) {
          // reset exponential backoff timer
          LTRACEF("Triggering event %p callback %p %p on thread %s\n", ev,
                  ev->notify_fn, ev->notify_ctx, get_current_thread()->name);
          backoff_wait_time = 0;
          ev->notify_fn(ev, ev->notify_ctx);
          ev->callback_called = true;
          events[i] = nullptr;
        } else {
          printf("Event %p with callback %p %p already called, unusual\n", ev,
                 ev->notify_fn, ev->notify_ctx);
        }
      }
      count = 0;
    });
  }
  // Some UEFI applications repeadtely call check_event(NULL) as a way to poll
  // for completed events. To avoid busy waiting, implement an exponential
  // backoff strategy if check_event is called repeatdely AND no events in the
  // system are completed.
  if (event == nullptr) {
    if (backoff_wait_time == 0) {
      thread_yield();
      backoff_wait_time++;
    } else {
      thread_sleep(backoff_wait_time);
      if (backoff_wait_time < 500) {
        backoff_wait_time <<= 1;
      }
    }
    return INVALID_PARAMETER;
  }
  if (event->ready()) {
    return SUCCESS;
  }
  return NOT_READY;
}

EfiStatus close_event(EfiEvent event) {
  event_destroy(&event->ev);
  free(event);
  return SUCCESS;
}