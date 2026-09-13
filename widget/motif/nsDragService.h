/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef nsDragService_h__
#define nsDragService_h__

#include "nsBaseDragService.h"

class nsDragService final : public nsBaseDragService
{
public:
  nsDragService() = default;

protected:
  ~nsDragService() override = default;

  nsresult InvokeDragSessionImpl(nsIArray* aTransferableArray,
                                 nsIScriptableRegion* aDragRgn,
                                 uint32_t aActionType) override;
};

#endif // nsDragService_h__
