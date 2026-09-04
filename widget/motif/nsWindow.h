/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef nsWindow_h__
#define nsWindow_h__

#include "nsBaseWidget.h"
#include <X11/Intrinsic.h>

class nsWindow final : public nsBaseWidget
{
public:
  nsWindow();
  NS_DECL_ISUPPORTS_INHERITED

  using nsBaseWidget::Create;
  [[nodiscard]] nsresult Create(nsIWidget* aParent,
                                nsNativeWidget aNativeParent,
                                const LayoutDeviceIntRect& aRect,
                                nsWidgetInitData* aInitData) override;
  void Destroy() override;

  nsresult DispatchEvent(mozilla::WidgetGUIEvent* aEvent,
                         nsEventStatus& aStatus) override;
  void OnDestroy() override;

  NS_IMETHOD Show(bool aState) override;
  NS_IMETHOD Move(double aX, double aY) override;
  NS_IMETHOD Resize(double aWidth, double aHeight, bool aRepaint) override;
  NS_IMETHOD Resize(double aX, double aY, double aWidth, double aHeight,
                    bool aRepaint) override;
  NS_IMETHOD Enable(bool aState) override;
  bool IsEnabled() const override { return mEnabled; }
  bool IsVisible() const override { return mShown; }
  NS_IMETHOD SetFocus(bool aRaise = false) override;

  NS_IMETHOD SetTitle(const nsAString& aTitle) override;
  NS_IMETHOD SetCursor(nsCursor aCursor) override;
  NS_IMETHOD SetCursor(imgIContainer* aCursor, uint32_t aHotspotX,
                       uint32_t aHotspotY) override;
  NS_IMETHOD Invalidate(const LayoutDeviceIntRect& aRect) override;

  void* GetNativeData(uint32_t aDataType) override;
  LayoutDeviceIntPoint WidgetToScreenOffset() override;
  LayoutDeviceIntRect GetScreenBounds() override;
  LayoutDeviceIntRect GetClientBounds() override;
  LayoutDeviceIntSize GetClientSize() override;
  LayoutDeviceIntPoint GetClientOffset() override;
  float GetDPI() override;
  double GetDefaultScaleInternal() override;
  mozilla::DesktopToLayoutDeviceScale GetDesktopToDeviceScale() final
  {
    return mozilla::DesktopToLayoutDeviceScale(1.0);
  }

  bool HasPendingInputEvent() override;
  void SetModal(bool aModal) override;
  void SetSizeConstraints(const SizeConstraints& aConstraints) override;
  void CaptureMouse(bool aCapture) override;
  void CaptureRollupEvents(nsIRollupListener* aListener, bool aDoCapture) override;

  already_AddRefed<mozilla::gfx::DrawTarget>
  StartRemoteDrawingInRegion(LayoutDeviceIntRegion& aInvalidRegion,
                             mozilla::layers::BufferMode* aBufferMode) override;
  void EndRemoteDrawingInRegion(mozilla::gfx::DrawTarget* aDrawTarget,
                                LayoutDeviceIntRegion& aInvalidRegion) override;

  void GetCompositorWidgetInitData(
      mozilla::widget::CompositorWidgetInitData* aInitData) override;

  Widget XtWidget() const { return mWidget; }
  Window XWindow() const;
  Display* XDisplay() const;

private:
  ~nsWindow() override;

  static void XtEventHandler(Widget aWidget, XtPointer aClosure,
                             XEvent* aEvent, Boolean* aContinue);
  static void XtDestroyCallback(Widget aWidget, XtPointer aClosure,
                                XtPointer aCallData);
  void HandleXEvent(XEvent* aEvent);
  void Paint(const XExposeEvent& aEvent);
  void DispatchMouse(const XEvent& aEvent);
  void DispatchKey(const XKeyEvent& aEvent, bool aDown);
  void DispatchResized();
  void UpdateWMProtocols();

  Widget mWidget;
  Display* mDisplay;
  LayoutDeviceIntRect mBounds;
  bool mCreated;
  bool mDestroyed;
  bool mShown;
  bool mEnabled;
  bool mTopLevel;
  bool mModal;
  Cursor mNativeCursor;
};

#endif /* nsWindow_h__ */
