/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nsScreenMotif.h"
#include "nsAppShell.h"

#include <X11/Xlib.h>

nsScreenMotif::nsScreenMotif(uint32_t aScreenNum)
  : mScreenNum(aScreenNum)
{
}

NS_IMETHODIMP
nsScreenMotif::GetId(uint32_t* aId)
{
  *aId = mScreenNum;
  return NS_OK;
}

NS_IMETHODIMP
nsScreenMotif::GetRect(int32_t* aLeft, int32_t* aTop,
                       int32_t* aWidth, int32_t* aHeight)
{
  Display* dpy = nsAppShell::GetDisplay();
  if (!dpy) {
    return NS_ERROR_NOT_INITIALIZED;
  }
  *aLeft = 0;
  *aTop = 0;
  *aWidth = DisplayWidth(dpy, mScreenNum);
  *aHeight = DisplayHeight(dpy, mScreenNum);
  return NS_OK;
}

NS_IMETHODIMP
nsScreenMotif::GetAvailRect(int32_t* aLeft, int32_t* aTop,
                            int32_t* aWidth, int32_t* aHeight)
{
  // Motif itself does not impose a work area. The WM may publish EWMH
  // _NET_WORKAREA, but IRIX-era WMs are not required to do so. Start with
  // the complete X screen; the window manager remains free to constrain it.
  return GetRect(aLeft, aTop, aWidth, aHeight);
}

NS_IMETHODIMP
nsScreenMotif::GetPixelDepth(int32_t* aPixelDepth)
{
  Display* dpy = nsAppShell::GetDisplay();
  if (!dpy) {
    return NS_ERROR_NOT_INITIALIZED;
  }
  *aPixelDepth = DefaultDepth(dpy, mScreenNum);
  return NS_OK;
}

NS_IMETHODIMP
nsScreenMotif::GetColorDepth(int32_t* aColorDepth)
{
  return GetPixelDepth(aColorDepth);
}

NS_IMETHODIMP
nsScreenMotif::GetDefaultCSSScaleFactor(double* aScaleFactor)
{
  *aScaleFactor = 1.0;
  return NS_OK;
}
