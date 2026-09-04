/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef nsClipboard_h__
#define nsClipboard_h__

#include "nsIClipboard.h"
#include "nsCOMPtr.h"

class nsClipboard final : public nsIClipboard
{
public:
  nsClipboard();
  NS_DECL_ISUPPORTS
  NS_DECL_NSICLIPBOARD
  nsresult Init();

private:
  ~nsClipboard() override;
  nsITransferable* TransferableFor(int32_t aWhichClipboard) const;
  nsIClipboardOwner* OwnerFor(int32_t aWhichClipboard) const;
  void SetSlot(int32_t aWhichClipboard, nsITransferable* aTransferable,
               nsIClipboardOwner* aOwner);

  nsCOMPtr<nsITransferable> mSelectionTransferable;
  nsCOMPtr<nsITransferable> mGlobalTransferable;
  nsCOMPtr<nsIClipboardOwner> mSelectionOwner;
  nsCOMPtr<nsIClipboardOwner> mGlobalOwner;
};

#endif
