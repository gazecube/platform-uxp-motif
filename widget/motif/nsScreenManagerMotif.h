/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef nsScreenManagerMotif_h__
#define nsScreenManagerMotif_h__

#include "nsIScreenManager.h"
#include "nsCOMArray.h"

class nsScreenManagerMotif final : public nsIScreenManager
{
public:
  nsScreenManagerMotif();
  NS_DECL_ISUPPORTS
  NS_DECL_NSISCREENMANAGER

private:
  ~nsScreenManagerMotif() override = default;
  nsresult EnsureInit();
  nsCOMArray<nsIScreen> mScreens;
};

#endif
