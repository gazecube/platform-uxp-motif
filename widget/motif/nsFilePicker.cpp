/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nsFilePicker.h"
#include "nsAppShell.h"

#include "nsArrayEnumerator.h"
#include "nsCOMArray.h"
#include "nsIFile.h"
#include "nsIFileURL.h"
#include "nsIURI.h"
#include "nsIWidget.h"
#include "nsNetUtil.h"
#include "nsReadableUtils.h"

#include <Xm/FileSB.h>
#include <Xm/Xm.h>

NS_IMPL_ISUPPORTS(nsFilePicker, nsIFilePicker)

nsFilePicker::nsFilePicker()
  : mSelectedType(0)
  , mRunning(false)
  , mResult(nsIFilePicker::returnCancel)
{
}

nsFilePicker::~nsFilePicker() = default;

void
nsFilePicker::InitNative(nsIWidget* aParent, const nsAString& aTitle)
{
  mParentWidget = aParent;
  mTitle = aTitle;
}

NS_IMETHODIMP
nsFilePicker::AppendFilter(const nsAString& aTitle, const nsAString& aFilter)
{
  if (aFilter.EqualsLiteral("..apps")) {
    return NS_OK;
  }
  nsCString title;
  nsCString filter;
  CopyUTF16toUTF8(aTitle, title);
  CopyUTF16toUTF8(aFilter, filter);
  mFilterNames.AppendElement(title);
  mFilters.AppendElement(filter);
  return NS_OK;
}

NS_IMETHODIMP
nsFilePicker::SetDefaultString(const nsAString& aString)
{
  mDefault = aString;
  return NS_OK;
}

