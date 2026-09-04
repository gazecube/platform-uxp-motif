/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef nsFilePicker_h__
#define nsFilePicker_h__

#include "nsBaseFilePicker.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsTArray.h"

#include <X11/Intrinsic.h>

class nsIWidget;

class nsFilePicker final : public nsBaseFilePicker
{
public:
  nsFilePicker();

  NS_DECL_ISUPPORTS

  NS_IMETHOD AppendFilter(const nsAString& aTitle,
                          const nsAString& aFilter) override;
  NS_IMETHOD SetDefaultString(const nsAString& aString) override;
  NS_IMETHOD GetDefaultString(nsAString& aString) override;
  NS_IMETHOD SetDefaultExtension(const nsAString& aExtension) override;
  NS_IMETHOD GetDefaultExtension(nsAString& aExtension) override;
  NS_IMETHOD GetFilterIndex(int32_t* aFilterIndex) override;
  NS_IMETHOD SetFilterIndex(int32_t aFilterIndex) override;
  NS_IMETHOD GetFile(nsIFile** aFile) override;
  NS_IMETHOD GetFileURL(nsIURI** aFileURL) override;
  NS_IMETHOD GetFiles(nsISimpleEnumerator** aFiles) override;
  NS_IMETHOD Show(int16_t* aReturn) override;

  void InitNative(nsIWidget* aParent, const nsAString& aTitle) override;

private:
  ~nsFilePicker() override;

  static void OkCallback(Widget aWidget, XtPointer aClosure,
                         XtPointer aCallData);
  static void CancelCallback(Widget aWidget, XtPointer aClosure,
                             XtPointer aCallData);

  void FinishFromCallback(bool aAccepted, XtPointer aCallData);
  nsCString CurrentPattern() const;

  nsCOMPtr<nsIWidget> mParentWidget;
  nsString mTitle;
  nsString mDefault;
  nsString mDefaultExtension;
  nsCString mSelectedPath;
  nsTArray<nsCString> mFilters;
  nsTArray<nsCString> mFilterNames;
  int32_t mSelectedType;
  bool mRunning;
  int16_t mResult;
};

#endif
