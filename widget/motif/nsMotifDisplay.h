/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef nsMotifDisplay_h__
#define nsMotifDisplay_h__

#include <X11/Xlib.h>

/*
 * Reconstructed UXP Motif bridge: expose the toolkit-owned X Display without
 * making shared gfx headers depend on the private nsAppShell implementation.
 */
Display* nsMotifGetDisplay();

#endif /* nsMotifDisplay_h__ */
