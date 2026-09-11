/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/message_loop.h"

#include <algorithm>

#include "mozilla/Atomics.h"
#include "base/compiler_specific.h"
#include "base/logging.h"
#include "base/message_pump_default.h"
#include "base/string_util.h"
#include "base/thread_local.h"

#if defined(OS_MACOSX)
#include "base/message_pump_mac.h"
#endif
#if defined(OS_POSIX)
#include "base/message_pump_libevent.h"
#endif
#if defined(OS_LINUX) || defined(OS_BSD) || defined (OS_SOLARIS)
#if defined(MOZ_WIDGET_GTK)
#include "base/message_pump_glib.h"
#endif
#endif
#ifdef MOZ_TASK_TRACER
#include "GeckoTaskTracer.h"
#include "TracedTaskCommon.h"
#endif

#include "MessagePump.h"

using base::Time;
using base::TimeDelta;
using base::TimeTicks;

using mozilla::Move;
using mozilla::Runnable;

static base::ThreadLocalPointer<MessageLoop>& get_tls_ptr() {
  static base::ThreadLocalPointer<MessageLoop> tls_ptr;
  return tls_ptr;
}

//------------------------------------------------------------------------------

// Logical events for Histogram profiling. Run with -message-loop-histogrammer
// to get an accounting of messages and actions taken on each thread.
static const int kTaskRunEvent = 0x1;
static const int kTimerEvent = 0x2;
static const int kMaxMessageId = 1099;
static const int kNumberOfDistinctMessagesDisplayed = 1100;

//------------------------------------------------------------------------------

#if defined(OS_WIN)

// Upon a SEH exception in this thread, it restores the original unhandled
// exception filter.
static int SEHFilter(LPTOP_LEVEL_EXCEPTION_FILTER old_filter) {
  ::SetUnhandledExceptionFilter(old_filter);
  return EXCEPTION_CONTINUE_SEARCH;
}

// Retrieves a pointer to the current unhandled exception filter. There is no
// standalone getter method.
static LPTOP_LEVEL_EXCEPTION_FILTER GetTopSEHFilter() {
  LPTOP_LEVEL_EXCEPTION_FILTER top_filter = NULL;
  top_filter = ::SetUnhandledExceptionFilter(0);
  ::SetUnhandledExceptionFilter(top_filter);
  return top_filter;
}

#endif  // defined(OS_WIN)

//------------------------------------------------------------------------------

MessageLoop::MessageLoop(Type type, nsIThread* aThread)
    : type_(type),
      id_(++message_loop_id_seq),
      nestable_tasks_allowed_(true),
      exception_restoration_(false),
      state_(NULL),
      run_depth_base_(1),
#ifdef OS_WIN
      os_modal_loop_(false),
