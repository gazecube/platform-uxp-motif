/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nsScreenManagerMotif.h"
#include "nsScreenMotif.h"
#include "nsAppShell.h"
#include "nsRect.h"

#include <X11/Xlib.h>

NS_IMPL_ISUPPORTS(nsScreenManagerMotif, nsIScreenManager)

nsScreenManagerMotif::nsScreenManagerMotif() = default;

nsresult
nsScreenManagerMotif::EnsureInit()
{
  if (mScreens.Count()) {
    return NS_OK;
  }
  Display* dpy = nsAppShell::GetDisplay();
  if (!dpy) {
    return NS_ERROR_NOT_INITIALIZED;
  }
  const int count = ScreenCount(dpy);
  for (int i = 0; i < count; ++i) {
    nsCOMPtr<nsIScreen> screen = new nsScreenMotif(i);
    mScreens.AppendObject(screen);
  }
  return mScreens.Count() ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsScreenManagerMotif::ScreenForRect(int32_t aX, int32_t aY,
                                    int32_t aWidth, int32_t aHeight,
                                    nsIScreen** aOutScreen)
{
  *aOutScreen = nullptr;
  nsresult rv = EnsureInit();
  if (NS_FAILED(rv)) return rv;

  uint32_t bestArea = 0;
  int32_t best = 0;
  nsIntRect wanted(aX, aY, aWidth, aHeight);
  for (int32_t i = 0; i < mScreens.Count(); ++i) {
    int32_t x, y, w, h;
    mScreens[i]->GetRect(&x, &y, &w, &h);
    nsIntRect intersection;
    intersection.IntersectRect(nsIntRect(x, y, w, h), wanted);
    uint32_t area = std::max(0, intersection.width) *
                    std::max(0, intersection.height);
    if (area >= bestArea) {
      bestArea = area;
      best = i;
    }
  }
  NS_IF_ADDREF(*aOutScreen = mScreens.SafeObjectAt(best));
  return *aOutScreen ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsScreenManagerMotif::ScreenForId(uint32_t aId, nsIScreen** aOutScreen)
{
  *aOutScreen = nullptr;
  nsresult rv = EnsureInit();
  if (NS_FAILED(rv)) return rv;
  for (int32_t i = 0; i < mScreens.Count(); ++i) {
    uint32_t id = UINT32_MAX;
    mScreens[i]->GetId(&id);
    if (id == aId) {
      NS_IF_ADDREF(*aOutScreen = mScreens[i]);
      return NS_OK;
    }
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsScreenManagerMotif::GetPrimaryScreen(nsIScreen** aPrimaryScreen)
{
  *aPrimaryScreen = nullptr;
  nsresult rv = EnsureInit();
  if (NS_FAILED(rv)) return rv;
  NS_IF_ADDREF(*aPrimaryScreen = mScreens.SafeObjectAt(0));
  return *aPrimaryScreen ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsScreenManagerMotif::GetNumberOfScreens(uint32_t* aNumberOfScreens)
{
  nsresult rv = EnsureInit();
  if (NS_FAILED(rv)) return rv;
  *aNumberOfScreens = mScreens.Count();
  return NS_OK;
}

NS_IMETHODIMP
nsScreenManagerMotif::GetSystemDefaultScale(float* aDefaultScale)
{
  *aDefaultScale = 1.0f;
  return NS_OK;
}

NS_IMETHODIMP
nsScreenManagerMotif::ScreenForNativeWidget(void* aWidget,
                                            nsIScreen** aOutScreen)
{
  // Xt widgets may be translated to root coordinates, but the single-screen
  // path is overwhelmingly common for the IRIX target and remains correct on
  // ordinary X11.  Multi-screen callers fall back to the primary X screen.
  return GetPrimaryScreen(aOutScreen);
}
