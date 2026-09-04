/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/*
 * UXP's gfxPlatform entry point historically names its Unix implementation
 * gfxPlatformGtk.  The Motif port deliberately does not define MOZ_WIDGET_GTK
 * globally.  gfx/thebes/moz.build renames that declaration in gfxPlatform.cpp,
 * and we make the same local rename here.  This keeps GTK out of the Motif
 * build while avoiding an invasive change to the common gfxPlatform TU.
 */
#define gfxPlatformGtk gfxPlatformMotif
#include "gfxPlatformGtk.h"
#undef gfxPlatformGtk

#include "prenv.h"
#include "nsUnicharUtils.h"
#include "nsUnicodeProperties.h"
#include "gfx2DGlue.h"
#include "gfxFcPlatformFontList.h"
#include "gfxConfig.h"
#include "gfxContext.h"
#include "gfxUserFontSet.h"
#include "gfxUtils.h"
#include "gfxFT2FontBase.h"
#include "gfxPrefs.h"
#include "gfxTextRun.h"
#include "VsyncSource.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/gfx/gfxVars.h"
#include "mozilla/Preferences.h"
#include "nsMathUtils.h"

#include "gfxImageSurface.h"
#include "gfxXlibSurface.h"
#include "cairo-xlib.h"

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrender.h>
#include <fontconfig/fontconfig.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

using namespace mozilla;
using namespace mozilla::gfx;
using namespace mozilla::unicode;

#define GFX_PREF_MAX_GENERIC_SUBSTITUTIONS \
    "gfx.font_rendering.fontconfig.max_generic_substitutions"

static Display* sMotifDisplay;
static int32_t sMotifDPI;

static Display*
MotifDisplay()
{
    if (!sMotifDisplay) {
        sMotifDisplay = XOpenDisplay(nullptr);
    }
    return sMotifDisplay;
}

gfxPlatformMotif::gfxPlatformMotif()
{
    mMaxGenericSubstitutions = UNINITIALIZED_VALUE;

    Display* dpy = MotifDisplay();
    if (dpy && XRE_IsParentProcess() &&
        Preferences::GetBool("gfx.xrender.enabled", true)) {
        int eventBase = 0;
        int errorBase = 0;
        if (XRenderQueryExtension(dpy, &eventBase, &errorBase)) {
            gfxVars::SetUseXRender(true);
        }
    }

    uint32_t canvasMask = BackendTypeBit(BackendType::CAIRO);
    uint32_t contentMask = BackendTypeBit(BackendType::CAIRO);
#ifdef USE_SKIA
    canvasMask |= BackendTypeBit(BackendType::SKIA);
    contentMask |= BackendTypeBit(BackendType::SKIA);
#endif
    InitBackendPrefs(canvasMask, BackendType::CAIRO,
                     contentMask, BackendType::CAIRO);

    mCompositorDisplay = XOpenDisplay(nullptr);
}

gfxPlatformMotif::~gfxPlatformMotif()
{
    if (mCompositorDisplay) {
        XCloseDisplay(mCompositorDisplay);
        mCompositorDisplay = nullptr;
    }
}

void
gfxPlatformMotif::FlushContentDrawing()
{
    Display* dpy = MotifDisplay();
    if (dpy) {
        XFlush(dpy);
    }
}

already_AddRefed<gfxASurface>
gfxPlatformMotif::CreateOffscreenSurface(const IntSize& aSize,
                                         gfxImageFormat aFormat)
{
    if (!Factory::AllowedSurfaceSize(aSize)) {
        return nullptr;
    }

    RefPtr<gfxASurface> surface;
    Display* dpy = MotifDisplay();
    if (dpy && gfxVars::UseXRender()) {
        Screen* screen = DefaultScreenOfDisplay(dpy);
        XRenderPictFormat* format =
            gfxXlibSurface::FindRenderFormat(dpy, aFormat);
        if (format) {
            surface = gfxXlibSurface::Create(screen, format, aSize);
        }
    }

    bool needsClear = !!surface;
    if (!surface) {
        surface = new gfxImageSurface(aSize, aFormat);
        needsClear = false;
    }

    if (surface->CairoStatus()) {
        return nullptr;
    }
    if (needsClear) {
        gfxUtils::ClearThebesSurface(surface);
    }
    return surface.forget();
}

