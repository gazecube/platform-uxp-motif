/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef nsBidiKeyboard_h__
#define nsBidiKeyboard_h__

#include "nsIBidiKeyboard.h"

class nsBidiKeyboard final : public nsIBidiKeyboard
{
public:
  nsBidiKeyboard();
  NS_DECL_ISUPPORTS
  NS_DECL_NSIBIDIKEYBOARD
private:
  ~nsBidiKeyboard() override = default;
};

#endif
