/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nsBidiKeyboard.h"

NS_IMPL_ISUPPORTS(nsBidiKeyboard, nsIBidiKeyboard)

nsBidiKeyboard::nsBidiKeyboard() = default;

NS_IMETHODIMP
nsBidiKeyboard::Reset()
{
  return NS_OK;
}

NS_IMETHODIMP
nsBidiKeyboard::IsLangRTL(bool* aIsRTL)
{
  if (!aIsRTL) {
    return NS_ERROR_INVALID_ARG;
  }
  *aIsRTL = false;
  return NS_OK;
}

NS_IMETHODIMP
nsBidiKeyboard::GetHaveBidiKeyboards(bool* aResult)
{
  if (!aResult) {
    return NS_ERROR_INVALID_ARG;
  }
  *aResult = false;
  return NS_OK;
}
