/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef widget_motif_X11CompositorWidget_h
#define widget_motif_X11CompositorWidget_h

#include "mozilla/widget/CompositorWidget.h"
#include "nsIWidget.h"
#include <X11/Xlib.h>

#ifdef MOZ_WIDGET_MOTIF
/*
 * Shared GLX code historically names these through the GTK X11 backend.
 * Motif exposes the same native X handles through nsIWidget directly.
 */
#ifndef NS_NATIVE_COMPOSITOR_DISPLAY
#define NS_NATIVE_COMPOSITOR_DISPLAY NS_NATIVE_DISPLAY
#endif
#ifndef GET_NATIVE_WINDOW
#define GET_NATIVE_WINDOW(aWidget) \
  static_cast<Window>(reinterpret_cast<uintptr_t>( \
    (aWidget)->GetNativeData(NS_NATIVE_WINDOW)))
#endif
#endif

namespace mozilla {
namespace widget {

/*
 * Motif bootstrap adapter for the generic in-process compositor.
 *
 * Unlike GTK's X11CompositorWidget this does not implement an OOP surface
 * provider.  It only exposes the native X Display and Window already owned by
 * the Motif nsWindow so GLX can create a context for the in-process widget.
 */
class X11CompositorWidget : public CompositorWidget
{
public:
  X11CompositorWidget* AsX11() override { return this; }

  Display* XDisplay() const
  {
    nsIWidget* widget = const_cast<X11CompositorWidget*>(this)->RealWidget();
    return widget
      ? static_cast<Display*>(widget->GetNativeData(NS_NATIVE_DISPLAY))
      : nullptr;
  }

  Window XWindow() const
  {
    nsIWidget* widget = const_cast<X11CompositorWidget*>(this)->RealWidget();
    return widget
      ? static_cast<Window>(reinterpret_cast<uintptr_t>(
          widget->GetNativeData(NS_NATIVE_WINDOW)))
      : static_cast<Window>(0);
  }

protected:
  ~X11CompositorWidget() override = default;
};

} // namespace widget
} // namespace mozilla

#endif // widget_motif_X11CompositorWidget_h
