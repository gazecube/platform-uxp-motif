/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef nsLookAndFeel_h__
#define nsLookAndFeel_h__

#include "nsXPLookAndFeel.h"
#include "gfxFont.h"

class nsLookAndFeel final : public nsXPLookAndFeel
{
public:
  nsLookAndFeel();
  ~nsLookAndFeel() override;

  nsresult NativeGetColor(ColorID aID, nscolor& aResult) override;
  nsresult GetIntImpl(IntID aID, int32_t& aResult) override;
  nsresult GetFloatImpl(FloatID aID, float& aResult) override;
  bool GetFontImpl(FontID aID, nsString& aFontName,
                   gfxFontStyle& aFontStyle,
                   float aDevPixPerCSSPixel) override;
  void RefreshImpl() override;
  char16_t GetPasswordCharacterImpl() override;
  bool GetEchoPasswordImpl() override;
};

#endif
