/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef gfxMotifFontCompat_h
#define gfxMotifFontCompat_h

/*
 * Reconstructed Motif compatibility shim for UXP's toolkit-neutral
 * Fontconfig path.
 *
 * In this revision the Azure Fontconfig factory entry point and the Unix
 * gfxPlatform declaration are both hidden behind MOZ_WIDGET_GTK, even though
 * neither operation intrinsically requires GDK.  Do not define GTK globally:
 * expose just those declarations to gfxFcPlatformFontList.cpp, then restore
 * the real Motif build state before that source is parsed.
 */
#define MOZ_WIDGET_GTK 1
#include "mozilla/gfx/2D.h"
#undef MOZ_WIDGET_GTK

#define gfxPlatformGtk gfxPlatformMotif
#include "gfxPlatformGtk.h"
#undef gfxPlatformGtk

using gfxPlatformGtk = gfxPlatformMotif;

#endif /* gfxMotifFontCompat_h */
