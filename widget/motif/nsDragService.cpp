/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nsDragService.h"

nsresult
nsDragService::InvokeDragSessionImpl(nsIArray* aTransferableArray,
                                     nsIScriptableRegion* aDragRgn,
                                     uint32_t aActionType)
{
  // Motif drag-and-drop transport is not wired yet. Providing the widget
  // drag service still lets callers query the current drag session without
  // treating the service itself as missing. Native drag initiation remains
  // explicitly unsupported until the Xm drag bridge is implemented.
  return NS_ERROR_NOT_IMPLEMENTED;
}