#endif  // OS_WIN
      transient_hang_timeout_(0),
      permanent_hang_timeout_(0),
      next_sequence_num_(0) {
  DCHECK(!current()) << "should only have one message loop per thread";
  get_tls_ptr().Set(this);

  switch (type_) {
  case TYPE_MOZILLA_PARENT:
    MOZ_RELEASE_ASSERT(!aThread);
    pump_ = new mozilla::ipc::MessagePump(aThread);
    return;
  case TYPE_MOZILLA_CHILD:
    MOZ_RELEASE_ASSERT(!aThread);
    pump_ = new mozilla::ipc::MessagePumpForChildProcess();
    // There is a MessageLoop Run call from XRE_InitChildProcess
    // and another one from MessagePumpForChildProcess. The one
    // from MessagePumpForChildProcess becomes the base, so we need
    // to set run_depth_base_ to 2 or we'll never be able to process
    // Idle tasks.
    run_depth_base_ = 2;
    return;
  case TYPE_MOZILLA_NONMAINTHREAD:
    pump_ = new mozilla::ipc::MessagePumpForNonMainThreads(aThread);
    return;
#if defined(OS_WIN)
  case TYPE_MOZILLA_NONMAINUITHREAD:
    pump_ = new mozilla::ipc::MessagePumpForNonMainUIThreads(aThread);
    return;
#endif
  default:
    // Create one of Chromium's standard MessageLoop types below.
    break;
  }

#if defined(OS_WIN)
  // TODO(rvargas): Get rid of the OS guards.
  if (type_ == TYPE_DEFAULT) {
    pump_ = new base::MessagePumpDefault();
  } else if (type_ == TYPE_IO) {
    pump_ = new base::MessagePumpForIO();
  } else {
    DCHECK(type_ == TYPE_UI);
    pump_ = new base::MessagePumpForUI();
  }
#elif defined(OS_POSIX)
  if (type_ == TYPE_UI) {
#if defined(OS_MACOSX)
    pump_ = base::MessagePumpMac::Create();
#elif defined(OS_LINUX) || defined(OS_BSD) || defined(OS_SOLARIS)
#if defined(MOZ_WIDGET_GTK)
    pump_ = new base::MessagePumpForUI();
#else
    // Non-GTK Unix backends do not have Chromium's GLib UI pump available.
    // Motif/Xt integration is owned by nsAppShell, so use the generic pump
    // for Chromium TYPE_UI loops instead of depending on GTK/GLib.
    pump_ = new base::MessagePumpDefault();
#endif
#endif  // OS_LINUX
  } else if (type_ == TYPE_IO) {
    pump_ = new base::MessagePumpLibevent();
  } else {
    pump_ = new base::MessagePumpDefault();
  }
#endif  // OS_POSIX
}

MessageLoop::~MessageLoop() {
  DCHECK(this == current());

  // Let interested parties have one last shot at accessing this.
  FOR_EACH_OBSERVER(DestructionObserver, destruction_observers_,
                    WillDestroyCurrentMessageLoop());

  DCHECK(!state_);

  // Clean up any unprocessed tasks, but take care: deleting a task could
  // result in the addition of more tasks (e.g., via DeleteSoon).  We set a
  // limit on the number of times we will allow a deleted task to generate more
  // tasks.  Normally, we should always process everything without hitting this
  // limit.  If we hit it, that probably means we have one task that is being
  // posted over and over from another task.
  for (int i = 0; i < 100; ++i) {
    DeletePendingTasks();
    if (!DeferOrRunPendingTask(NULL))
      break;
  }

  DCHECK(this == current());
  get_tls_ptr().Set(NULL);
}

// static
MessageLoop* MessageLoop::current() {
  return get_tls_ptr().Get();
}

void MessageLoop::Run() {
  DCHECK_EQ(this, current());

  RunHandler handler;
  handler.Run(this);
}

void MessageLoop::RunHandler::Run(MessageLoop* loop) {
  DCHECK(loop);
  run_loop_ = loop;
  MessageLoop::RunState state(loop);
  loop->RunInternal();
  run_loop_ = NULL;
}

// static
MessageLoop::RunHandler* MessageLoop::RunHandler::current() {
  MessageLoop* loop = MessageLoop::current();
  return loop ? loop->run_handler_ : NULL;
}

void MessageLoop::RunInternal() {
  AutoRunState save_state(this);
  pump_->Run(this);
}

void MessageLoop::Quit() {
  DCHECK_EQ(this, current());
  if (state_) {
    state_->quit_received = true;
    pump_->Quit();
  } else {
    NOTREACHED();
  }
}

void MessageLoop::QuitNow() {
  DCHECK_EQ(this, current());
  DCHECK(state_);
  state_->quit_received = true;
  pump_->Quit();
}

void MessageLoop::QuitWhenIdle() {
  DCHECK_EQ(this, current());
  state_->quit_when_idle_received = true;
}

void MessageLoop::PostTask(const tracked_objects::Location& from_here,
                           Task* task) {
  task_runner_->PostTask(from_here, task);
}

void MessageLoop::PostDelayedTask(const tracked_objects::Location& from_here,
                                  Task* task,
                                  int delay_ms) {
  task_runner_->PostDelayedTask(from_here, task, delay_ms);
}

void MessageLoop::PostNonNestableTask(
    const tracked_objects::Location& from_here, Task* task) {
  task_runner_->PostNonNestableTask(from_here, task);
}

