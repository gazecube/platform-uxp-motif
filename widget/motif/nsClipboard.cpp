/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nsClipboard.h"

#include "nsIArray.h"
#include "nsISupportsPrimitives.h"
#include "nsITransferable.h"
#include "nsIClipboardOwner.h"

NS_IMPL_ISUPPORTS(nsClipboard, nsIClipboard)

nsClipboard::nsClipboard() = default;
nsClipboard::~nsClipboard()
{
  EmptyClipboard(kSelectionClipboard);
  EmptyClipboard(kGlobalClipboard);
}

nsresult
nsClipboard::Init()
{
  return NS_OK;
}

nsITransferable*
nsClipboard::TransferableFor(int32_t aWhichClipboard) const
{
  return aWhichClipboard == kSelectionClipboard
         ? mSelectionTransferable.get()
         : aWhichClipboard == kGlobalClipboard
           ? mGlobalTransferable.get() : nullptr;
}

nsIClipboardOwner*
nsClipboard::OwnerFor(int32_t aWhichClipboard) const
{
  return aWhichClipboard == kSelectionClipboard
         ? mSelectionOwner.get()
         : aWhichClipboard == kGlobalClipboard
           ? mGlobalOwner.get() : nullptr;
}

void
nsClipboard::SetSlot(int32_t aWhichClipboard,
                     nsITransferable* aTransferable,
                     nsIClipboardOwner* aOwner)
{
  if (aWhichClipboard == kSelectionClipboard) {
    mSelectionTransferable = aTransferable;
    mSelectionOwner = aOwner;
  } else if (aWhichClipboard == kGlobalClipboard) {
    mGlobalTransferable = aTransferable;
    mGlobalOwner = aOwner;
  }
}

NS_IMETHODIMP
nsClipboard::SetData(nsITransferable* aTransferable,
                     nsIClipboardOwner* aOwner,
                     int32_t aWhichClipboard)
{
  if (!aTransferable ||
      (aWhichClipboard != kSelectionClipboard &&
       aWhichClipboard != kGlobalClipboard)) {
    return NS_ERROR_INVALID_ARG;
  }

  nsCOMPtr<nsIClipboardOwner> oldOwner = OwnerFor(aWhichClipboard);
  nsCOMPtr<nsITransferable> oldTransferable = TransferableFor(aWhichClipboard);
  if (oldOwner && oldTransferable && oldOwner != aOwner) {
    oldOwner->LosingOwnership(oldTransferable);
  }
  SetSlot(aWhichClipboard, aTransferable, aOwner);
  return NS_OK;
}

NS_IMETHODIMP
nsClipboard::GetData(nsITransferable* aTransferable,
                     int32_t aWhichClipboard)
{
  if (!aTransferable) {
    return NS_ERROR_INVALID_ARG;
  }
  nsCOMPtr<nsITransferable> source = TransferableFor(aWhichClipboard);
  if (!source) {
    return NS_OK;
  }

  nsCOMPtr<nsIArray> flavors;
  nsresult rv = aTransferable->FlavorsTransferableCanImport(
      getter_AddRefs(flavors));
  if (NS_FAILED(rv) || !flavors) {
    return rv;
  }

  uint32_t count = 0;
  flavors->GetLength(&count);
  for (uint32_t i = 0; i < count; ++i) {
    nsCOMPtr<nsISupportsCString> flavor = do_QueryElementAt(flavors, i);
    if (!flavor) {
      continue;
    }
    nsAutoCString name;
    flavor->GetData(name);
    nsCOMPtr<nsISupports> data;
    uint32_t length = 0;
    if (NS_SUCCEEDED(source->GetTransferData(name.get(),
                                             getter_AddRefs(data),
                                             &length)) && data) {
      aTransferable->SetTransferData(name.get(), data, length);
      return NS_OK;
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsClipboard::EmptyClipboard(int32_t aWhichClipboard)
{
  if (aWhichClipboard != kSelectionClipboard &&
      aWhichClipboard != kGlobalClipboard) {
    return NS_ERROR_INVALID_ARG;
  }
  nsCOMPtr<nsIClipboardOwner> owner = OwnerFor(aWhichClipboard);
  nsCOMPtr<nsITransferable> transferable = TransferableFor(aWhichClipboard);
  SetSlot(aWhichClipboard, nullptr, nullptr);
  if (owner && transferable) {
    owner->LosingOwnership(transferable);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsClipboard::HasDataMatchingFlavors(const char** aFlavorList,
                                    uint32_t aLength,
                                    int32_t aWhichClipboard,
                                    bool* aHasType)
{
  if (!aHasType) {
    return NS_ERROR_INVALID_ARG;
  }
  *aHasType = false;
  nsCOMPtr<nsITransferable> source = TransferableFor(aWhichClipboard);
  if (!source) {
    return NS_OK;
  }

  nsCOMPtr<nsIArray> flavors;
  nsresult rv = source->FlavorsTransferableCanExport(getter_AddRefs(flavors));
  if (NS_FAILED(rv) || !flavors) {
    return rv;
  }

  uint32_t count = 0;
  flavors->GetLength(&count);
  for (uint32_t i = 0; i < count && !*aHasType; ++i) {
    nsCOMPtr<nsISupportsCString> flavor = do_QueryElementAt(flavors, i);
    if (!flavor) {
      continue;
    }
    nsAutoCString exported;
    flavor->GetData(exported);
    for (uint32_t j = 0; j < aLength; ++j) {
      if (aFlavorList[j] && exported.Equals(aFlavorList[j])) {
        *aHasType = true;
        break;
      }
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsClipboard::SupportsSelectionClipboard(bool* aResult)
{
  *aResult = true;
  return NS_OK;
}

NS_IMETHODIMP
nsClipboard::SupportsFindClipboard(bool* aResult)
{
  *aResult = false;
  return NS_OK;
}
