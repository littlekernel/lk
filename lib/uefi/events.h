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

#ifndef __LIB_UEFI_EVENTS_H_
#define __LIB_UEFI_EVENTS_H_
#include <kernel/event.h>
#include <kernel/thread.h>
#include <lk/list.h>
#include <uefi/types.h>

struct EfiEventImpl final {
  EfiEventType type;
  event_t ev;
  EfiEventNotify notify_fn = nullptr;
  void *notify_ctx = nullptr;
  volatile bool callback_called = false;
  thread_t *creator_thread = nullptr;
  bool ready() const { return ev.signaled; }
  struct list_node node;
};

EfiStatus wait_for_event(size_t num_events, EfiEvent *event, size_t *index);

// Safe to call from interrupt context
EfiStatus signal_event(EfiEvent event);

EfiStatus check_event(EfiEvent event);

EfiStatus create_event(EfiEventType type, EfiTpl notify_tpl,
                       EfiEventNotify notify_fn, void *notify_ctx,
                       EfiEvent *event);

EfiStatus close_event(EfiEvent event);

EfiStatus set_timer(EfiEvent event, EfiTimerDelay type, uint64_t trigger_time);

#endif