void MessageLoop::PostNonNestableDelayedTask(
    const tracked_objects::Location& from_here,
    Task* task,
    int delay_ms) {
  task_runner_->PostNonNestableDelayedTask(from_here, task, delay_ms);
}

bool MessageLoop::DoWork() {
  for (;;) {
    bool did_work = DeferOrRunPendingTask(NULL);
    if (!did_work)
      break;
  }
  return false;
}

bool MessageLoop::DoDelayedWork(TimeTicks* next_delayed_work_time) {
  if (delayed_work_queue_.empty()) {
    recent_time_ = *next_delayed_work_time = TimeTicks();
    return false;
  }

  TimeTicks next_run_time = delayed_work_queue_.top().delayed_run_time;
  if (next_run_time > recent_time_) {
    recent_time_ = TimeTicks::Now();
  }

  if (next_run_time > recent_time_) {
    *next_delayed_work_time = next_run_time;
    return false;
  }

  PendingTask pending_task = delayed_work_queue_.top();
  delayed_work_queue_.pop();

  if (pending_task.sequence_num != next_sequence_num_) {
    incoming_queue_.AddToDelayedWorkQueue(&delayed_work_queue_);
  }

  if (!DeferOrRunPendingTask(&pending_task)) {
    return false;
  }

  return true;
}

bool MessageLoop::DoIdleWork() {
  if (ProcessNextDelayedNonNestableTask())
    return true;

  if (state_->quit_when_idle_received) {
    state_->quit_received = true;
    pump_->Quit();
  }

  return false;
}

bool MessageLoop::DeferOrRunPendingTask(const PendingTask* pending_task) {
  PendingTask task;
  if (pending_task) {
    task = *pending_task;
  } else {
    if (!incoming_queue_.ReloadWorkQueue(&work_queue_))
      return false;

    task = work_queue_.front();
    work_queue_.pop();
  }

  if (task.delayed_run_time.is_null()) {
    RunTask(task);
  } else {
    delayed_work_queue_.push(task);
  }

  return true;
}

void MessageLoop::RunTask(const PendingTask& pending_task) {
  DCHECK_EQ(this, current());

  base::TimeTicks start = base::TimeTicks::Now();
  Task* task = pending_task.task;
  tracked_objects::TaskStopwatch stopwatch;
  stopwatch.Start();

  if (task->Run()) {
    delete task;
  }

  stopwatch.Stop();

  base::TimeDelta duration = stopwatch.Elapsed();
  if (duration > transient_hang_timeout_) {
    if (duration > permanent_hang_timeout_ && permanent_hang_timeout_ > 0) {
      OnPermanentHang();
    } else if (transient_hang_timeout_ > 0) {
      OnTransientHang();
    }
  }
}

void MessageLoop::DeletePendingTasks() {
  incoming_queue_.WillDestroyCurrentMessageLoop();
  work_queue_.clear();
  delayed_work_queue_ = DelayedTaskQueue();
}

void MessageLoop::SetNestableTasksAllowed(bool allowed) {
  DCHECK_EQ(this, current());
  nestable_tasks_allowed_ = allowed;
}

bool MessageLoop::NestableTasksAllowed() const {
  return nestable_tasks_allowed_;
}

void MessageLoop::AddDestructionObserver(DestructionObserver* obs) {
  DCHECK_EQ(this, current());
  destruction_observers_.AddObserver(obs);
}

void MessageLoop::RemoveDestructionObserver(DestructionObserver* obs) {
  DCHECK_EQ(this, current());
  destruction_observers_.RemoveObserver(obs);
}

void MessageLoop::AddTaskObserver(TaskObserver* obs) {
  DCHECK_EQ(this, current());
  task_observers_.AddObserver(obs);
}

void MessageLoop::RemoveTaskObserver(TaskObserver* obs) {
  DCHECK_EQ(this, current());
  task_observers_.RemoveObserver(obs);
}

void MessageLoop::OnTransientHang() {
}

void MessageLoop::OnPermanentHang() {
}