nsresult
gfxPlatformMotif::GetFontList(nsIAtom* aLangGroup,
                              const nsACString& aGenericFamily,
                              nsTArray<nsString>& aListOfFonts)
{
    gfxPlatformFontList::PlatformFontList()->GetFontList(
        aLangGroup, aGenericFamily, aListOfFonts);
    return NS_OK;
}

nsresult
gfxPlatformMotif::UpdateFontList()
{
    gfxPlatformFontList::PlatformFontList()->UpdateFontList();
    return NS_OK;
}

static const char kFontDejaVuSans[] = "DejaVu Sans";
static const char kFontDejaVuSerif[] = "DejaVu Serif";
static const char kFontFreeSans[] = "FreeSans";
static const char kFontFreeSerif[] = "FreeSerif";
static const char kFontTwemojiMozilla[] = "Twemoji Mozilla";
static const char kFontDroidSansFallback[] = "Droid Sans Fallback";
static const char kFontWenQuanYiMicroHei[] = "WenQuanYi Micro Hei";
static const char kFontNanumGothic[] = "NanumGothic";

void
gfxPlatformMotif::GetCommonFallbackFonts(uint32_t aCh, uint32_t aNextCh,
                                         Script aRunScript,
                                         nsTArray<const char*>& aFontList)
{
    EmojiPresentation emoji = GetEmojiPresentation(aCh);
    EmojiPresentation next = GetEmojiPresentation(aNextCh, true);
    if (aNextCh != kVariationSelector15 &&
        emoji != EmojiPresentation::TextOnly &&
        (emoji != EmojiPresentation::TextDefault ||
         next == EmojiPresentation::EmojiComponent)) {
        aFontList.AppendElement(kFontTwemojiMozilla);
    }

    aFontList.AppendElement(kFontDejaVuSerif);
    aFontList.AppendElement(kFontFreeSerif);
    aFontList.AppendElement(kFontDejaVuSans);
    aFontList.AppendElement(kFontFreeSans);

    if (aCh >= 0x3000 &&
        ((aCh < 0xe000) ||
         (aCh >= 0xf900 && aCh < 0xfff0) ||
         ((aCh >> 16) == 2))) {
        aFontList.AppendElement(kFontDroidSansFallback);
        aFontList.AppendElement(kFontWenQuanYiMicroHei);
        aFontList.AppendElement(kFontNanumGothic);
    }
}

gfxPlatformFontList*
gfxPlatformMotif::CreatePlatformFontList()
{
    gfxPlatformFontList* list = new gfxFcPlatformFontList();
    if (NS_SUCCEEDED(list->InitFontList())) {
        return list;
    }
    gfxPlatformFontList::Shutdown();
    return nullptr;
}

nsresult
gfxPlatformMotif::GetStandardFamilyName(const nsAString& aFontName,
                                        nsAString& aFamilyName)
{
    gfxPlatformFontList::PlatformFontList()->GetStandardFamilyName(
        aFontName, aFamilyName);
    return NS_OK;
}

gfxFontGroup*
gfxPlatformMotif::CreateFontGroup(const FontFamilyList& aFontFamilyList,
                                  const gfxFontStyle* aStyle,
                                  gfxTextPerfMetrics* aTextPerf,
                                  gfxUserFontSet* aUserFontSet,
                                  gfxFloat aDevToCssSize)
{
    return new gfxFontGroup(aFontFamilyList, aStyle, aTextPerf,
                            aUserFontSet, aDevToCssSize);
}

gfxFontEntry*
gfxPlatformMotif::LookupLocalFont(const nsAString& aFontName,
                                  uint16_t aWeight,
                                  int16_t aStretch,
                                  uint8_t aStyle)
{
    return gfxPlatformFontList::PlatformFontList()->LookupLocalFont(
        aFontName, aWeight, aStretch, aStyle);
}

gfxFontEntry*
gfxPlatformMotif::MakePlatformFont(const nsAString& aFontName,
                                   uint16_t aWeight,
                                   int16_t aStretch,
                                   uint8_t aStyle,
                                   const uint8_t* aFontData,
                                   uint32_t aLength)
{
    return gfxPlatformFontList::PlatformFontList()->MakePlatformFont(
        aFontName, aWeight, aStretch, aStyle, aFontData, aLength);
}

