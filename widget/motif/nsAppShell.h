/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef nsAppShell_h__
#define nsAppShell_h__

#include "nsBaseAppShell.h"
#include <X11/Intrinsic.h>

class nsAppShell final : public nsBaseAppShell
{
public:
    nsAppShell();

    nsresult Init();
    void ScheduleNativeEventCallback() override;
    bool ProcessNextNativeEvent(bool aMayWait) override;

    static XtAppContext GetAppContext();
    static Display* GetDisplay();

private:
    ~nsAppShell() override;

    static void EventProcessorCallback(XtPointer aClosure,
                                       int* aFd,
                                       XtInputId* aId);

    int mPipeFDs[2];
    XtInputId mInputId;

    static XtAppContext sAppContext;
    static Display* sDisplay;
};

#endif /* nsAppShell_h__ */
