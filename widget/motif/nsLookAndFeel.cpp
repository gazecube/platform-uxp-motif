/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nsLookAndFeel.h"
#include "nsStyleConsts.h"

nsLookAndFeel::nsLookAndFeel() = default;
nsLookAndFeel::~nsLookAndFeel() = default;

nsresult
nsLookAndFeel::NativeGetColor(ColorID aID, nscolor& aResult)
{
  const nscolor bg = NS_RGB(0xb9, 0xb9, 0xb9);
  const nscolor light = NS_RGB(0xe8, 0xe8, 0xe8);
  const nscolor shadow = NS_RGB(0x66, 0x66, 0x66);
  const nscolor select = NS_RGB(0x54, 0x76, 0x9a);

  switch (aID) {
    case eColorID_window:
    case eColorID__moz_field:
    case eColorID__moz_eventreerow:
      aResult = NS_RGB(0xff, 0xff, 0xff); return NS_OK;
    case eColorID_windowtext:
    case eColorID__moz_fieldtext:
    case eColorID__moz_dialogtext:
    case eColorID_buttontext:
    case eColorID_menutext:
    case eColorID_captiontext:
      aResult = NS_RGB(0x00, 0x00, 0x00); return NS_OK;
    case eColorID_buttonface:
    case eColorID_threedface:
    case eColorID__moz_dialog:
    case eColorID_menu:
    case eColorID_scrollbar:
    case eColorID_activeborder:
    case eColorID_inactiveborder:
    case eColorID_background:
    case eColorID_appworkspace:
      aResult = bg; return NS_OK;
    case eColorID_buttonhighlight:
    case eColorID_threedhighlight:
    case eColorID_threedlightshadow:
      aResult = light; return NS_OK;
    case eColorID_buttonshadow:
    case eColorID_threedshadow:
      aResult = shadow; return NS_OK;
    case eColorID_threeddarkshadow:
      aResult = NS_RGB(0x22, 0x22, 0x22); return NS_OK;
    case eColorID_highlight:
    case eColorID__moz_menuhover:
      aResult = select; return NS_OK;
    case eColorID_highlighttext:
    case eColorID__moz_menuhovertext:
      aResult = NS_RGB(0xff, 0xff, 0xff); return NS_OK;
    case eColorID_graytext:
    case eColorID_inactivecaptiontext:
      aResult = NS_RGB(0x66, 0x66, 0x66); return NS_OK;
    case eColorID_infobackground:
      aResult = NS_RGB(0xff, 0xff, 0xd8); return NS_OK;
    case eColorID_infotext:
      aResult = NS_RGB(0x00, 0x00, 0x00); return NS_OK;
    case eColorID__moz_nativehyperlinktext:
      aResult = NS_RGB(0x00, 0x00, 0xee); return NS_OK;
    case eColorID_SpellCheckerUnderline:
      aResult = NS_RGB(0xff, 0x00, 0x00); return NS_OK;
    case eColorID_IMESelectedRawTextUnderline:
    case eColorID_IMESelectedConvertedTextUnderline:
      aResult = NS_TRANSPARENT; return NS_OK;
    default:
      return NS_ERROR_FAILURE;
  }
}

nsresult
nsLookAndFeel::GetIntImpl(IntID aID, int32_t& aResult)
{
  nsresult rv = nsXPLookAndFeel::GetIntImpl(aID, aResult);
  if (NS_SUCCEEDED(rv)) {
    return rv;
  }

  switch (aID) {
    case eIntID_CaretBlinkTime: aResult = 500; break;
    case eIntID_CaretWidth: aResult = 1; break;
    case eIntID_ShowCaretDuringSelection: aResult = 0; break;
    case eIntID_SelectTextfieldsOnKeyFocus: aResult = 1; break;
    case eIntID_SubmenuDelay: aResult = 200; break;
    case eIntID_TooltipDelay: aResult = 500; break;
    case eIntID_MenusCanOverlapOSBar: aResult = 1; break;
    case eIntID_SkipNavigatingDisabledMenuItem: aResult = 1; break;
    case eIntID_DragThresholdX:
    case eIntID_DragThresholdY: aResult = 4; break;
    case eIntID_ScrollArrowStyle:
      aResult = mozilla::LookAndFeel::eScrollArrowStyle_Single; break;
    case eIntID_ScrollSliderStyle:
      aResult = eScrollThumbStyle_Proportional; break;
    case eIntID_TreeOpenDelay:
    case eIntID_TreeCloseDelay: aResult = 1000; break;
    case eIntID_TreeLazyScrollDelay: aResult = 150; break;
    case eIntID_TreeScrollDelay: aResult = 100; break;
    case eIntID_TreeScrollLinesMax: aResult = 3; break;
    case eIntID_TouchEnabled: aResult = 0; break;
    case eIntID_AlertNotificationOrigin: aResult = NS_ALERT_TOP; break;
    case eIntID_ScrollbarButtonAutoRepeatBehavior: aResult = 1; break;
    case eIntID_SwipeAnimationEnabled: aResult = 0; break;
    case eIntID_ColorPickerAvailable: aResult = 0; break;
    case eIntID_ContextMenuOffsetVertical:
    case eIntID_ContextMenuOffsetHorizontal: aResult = 2; break;
    case eIntID_ScrollButtonLeftMouseButtonAction: aResult = 0; break;
    case eIntID_ScrollButtonMiddleMouseButtonAction: aResult = 1; break;
    case eIntID_ScrollButtonRightMouseButtonAction: aResult = 2; break;
    default:
      aResult = 0;
      return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

nsresult
nsLookAndFeel::GetFloatImpl(FloatID aID, float& aResult)
{
  nsresult rv = nsXPLookAndFeel::GetFloatImpl(aID, aResult);
  if (NS_SUCCEEDED(rv)) {
    return rv;
  }
  switch (aID) {
    case eFloatID_IMEUnderlineRelativeSize:
    case eFloatID_SpellCheckerUnderlineRelativeSize:
      aResult = 1.0f; return NS_OK;
    case eFloatID_CaretAspectRatio:
      aResult = 0.05f; return NS_OK;
    default:
      aResult = -1.0f; return NS_ERROR_FAILURE;
  }
}

bool
nsLookAndFeel::GetFontImpl(FontID aID, nsString& aFontName,
                           gfxFontStyle& aFontStyle,
                           float aDevPixPerCSSPixel)
{
  aFontName.AssignLiteral("Helvetica");
  aFontStyle.style = NS_FONT_STYLE_NORMAL;
  aFontStyle.weight = NS_FONT_WEIGHT_NORMAL;
  aFontStyle.stretch = NS_FONT_STRETCH_NORMAL;
  aFontStyle.size = 12.0f * aDevPixPerCSSPixel;
  aFontStyle.systemFont = true;
  return true;
}

void
nsLookAndFeel::RefreshImpl()
{
  nsXPLookAndFeel::RefreshImpl();
}

char16_t
nsLookAndFeel::GetPasswordCharacterImpl()
{
  return char16_t(0x2022);
}

bool
nsLookAndFeel::GetEchoPasswordImpl()
{
  return false;
}