bool
gfxPlatformMotif::IsFontFormatSupported(nsIURI* aFontURI,
                                        uint32_t aFormatFlags)
{
    if (aFormatFlags & gfxUserFontSet::FLAG_FORMATS_COMMON) {
        return true;
    }
    return aFormatFlags == 0;
}

int32_t
gfxPlatformMotif::GetDPI()
{
    if (sMotifDPI > 0) {
        return sMotifDPI;
    }

    Display* dpy = MotifDisplay();
    if (!dpy) {
        sMotifDPI = 96;
        return sMotifDPI;
    }

    int screen = DefaultScreen(dpy);
    int mm = DisplayWidthMM(dpy, screen);
    int px = DisplayWidth(dpy, screen);
    if (mm > 0) {
        sMotifDPI = int32_t(lround(double(px) * 25.4 / double(mm)));
    }
    if (sMotifDPI < 50 || sMotifDPI > 500) {
        sMotifDPI = 96;
    }
    return sMotifDPI;
}

double
gfxPlatformMotif::GetDPIScale()
{
    return double(GetDPI()) / 96.0;
}

bool
gfxPlatformMotif::UseImageOffscreenSurfaces()
{
    return GetDefaultContentBackend() != BackendType::CAIRO ||
           gfxPrefs::UseImageOffscreenSurfaces();
}

gfxImageFormat
gfxPlatformMotif::GetOffscreenFormat()
{
    Display* dpy = MotifDisplay();
    if (dpy && DefaultDepth(dpy, DefaultScreen(dpy)) == 16) {
        return SurfaceFormat::R5G6B5_UINT16;
    }
    return SurfaceFormat::X8R8G8B8_UINT32;
}

bool
gfxPlatformMotif::SupportsApzTouchInput() const
{
    return false;
}

void
gfxPlatformMotif::FontsPrefsChanged(const char* aPref)
{
    if (strcmp(GFX_PREF_MAX_GENERIC_SUBSTITUTIONS, aPref)) {
        gfxPlatform::FontsPrefsChanged(aPref);
        return;
    }

    mMaxGenericSubstitutions = UNINITIALIZED_VALUE;
    gfxFcPlatformFontList* pfl = gfxFcPlatformFontList::PlatformFontList();
    pfl->ClearGenericMappings();
    FlushFontAndWordCaches();
}

uint32_t
gfxPlatformMotif::MaxGenericSubstitions()
{
    if (mMaxGenericSubstitutions == UNINITIALIZED_VALUE) {
        mMaxGenericSubstitutions =
            Preferences::GetInt(GFX_PREF_MAX_GENERIC_SUBSTITUTIONS, 3);
        if (mMaxGenericSubstitutions < 0) {
            mMaxGenericSubstitutions = 3;
        }
    }
    return uint32_t(mMaxGenericSubstitutions);
}

void
gfxPlatformMotif::GetPlatformCMSOutputProfile(void*& aMem, size_t& aSize)
{
    aMem = nullptr;
    aSize = 0;

    Display* dpy = MotifDisplay();
    if (!dpy) {
        return;
    }

    Atom profile = XInternAtom(dpy, "_ICC_PROFILE", True);
    if (profile == None) {
        return;
    }

    Atom actualType = None;
    int actualFormat = 0;
    unsigned long nitems = 0;
    unsigned long bytesAfter = 0;
    unsigned char* property = nullptr;

    if (XGetWindowProperty(dpy, DefaultRootWindow(dpy), profile,
                           0, INT_MAX, False, AnyPropertyType,
                           &actualType, &actualFormat, &nitems,
                           &bytesAfter, &property) != Success || !property) {
        return;
    }

    if (actualFormat == 8 && nitems) {
        void* data = malloc(nitems);
        if (data) {
            memcpy(data, property, nitems);
            aMem = data;
            aSize = nitems;
        }
    }
    XFree(property);
}

#ifdef GL_PROVIDER_GLX
already_AddRefed<mozilla::gfx::VsyncSource>
gfxPlatformMotif::CreateHardwareVsyncSource()
{
    return gfxPlatform::CreateHardwareVsyncSource();
}
#endif
