/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/ModuleUtils.h"
#include "mozilla/WidgetUtils.h"
#include "nsWidgetsCID.h"
#include "nsAppShell.h"
#include "nsAppShellSingleton.h"
#include "nsWindow.h"
#include "nsLookAndFeel.h"
#include "nsBidiKeyboard.h"
#include "nsClipboard.h"
#include "nsClipboardHelper.h"
#include "nsScreenManagerMotif.h"
#include "nsTransferable.h"
#include "nsHTMLFormatConverter.h"
#include "nsComponentManagerUtils.h"
#include "nsIFilePicker.h"

#if defined(MOZ_X11)
#include "GfxInfoX11.h"
#endif

using namespace mozilla;
using namespace mozilla::widget;

/* The XUL picker is kept as the fallback while the native XmFileSelectionBox
 * bridge is deliberately isolated from initial browser bring-up. */
#define XULFILEPICKER_CID \
  { 0x54ae32f8, 0x1dd2, 0x11b2, \
    { 0xa2, 0x09, 0xdf, 0x7c, 0x50, 0x53, 0x70, 0xf8 } }
static NS_DEFINE_CID(kXULFilePickerCID, XULFILEPICKER_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR(nsWindow)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsTransferable)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsHTMLFormatConverter)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsBidiKeyboard)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsScreenManagerMotif)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsClipboardHelper)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsClipboard, Init)

#if defined(MOZ_X11)
namespace mozilla {
namespace widget {
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(GfxInfo, Init)
}
}
#endif

static nsresult
nsFilePickerConstructor(nsISupports* aOuter, REFNSIID aIID, void** aResult)
{
  if (aOuter) {
    return NS_ERROR_NO_AGGREGATION;
  }
  *aResult = nullptr;
  nsCOMPtr<nsIFilePicker> picker = do_CreateInstance(kXULFilePickerCID);
  if (!picker) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return picker->QueryInterface(aIID, aResult);
}

NS_DEFINE_NAMED_CID(NS_WINDOW_CID);
NS_DEFINE_NAMED_CID(NS_CHILD_CID);
NS_DEFINE_NAMED_CID(NS_APPSHELL_CID);
NS_DEFINE_NAMED_CID(NS_FILEPICKER_CID);
NS_DEFINE_NAMED_CID(NS_TRANSFERABLE_CID);
NS_DEFINE_NAMED_CID(NS_CLIPBOARD_CID);
NS_DEFINE_NAMED_CID(NS_CLIPBOARDHELPER_CID);
NS_DEFINE_NAMED_CID(NS_HTMLFORMATCONVERTER_CID);
NS_DEFINE_NAMED_CID(NS_BIDIKEYBOARD_CID);
NS_DEFINE_NAMED_CID(NS_SCREENMANAGER_CID);
#if defined(MOZ_X11)
NS_DEFINE_NAMED_CID(NS_GFXINFO_CID);
#endif

static const Module::CIDEntry kWidgetCIDs[] = {
  { &kNS_WINDOW_CID, false, nullptr, nsWindowConstructor },
  { &kNS_CHILD_CID, false, nullptr, nsWindowConstructor },
  { &kNS_APPSHELL_CID, false, nullptr, nsAppShellConstructor,
    Module::ALLOW_IN_GPU_PROCESS },
  { &kNS_FILEPICKER_CID, false, nullptr, nsFilePickerConstructor,
    Module::MAIN_PROCESS_ONLY },
  { &kNS_TRANSFERABLE_CID, false, nullptr, nsTransferableConstructor },
  { &kNS_CLIPBOARD_CID, false, nullptr, nsClipboardConstructor,
    Module::MAIN_PROCESS_ONLY },
  { &kNS_CLIPBOARDHELPER_CID, false, nullptr, nsClipboardHelperConstructor },
  { &kNS_HTMLFORMATCONVERTER_CID, false, nullptr,
    nsHTMLFormatConverterConstructor },
  { &kNS_BIDIKEYBOARD_CID, false, nullptr, nsBidiKeyboardConstructor,
    Module::MAIN_PROCESS_ONLY },
  { &kNS_SCREENMANAGER_CID, false, nullptr, nsScreenManagerMotifConstructor,
    Module::MAIN_PROCESS_ONLY },
#if defined(MOZ_X11)
  { &kNS_GFXINFO_CID, false, nullptr, mozilla::widget::GfxInfoConstructor },
#endif
  { nullptr }
};

static const Module::ContractIDEntry kWidgetContracts[] = {
  { "@mozilla.org/widget/window/motif;1", &kNS_WINDOW_CID },
  { "@mozilla.org/widgets/child_window/motif;1", &kNS_CHILD_CID },
  { "@mozilla.org/widget/appshell/motif;1", &kNS_APPSHELL_CID,
    Module::ALLOW_IN_GPU_PROCESS },
  { "@mozilla.org/filepicker;1", &kNS_FILEPICKER_CID,
    Module::MAIN_PROCESS_ONLY },
  { "@mozilla.org/widget/transferable;1", &kNS_TRANSFERABLE_CID },
  { "@mozilla.org/widget/clipboard;1", &kNS_CLIPBOARD_CID,
    Module::MAIN_PROCESS_ONLY },
  { "@mozilla.org/widget/clipboardhelper;1", &kNS_CLIPBOARDHELPER_CID },
  { "@mozilla.org/widget/htmlformatconverter;1", &kNS_HTMLFORMATCONVERTER_CID },
  { "@mozilla.org/widget/bidikeyboard;1", &kNS_BIDIKEYBOARD_CID,
    Module::MAIN_PROCESS_ONLY },
  { "@mozilla.org/gfx/screenmanager;1", &kNS_SCREENMANAGER_CID,
    Module::MAIN_PROCESS_ONLY },
#if defined(MOZ_X11)
  { "@mozilla.org/gfx/info;1", &kNS_GFXINFO_CID },
#endif
  { nullptr }
};

static void
nsWidgetMotifModuleDtor()
{
  WidgetUtils::Shutdown();
  nsLookAndFeel::Shutdown();
  nsAppShellShutdown();
}

static const Module kWidgetModule = {
  Module::kVersion,
  kWidgetCIDs,
  kWidgetContracts,
  nullptr,
  nullptr,
  nsAppShellInit,
  nsWidgetMotifModuleDtor,
  Module::ALLOW_IN_GPU_PROCESS
};

NSMODULE_DEFN(nsWidgetMotifModule) = &kWidgetModule;
