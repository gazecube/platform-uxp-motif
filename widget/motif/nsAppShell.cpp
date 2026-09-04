/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nsAppShell.h"

#include "mozilla/HangMonitor.h"
#include "mozilla/Unused.h"
#include "GeckoProfiler.h"

#include <X11/Shell.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

using mozilla::Unused;

#define NOTIFY_TOKEN 0xFA

XtAppContext nsAppShell::sAppContext = nullptr;
Display* nsAppShell::sDisplay = nullptr;

nsAppShell::nsAppShell()
    : mInputId(0)
{
    mPipeFDs[0] = mPipeFDs[1] = -1;
}

nsAppShell::~nsAppShell()
{
    if (mInputId) {
        XtRemoveInput(mInputId);
        mInputId = 0;
    }
    if (mPipeFDs[0] >= 0) {
        close(mPipeFDs[0]);
    }
    if (mPipeFDs[1] >= 0) {
        close(mPipeFDs[1]);
    }
}

/* static */ XtAppContext
nsAppShell::GetAppContext()
{
    return sAppContext;
}

/* static */ Display*
nsAppShell::GetDisplay()
{
    return sDisplay;
}

/* static */ void
nsAppShell::EventProcessorCallback(XtPointer aClosure,
                                   int* aFd,
                                   XtInputId* aId)
{
    nsAppShell* self = static_cast<nsAppShell*>(aClosure);
    unsigned char token;
    while (read(self->mPipeFDs[0], &token, 1) == 1) {
        // Drain all wakeup tokens before returning to Xt.
    }
    self->NativeEventCallback();
}

nsresult
nsAppShell::Init()
{
    if (!sAppContext) {
        XtToolkitInitialize();
        sAppContext = XtCreateApplicationContext();
        if (!sAppContext) {
            return NS_ERROR_FAILURE;
        }

        int argc = 0;
        char** argv = nullptr;
        sDisplay = XtOpenDisplay(sAppContext, nullptr, nullptr,
                                 const_cast<char*>("Mozilla"),
                                 nullptr, 0, &argc, argv);
        if (!sDisplay) {
            XtDestroyApplicationContext(sAppContext);
            sAppContext = nullptr;
            return NS_ERROR_FAILURE;
        }
    }

    if (pipe(mPipeFDs) != 0) {
        return NS_ERROR_FAILURE;
    }

    for (int i = 0; i < 2; ++i) {
        int flags = fcntl(mPipeFDs[i], F_GETFL, 0);
        if (flags < 0 ||
            fcntl(mPipeFDs[i], F_SETFL, flags | O_NONBLOCK) < 0) {
            return NS_ERROR_FAILURE;
        }
    }

    mInputId = XtAppAddInput(sAppContext, mPipeFDs[0],
                             reinterpret_cast<XtPointer>(XtInputReadMask),
                             EventProcessorCallback, this);

    return nsBaseAppShell::Init();
}

void
nsAppShell::ScheduleNativeEventCallback()
{
    if (mPipeFDs[1] < 0) {
        return;
    }
    unsigned char token = NOTIFY_TOKEN;
    Unused << write(mPipeFDs[1], &token, 1);
}

bool
nsAppShell::ProcessNextNativeEvent(bool aMayWait)
{
    if (!sAppContext) {
        return false;
    }

    if (!aMayWait && XtAppPending(sAppContext) == 0) {
        return false;
    }

    mozilla::HangMonitor::Suspend();
    profiler_sleep_start();

    XEvent event;
    XtAppNextEvent(sAppContext, &event);

    profiler_sleep_end();
    mozilla::HangMonitor::NotifyActivity();

    XtDispatchEvent(&event);
    return true;
}
