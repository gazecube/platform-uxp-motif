/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef nsScreenMotif_h__
#define nsScreenMotif_h__

#include "nsBaseScreen.h"
#include "nsRect.h"

class nsScreenMotif final : public nsBaseScreen
{
public:
  explicit nsScreenMotif(uint32_t aScreenNum);

  NS_IMETHOD GetId(uint32_t* aId) override;
  NS_IMETHOD GetRect(int32_t* aLeft, int32_t* aTop,
                     int32_t* aWidth, int32_t* aHeight) override;
  NS_IMETHOD GetAvailRect(int32_t* aLeft, int32_t* aTop,
                          int32_t* aWidth, int32_t* aHeight) override;
  NS_IMETHOD GetPixelDepth(int32_t* aPixelDepth) override;
  NS_IMETHOD GetColorDepth(int32_t* aColorDepth) override;
  NS_IMETHOD GetDefaultCSSScaleFactor(double* aScaleFactor) override;

private:
  ~nsScreenMotif() override = default;
  uint32_t mScreenNum;
};

#endif
