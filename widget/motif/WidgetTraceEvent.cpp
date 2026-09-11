/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/WidgetTraceEvent.h"
#include "nsAppShell.h"

#include "mozilla/CondVar.h"
#include "mozilla/Mutex.h"

using mozilla::CondVar;
using mozilla::Mutex;
using mozilla::MutexAutoLock;

namespace {

Mutex* sMutex = nullptr;
CondVar* sCondVar = nullptr;
bool sTracerProcessed = false;

void
TracerCallback(XtPointer aClosure, XtIntervalId* aId)
{
  mozilla::SignalTracerThread();
}

} // namespace

namespace mozilla {

bool
InitWidgetTracing()
{
  sMutex = new Mutex("Event tracer thread mutex");
  sCondVar = new CondVar(*sMutex, "Event tracer thread condvar");
  return true;
}

void
CleanUpWidgetTracing()
{
  delete sMutex;
  delete sCondVar;
  sMutex = nullptr;
  sCondVar = nullptr;
}

bool
FireAndWaitForTracerEvent()
{
  MOZ_ASSERT(sMutex && sCondVar, "Tracing not initialized!");

  XtAppContext appContext = nsAppShell::GetAppContext();
  if (!appContext) {
    return false;
  }

  MutexAutoLock lock(*sMutex);
  MOZ_ASSERT(!sTracerProcessed, "Tracer synchronization state is wrong");

  XtAppAddTimeOut(appContext, 0, TracerCallback, nullptr);
  while (!sTracerProcessed) {
    sCondVar->Wait();
  }
  sTracerProcessed = false;
  return true;
}

void
SignalTracerThread()
{
  if (!sMutex || !sCondVar) {
    return;
  }

  MutexAutoLock lock(*sMutex);
  if (!sTracerProcessed) {
    sTracerProcessed = true;
    sCondVar->Notify();
  }
}

} // namespace mozilla