NS_IMETHODIMP
nsFilePicker::GetDefaultString(nsAString& aString)
{
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsFilePicker::SetDefaultExtension(const nsAString& aExtension)
{
  mDefaultExtension = aExtension;
  return NS_OK;
}

NS_IMETHODIMP
nsFilePicker::GetDefaultExtension(nsAString& aExtension)
{
  aExtension = mDefaultExtension;
  return NS_OK;
}

NS_IMETHODIMP
nsFilePicker::GetFilterIndex(int32_t* aFilterIndex)
{
  NS_ENSURE_ARG_POINTER(aFilterIndex);
  *aFilterIndex = mSelectedType;
  return NS_OK;
}

NS_IMETHODIMP
nsFilePicker::SetFilterIndex(int32_t aFilterIndex)
{
  mSelectedType = aFilterIndex;
  return NS_OK;
}

NS_IMETHODIMP
nsFilePicker::GetFile(nsIFile** aFile)
{
  NS_ENSURE_ARG_POINTER(aFile);
  *aFile = nullptr;
  if (mSelectedPath.IsEmpty()) {
    return NS_OK;
  }
  return NS_NewNativeLocalFile(mSelectedPath, false, aFile);
}

NS_IMETHODIMP
nsFilePicker::GetFileURL(nsIURI** aFileURL)
{
  NS_ENSURE_ARG_POINTER(aFileURL);
  *aFileURL = nullptr;
  nsCOMPtr<nsIFile> file;
  nsresult rv = GetFile(getter_AddRefs(file));
  NS_ENSURE_SUCCESS(rv, rv);
  if (!file) {
    return NS_OK;
  }
  return NS_NewFileURI(aFileURL, file);
}

NS_IMETHODIMP
nsFilePicker::GetFiles(nsISimpleEnumerator** aFiles)
{
  NS_ENSURE_ARG_POINTER(aFiles);
  nsCOMArray<nsIFile> files;
  nsCOMPtr<nsIFile> file;
  nsresult rv = GetFile(getter_AddRefs(file));
  NS_ENSURE_SUCCESS(rv, rv);
  if (file) {
    files.AppendObject(file);
  }
  return NS_NewArrayEnumerator(aFiles, files);
}

nsCString
nsFilePicker::CurrentPattern() const
{
  if (mSelectedType < 0 ||
      uint32_t(mSelectedType) >= mFilters.Length()) {
    return NS_LITERAL_CSTRING("*");
  }

  nsCString pattern(mFilters[mSelectedType]);
  // XmFileSelectionBox has a single shell-style pattern.  Mozilla filter
  // strings may contain a semicolon-separated list; use the first member for
  // the native Motif field rather than silently depending on GTK semantics.
  int32_t semi = pattern.FindChar(';');
  if (semi >= 0) {
    pattern.Truncate(semi);
  }
  pattern.Trim(" \t");
  if (pattern.IsEmpty() || pattern.EqualsLiteral("*.*")) {
    pattern.AssignLiteral("*");
  }
  return pattern;
}

/* static */ void
nsFilePicker::OkCallback(Widget aWidget, XtPointer aClosure,
                         XtPointer aCallData)
{
  static_cast<nsFilePicker*>(aClosure)->FinishFromCallback(true, aCallData);
}

/* static */ void
nsFilePicker::CancelCallback(Widget aWidget, XtPointer aClosure,
                             XtPointer aCallData)
{
  static_cast<nsFilePicker*>(aClosure)->FinishFromCallback(false, aCallData);
}

void
nsFilePicker::FinishFromCallback(bool aAccepted, XtPointer aCallData)
{
  mResult = nsIFilePicker::returnCancel;

  if (aAccepted && aCallData) {
    XmFileSelectionBoxCallbackStruct* info =
      static_cast<XmFileSelectionBoxCallbackStruct*>(aCallData);
    char* value = nullptr;
    if (info->value &&
        XmStringGetLtoR(info->value, XmFONTLIST_DEFAULT_TAG, &value) && value) {
      mSelectedPath.Assign(value);
      XtFree(value);
      mResult = nsIFilePicker::returnOK;
    }
  }

  mRunning = false;
}

NS_IMETHODIMP
nsFilePicker::Show(int16_t* aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  *aReturn = nsIFilePicker::returnCancel;

  if (mRunning) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  Widget parent = nullptr;
  if (mParentWidget) {
    parent = static_cast<Widget>(
      mParentWidget->GetNativeData(NS_NATIVE_SHELLWIDGET));
    if (!parent) {
      parent = static_cast<Widget>(
        mParentWidget->GetNativeData(NS_NATIVE_WIDGET));
    }
  }
  if (!parent) {
    return NS_ERROR_FAILURE;
  }

  Widget dialog = XmCreateFileSelectionDialog(
      parent, const_cast<char*>("fileSelection"), nullptr, 0);
  if (!dialog) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ConvertUTF16toUTF8 title(mTitle);
  XmString titleXm = XmStringCreateLocalized(
      const_cast<char*>(title.IsEmpty() ? "Select File" : title.get()));
  XtVaSetValues(dialog, XmNdialogTitle, titleXm, nullptr);
  XmStringFree(titleXm);

  nsCString pattern = CurrentPattern();
  XmString patternXm = XmStringCreateLocalized(
      const_cast<char*>(pattern.get()));
  XtVaSetValues(dialog, XmNpattern, patternXm, nullptr);
  XmStringFree(patternXm);

  if (mDisplayDirectory) {
    nsAutoCString nativePath;
    if (NS_SUCCEEDED(mDisplayDirectory->GetNativePath(nativePath)) &&
        !nativePath.IsEmpty()) {
      XmString dirXm = XmStringCreateLocalized(
          const_cast<char*>(nativePath.get()));
      XtVaSetValues(dialog, XmNdirectory, dirXm, nullptr);
      XmStringFree(dirXm);
    }
  }

  if (mMode == nsIFilePicker::modeSave && !mDefault.IsEmpty()) {
    NS_ConvertUTF16toUTF8 defaultName(mDefault);
    XmString nameXm = XmStringCreateLocalized(
        const_cast<char*>(defaultName.get()));
    XtVaSetValues(dialog, XmNdirSpec, nameXm, nullptr);
    XmStringFree(nameXm);
  }

  if (!mOkButtonLabel.IsEmpty()) {
    NS_ConvertUTF16toUTF8 okLabel(mOkButtonLabel);
    XmString okXm = XmStringCreateLocalized(
        const_cast<char*>(okLabel.get()));
    XtVaSetValues(dialog, XmNokLabelString, okXm, nullptr);
    XmStringFree(okXm);
  }

  XtAddCallback(dialog, XmNokCallback, OkCallback, this);
  XtAddCallback(dialog, XmNcancelCallback, CancelCallback, this);

  // The historical Mozilla Motif backend ran this dialog with a local Xt
  // modal loop.  Preserve that behavior so the deprecated synchronous Show()
  // contract remains correct; nsBaseFilePicker::Open wraps Show asynchronously
  // on the main thread for modern callers.
  mSelectedPath.Truncate();
  mResult = nsIFilePicker::returnCancel;
  mRunning = true;
  XtManageChild(dialog);

  XtAppContext app = nsAppShell::GetAppContext();
  while (mRunning) {
    XEvent event;
    XtAppNextEvent(app, &event);
    XtDispatchEvent(&event);
  }

  XtUnmanageChild(dialog);
  XtDestroyWidget(dialog);

  if (mResult == nsIFilePicker::returnOK && !mSelectedPath.IsEmpty()) {
    nsCOMPtr<nsIFile> file;
    if (NS_SUCCEEDED(GetFile(getter_AddRefs(file))) && file) {
      nsCOMPtr<nsIFile> dir;
      if (mMode == nsIFilePicker::modeGetFolder) {
        dir = file;
      } else {
        file->GetParent(getter_AddRefs(dir));
      }
      if (dir) {
        mDisplayDirectory = dir;
      }
    }
  }

  *aReturn = mResult;
  return NS_OK;
}
