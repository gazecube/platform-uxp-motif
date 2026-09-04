/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nsWindow.h"
#include "nsAppShell.h"

#include "gfxContext.h"
#include "gfxPlatform.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/MouseEvents.h"
#include "mozilla/TextEvents.h"
#include "mozilla/widget/CompositorWidget.h"
#include "nsIWidgetListener.h"
#include "nsIRollupListener.h"
#include "nsMathUtils.h"
#include "nsXULAppAPI.h"

#include <Xm/DrawingA.h>
#include <Xm/Xm.h>
#include <X11/Shell.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <cairo-xlib.h>

using namespace mozilla;
using namespace mozilla::gfx;
using namespace mozilla::layers;

NS_IMPL_ISUPPORTS_INHERITED0(nsWindow, nsBaseWidget)

static int16_t
ButtonForX(unsigned int aButton)
{
  switch (aButton) {
    case Button1: return WidgetMouseEvent::eLeftButton;
    case Button2: return WidgetMouseEvent::eMiddleButton;
    case Button3: return WidgetMouseEvent::eRightButton;
    default: return WidgetMouseEvent::eLeftButton;
  }
}

nsWindow::nsWindow()
  : mWidget(nullptr)
  , mDisplay(nullptr)
  , mCreated(false)
  , mDestroyed(false)
  , mShown(false)
  , mEnabled(true)
  , mTopLevel(false)
  , mModal(false)
  , mNativeCursor(None)
{
  mWindowType = eWindowType_child;
}

nsWindow::~nsWindow()
{
  Destroy();
}

nsresult
nsWindow::Create(nsIWidget* aParent,
                 nsNativeWidget aNativeParent,
                 const LayoutDeviceIntRect& aRect,
                 nsWidgetInitData* aInitData)
{
  BaseCreate(aParent, aInitData);
  mBounds = aRect;
  mTopLevel = (mWindowType == eWindowType_toplevel ||
               mWindowType == eWindowType_dialog ||
               mWindowType == eWindowType_popup);

  XtAppContext app = nsAppShell::GetAppContext();
  mDisplay = nsAppShell::GetDisplay();
  if (!app || !mDisplay) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  Widget parentWidget = nullptr;
  if (aParent) {
    parentWidget = static_cast<nsWindow*>(aParent)->XtWidget();
  } else if (aNativeParent) {
    parentWidget = reinterpret_cast<Widget>(aNativeParent);
  }

  Arg args[16];
  Cardinal n = 0;
  XtSetArg(args[n], XmNx, mBounds.x); ++n;
  XtSetArg(args[n], XmNy, mBounds.y); ++n;
  XtSetArg(args[n], XmNwidth, std::max(1, mBounds.width)); ++n;
  XtSetArg(args[n], XmNheight, std::max(1, mBounds.height)); ++n;

  if (mTopLevel) {
    bool popup = mWindowType == eWindowType_popup;
    WidgetClass shellClass = popup ? overrideShellWidgetClass
                                   : topLevelShellWidgetClass;
    mWidget = XtAppCreateShell(nullptr, const_cast<char*>("Mozilla"),
                               shellClass, mDisplay, args, n);
    if (parentWidget && !popup) {
      XtVaSetValues(mWidget, XtNtransientFor,
                    XtParent(parentWidget) ? XtParent(parentWidget) : parentWidget,
                    nullptr);
    }
  } else {
    if (!parentWidget) {
      return NS_ERROR_FAILURE;
    }
    mWidget = XmCreateDrawingArea(parentWidget,
                                  const_cast<char*>("mozillaDrawingArea"),
                                  args, n);
  }

  if (!mWidget) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  XtVaSetValues(mWidget,
                XmNtraversalOn, False,
                XmNnavigationType, XmNONE,
                nullptr);

  EventMask mask = ExposureMask | StructureNotifyMask |
                   KeyPressMask | KeyReleaseMask |
                   ButtonPressMask | ButtonReleaseMask |
                   PointerMotionMask | EnterWindowMask | LeaveWindowMask |
                   FocusChangeMask | PropertyChangeMask;
  XtAddEventHandler(mWidget, mask, False, XtEventHandler, this);
  XtAddCallback(mWidget, XmNdestroyCallback, XtDestroyCallback, this);

  if (mTopLevel) {
    XtRealizeWidget(mWidget);
    UpdateWMProtocols();
  } else {
    XtManageChild(mWidget);
  }

  mCreated = true;
  mDestroyed = false;
  return NS_OK;
}

void
nsWindow::Destroy()
{
  if (mDestroyed) {
    return;
  }
  mDestroyed = true;
  mCreated = false;

  if (mLayerManager) {
    mLayerManager->Destroy();
    mLayerManager = nullptr;
  }
  DestroyCompositor();
  ClearCachedResources();

  Widget widget = mWidget;
  mWidget = nullptr;
  if (widget) {
    XtRemoveEventHandler(widget, ~0L, True, XtEventHandler, this);
    XtDestroyWidget(widget);
  }

  OnDestroy();
}

void
nsWindow::OnDestroy()
{
  if (mOnDestroyCalled) {
    return;
  }
  mOnDestroyCalled = true;
  nsBaseWidget::OnDestroy();
  nsBaseWidget::Destroy();
  NotifyWindowDestroyed();
}

nsresult
nsWindow::DispatchEvent(WidgetGUIEvent* aEvent, nsEventStatus& aStatus)
{
  aStatus = nsEventStatus_eIgnore;
  nsIWidgetListener* listener =
    mAttachedWidgetListener ? mAttachedWidgetListener : mWidgetListener;
  if (listener) {
    aStatus = listener->HandleEvent(aEvent, mUseAttachedEvents);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsWindow::Show(bool aState)
{
  mShown = aState;
  if (!mWidget || !mCreated) {
    return NS_OK;
  }

  if (aState) {
    if (mTopLevel) {
      if (!XtIsRealized(mWidget)) {
        XtRealizeWidget(mWidget);
      }
      XtMapWidget(mWidget);
    } else {
      XtManageChild(mWidget);
    }
  } else {
    if (mTopLevel) {
      XtUnmapWidget(mWidget);
    } else {
      XtUnmanageChild(mWidget);
    }
    ClearCachedResources();
  }
  return NS_OK;
}

NS_IMETHODIMP
nsWindow::Move(double aX, double aY)
{
  double scale = BoundsUseDesktopPixels() ? GetDesktopToDeviceScale().scale : 1.0;
  mBounds.x = NSToIntRound(aX * scale);
  mBounds.y = NSToIntRound(aY * scale);
  if (mWidget) {
    XtVaSetValues(mWidget, XmNx, mBounds.x, XmNy, mBounds.y, nullptr);
  }
  NotifyWindowMoved(mBounds.x, mBounds.y);
  NotifyRollupGeometryChange();
  return NS_OK;
}

NS_IMETHODIMP
nsWindow::Resize(double aWidth, double aHeight, bool aRepaint)
{
  double scale = BoundsUseDesktopPixels() ? GetDesktopToDeviceScale().scale : 1.0;
  int32_t w = NSToIntRound(aWidth * scale);
  int32_t h = NSToIntRound(aHeight * scale);
  ConstrainSize(&w, &h);
  mBounds.SizeTo(w, h);
  if (mWidget) {
    XtVaSetValues(mWidget,
                  XmNwidth, std::max(1, w),
                  XmNheight, std::max(1, h),
                  nullptr);
  }
  DispatchResized();
  if (aRepaint) {
    Invalidate(LayoutDeviceIntRect(0, 0, w, h));
  }
  return NS_OK;
}

NS_IMETHODIMP
nsWindow::Resize(double aX, double aY, double aWidth, double aHeight,
                 bool aRepaint)
{
  Move(aX, aY);
  return Resize(aWidth, aHeight, aRepaint);
}

NS_IMETHODIMP
nsWindow::Enable(bool aState)
{
  mEnabled = aState;
  if (mWidget) {
    XtSetSensitive(mWidget, aState ? True : False);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsWindow::SetFocus(bool aRaise)
{
  if (!mWidget || !XtIsRealized(mWidget)) {
    return NS_ERROR_FAILURE;
  }
  Window window = XtWindow(mWidget);
  if (aRaise) {
    XRaiseWindow(mDisplay, window);
  }
  XSetInputFocus(mDisplay, window, RevertToParent, CurrentTime);
  return NS_OK;
}

NS_IMETHODIMP
nsWindow::SetTitle(const nsAString& aTitle)
{
  if (!mWidget || !mTopLevel) {
    return NS_OK;
  }
  NS_ConvertUTF16toUTF8 title(aTitle);
  XtVaSetValues(mWidget, XtNtitle, title.get(), nullptr);
  return NS_OK;
}

static unsigned int
CursorShape(nsCursor aCursor)
{
  switch (aCursor) {
    case eCursor_wait: return XC_watch;
    case eCursor_select: return XC_xterm;
    case eCursor_hyperlink: return XC_hand2;
    case eCursor_crosshair: return XC_crosshair;
    case eCursor_move: return XC_fleur;
    case eCursor_n_resize: return XC_top_side;
    case eCursor_s_resize: return XC_bottom_side;
    case eCursor_e_resize: return XC_right_side;
    case eCursor_w_resize: return XC_left_side;
    case eCursor_ne_resize: return XC_top_right_corner;
    case eCursor_nw_resize: return XC_top_left_corner;
    case eCursor_se_resize: return XC_bottom_right_corner;
    case eCursor_sw_resize: return XC_bottom_left_corner;
    default: return XC_left_ptr;
  }
}

NS_IMETHODIMP
nsWindow::SetCursor(nsCursor aCursor)
{
  if (!mDisplay || !mWidget || !XtIsRealized(mWidget)) {
    return NS_OK;
  }
  if (mNativeCursor != None) {
    XFreeCursor(mDisplay, mNativeCursor);
  }
  mNativeCursor = XCreateFontCursor(mDisplay, CursorShape(aCursor));
  XDefineCursor(mDisplay, XtWindow(mWidget), mNativeCursor);
  mCursor = aCursor;
  return NS_OK;
}

NS_IMETHODIMP
nsWindow::SetCursor(imgIContainer* aCursor, uint32_t aHotspotX,
                    uint32_t aHotspotY)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsWindow::Invalidate(const LayoutDeviceIntRect& aRect)
{
  if (!mWidget || !XtIsRealized(mWidget)) {
    return NS_OK;
  }
  XClearArea(mDisplay, XtWindow(mWidget),
             aRect.x, aRect.y,
             std::max(0, aRect.width), std::max(0, aRect.height),
             True);
  return NS_OK;
}

void*
nsWindow::GetNativeData(uint32_t aDataType)
{
  switch (aDataType) {
    case NS_NATIVE_WINDOW:
    case NS_NATIVE_SHAREABLE_WINDOW:
      return reinterpret_cast<void*>(static_cast<uintptr_t>(XWindow()));
    case NS_NATIVE_WIDGET:
    case NS_NATIVE_SHELLWIDGET:
      return mWidget;
    case NS_NATIVE_DISPLAY:
      return mDisplay;
    default:
      return nullptr;
  }
}

LayoutDeviceIntPoint
nsWindow::WidgetToScreenOffset()
{
  if (!mWidget || !XtIsRealized(mWidget)) {
    return mBounds.TopLeft();
  }
  Position x = 0, y = 0;
  XtTranslateCoords(mWidget, 0, 0, &x, &y);
  return LayoutDeviceIntPoint(x, y);
}

LayoutDeviceIntRect
nsWindow::GetScreenBounds()
{
  LayoutDeviceIntRect result(WidgetToScreenOffset(), mBounds.Size());
  return result;
}

LayoutDeviceIntRect
nsWindow::GetClientBounds()
{
  return GetScreenBounds();
}

LayoutDeviceIntSize
nsWindow::GetClientSize()
{
  return mBounds.Size();
}

LayoutDeviceIntPoint
nsWindow::GetClientOffset()
{
  return LayoutDeviceIntPoint(0, 0);
}

float
nsWindow::GetDPI()
{
  if (!mDisplay) {
    return 96.0f;
  }
  int screen = DefaultScreen(mDisplay);
  int mm = DisplayWidthMM(mDisplay, screen);
  if (mm <= 0) {
    return 96.0f;
  }
  return float(double(DisplayWidth(mDisplay, screen)) * 25.4 / double(mm));
}

double
nsWindow::GetDefaultScaleInternal()
{
  return GetDPI() / 96.0;
}

bool
nsWindow::HasPendingInputEvent()
{
  if (!mDisplay) {
    return false;
  }
  XEvent event;
  return XCheckMaskEvent(mDisplay,
                         KeyPressMask | KeyReleaseMask |
                         ButtonPressMask | ButtonReleaseMask |
                         PointerMotionMask,
                         &event) ? (XPutBackEvent(mDisplay, &event), true) : false;
}

void
nsWindow::SetModal(bool aModal)
{
  mModal = aModal;
  if (mWidget && mTopLevel) {
    XtVaSetValues(mWidget, XmNdialogStyle,
                  aModal ? XmDIALOG_FULL_APPLICATION_MODAL
                         : XmDIALOG_MODELESS,
                  nullptr);
  }
}

void
nsWindow::SetSizeConstraints(const SizeConstraints& aConstraints)
{
  nsBaseWidget::SetSizeConstraints(aConstraints);
  if (!mDisplay || !mWidget || !mTopLevel || !XtIsRealized(mWidget)) {
    return;
  }

  XSizeHints hints;
  memset(&hints, 0, sizeof(hints));
  hints.flags = PMinSize | PMaxSize;
  hints.min_width = aConstraints.mMinSize.width;
  hints.min_height = aConstraints.mMinSize.height;
  hints.max_width = aConstraints.mMaxSize.width == NS_MAXSIZE
                    ? DisplayWidth(mDisplay, DefaultScreen(mDisplay)) * 8
                    : aConstraints.mMaxSize.width;
  hints.max_height = aConstraints.mMaxSize.height == NS_MAXSIZE
                     ? DisplayHeight(mDisplay, DefaultScreen(mDisplay)) * 8
                     : aConstraints.mMaxSize.height;
  XSetWMNormalHints(mDisplay, XtWindow(mWidget), &hints);
}

void
nsWindow::CaptureMouse(bool aCapture)
{
  if (!mDisplay || !mWidget || !XtIsRealized(mWidget)) {
    return;
  }
  if (aCapture) {
    XGrabPointer(mDisplay, XtWindow(mWidget), True,
                 ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
                 GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
  } else {
    XUngrabPointer(mDisplay, CurrentTime);
  }
}

void
nsWindow::CaptureRollupEvents(nsIRollupListener* aListener, bool aDoCapture)
{
  if (aDoCapture) {
    gRollupListener = aListener;
    CaptureMouse(true);
  } else {
    CaptureMouse(false);
    gRollupListener = nullptr;
  }
}

already_AddRefed<DrawTarget>
nsWindow::StartRemoteDrawingInRegion(LayoutDeviceIntRegion& aInvalidRegion,
                                     BufferMode* aBufferMode)
{
  if (!mDisplay || !mWidget || !XtIsRealized(mWidget)) {
    return nullptr;
  }

  if (aBufferMode) {
    *aBufferMode = BufferMode::BUFFER_NONE;
  }

  Visual* visual = DefaultVisual(mDisplay, DefaultScreen(mDisplay));
  cairo_surface_t* surface =
    cairo_xlib_surface_create(mDisplay, XtWindow(mWidget), visual,
                              std::max(1, mBounds.width),
                              std::max(1, mBounds.height));
  if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
    cairo_surface_destroy(surface);
    return nullptr;
  }

  SurfaceFormat format = DefaultDepth(mDisplay, DefaultScreen(mDisplay)) == 16
                         ? SurfaceFormat::R5G6B5_UINT16
                         : SurfaceFormat::B8G8R8X8;
  RefPtr<DrawTarget> target = Factory::CreateDrawTargetForCairoSurface(
      surface, IntSize(std::max(1, mBounds.width), std::max(1, mBounds.height)),
      &format);
  cairo_surface_destroy(surface);
  return target.forget();
}

void
nsWindow::EndRemoteDrawingInRegion(DrawTarget* aDrawTarget,
                                   LayoutDeviceIntRegion& aInvalidRegion)
{
  if (aDrawTarget) {
    aDrawTarget->Flush();
  }
  if (mDisplay) {
    XFlush(mDisplay);
  }
}

void
nsWindow::GetCompositorWidgetInitData(
    mozilla::widget::CompositorWidgetInitData* aInitData)
{
  *aInitData = mozilla::widget::CompositorWidgetInitData();
}

Window
nsWindow::XWindow() const
{
  return (mWidget && XtIsRealized(mWidget)) ? XtWindow(mWidget) : None;
}

Display*
nsWindow::XDisplay() const
{
  return mDisplay;
}

/* static */ void
nsWindow::XtEventHandler(Widget aWidget, XtPointer aClosure,
                         XEvent* aEvent, Boolean* aContinue)
{
  static_cast<nsWindow*>(aClosure)->HandleXEvent(aEvent);
}

/* static */ void
nsWindow::XtDestroyCallback(Widget aWidget, XtPointer aClosure,
                            XtPointer aCallData)
{
  nsWindow* self = static_cast<nsWindow*>(aClosure);
  if (self->mWidget == aWidget) {
    self->mWidget = nullptr;
  }
}

void
nsWindow::HandleXEvent(XEvent* aEvent)
{
  if (mDestroyed) {
    return;
  }

  switch (aEvent->type) {
    case Expose:
      if (aEvent->xexpose.count == 0) {
        Paint(aEvent->xexpose);
      }
      break;
    case ConfigureNotify: {
      bool resized = (mBounds.width != aEvent->xconfigure.width ||
                      mBounds.height != aEvent->xconfigure.height);
      mBounds.MoveTo(aEvent->xconfigure.x, aEvent->xconfigure.y);
      mBounds.SizeTo(aEvent->xconfigure.width, aEvent->xconfigure.height);
      if (resized) {
        DispatchResized();
      }
      NotifyWindowMoved(mBounds.x, mBounds.y);
      break;
    }
    case ButtonPress:
    case ButtonRelease:
    case MotionNotify:
      DispatchMouse(*aEvent);
      break;
    case KeyPress:
      DispatchKey(aEvent->xkey, true);
      break;
    case KeyRelease:
      DispatchKey(aEvent->xkey, false);
      break;
    case FocusIn:
      if (mWidgetListener) {
        mWidgetListener->WindowActivated();
      }
      break;
    case FocusOut:
      if (mWidgetListener) {
        mWidgetListener->WindowDeactivated();
      }
      break;
    case ClientMessage: {
      Atom wmDelete = XInternAtom(mDisplay, "WM_DELETE_WINDOW", False);
      if (Atom(aEvent->xclient.data.l[0]) == wmDelete && mWidgetListener) {
        mWidgetListener->RequestWindowClose(this);
      }
      break;
    }
    default:
      break;
  }
}

void
nsWindow::Paint(const XExposeEvent& aEvent)
{
  nsIWidgetListener* listener =
    mAttachedWidgetListener ? mAttachedWidgetListener : mWidgetListener;
  if (!listener) {
    return;
  }

  LayoutDeviceIntRegion region(
      LayoutDeviceIntRect(aEvent.x, aEvent.y, aEvent.width, aEvent.height));

  listener->WillPaintWindow(this);

  if (GetLayerManager()->GetBackendType() == LayersBackend::LAYERS_CLIENT) {
    listener->PaintWindow(this, region);
    listener->DidPaintWindow();
    return;
  }

  BufferMode mode = BufferMode::BUFFER_NONE;
  RefPtr<DrawTarget> dt = StartRemoteDrawingInRegion(region, &mode);
  if (!dt) {
    return;
  }
  RefPtr<gfxContext> context = gfxContext::CreateOrNull(dt);
  if (!context) {
    return;
  }

  AutoLayerManagerSetup setup(this, context, mode);
  listener->PaintWindow(this, region);
  EndRemoteDrawingInRegion(dt, region);
  listener->DidPaintWindow();
}

void
nsWindow::DispatchMouse(const XEvent& aEvent)
{
  EventMessage message = eVoidEvent;
  int32_t x = 0;
  int32_t y = 0;
  unsigned int state = 0;
  unsigned int button = 0;

  if (aEvent.type == MotionNotify) {
    message = eMouseMove;
    x = aEvent.xmotion.x;
    y = aEvent.xmotion.y;
    state = aEvent.xmotion.state;
  } else {
    message = aEvent.type == ButtonPress ? eMouseDown : eMouseUp;
    x = aEvent.xbutton.x;
    y = aEvent.xbutton.y;
    state = aEvent.xbutton.state;
    button = aEvent.xbutton.button;
  }

  // X11 wheel buttons are represented as scroll input elsewhere.  Keep them
  // out of the ordinary mouse-button path until the dedicated wheel bridge is
  // installed.
  if ((aEvent.type == ButtonPress || aEvent.type == ButtonRelease) &&
      (button == Button4 || button == Button5 || button == 6 || button == 7)) {
    return;
  }

  WidgetMouseEvent event(true, message, this, WidgetMouseEvent::eReal);
  event.mRefPoint = LayoutDeviceIntPoint(x, y);
  if (button) {
    event.button = ButtonForX(button);
    event.mClickCount = 1;
  }
  event.mModifiers = 0;
  if (state & ShiftMask) event.mModifiers |= MODIFIER_SHIFT;
  if (state & ControlMask) event.mModifiers |= MODIFIER_CONTROL;
  if (state & Mod1Mask) event.mModifiers |= MODIFIER_ALT;
  if (state & Mod4Mask) event.mModifiers |= MODIFIER_META;

  DispatchInputEvent(&event);
}

void
nsWindow::DispatchKey(const XKeyEvent& aEvent, bool aDown)
{
  XKeyEvent key = aEvent;
  KeySym sym = XLookupKeysym(&key, 0);

  WidgetKeyboardEvent event(true, aDown ? eKeyDown : eKeyUp, this);
  event.mNativeKeyEvent = nullptr;
  event.mKeyCode = 0;
  event.mCharCode = 0;
  event.mModifiers = 0;
  if (key.state & ShiftMask) event.mModifiers |= MODIFIER_SHIFT;
  if (key.state & ControlMask) event.mModifiers |= MODIFIER_CONTROL;
  if (key.state & Mod1Mask) event.mModifiers |= MODIFIER_ALT;
  if (key.state & Mod4Mask) event.mModifiers |= MODIFIER_META;

  if (sym >= XK_A && sym <= XK_Z) {
    event.mKeyCode = NS_VK_A + (sym - XK_A);
  } else if (sym >= XK_a && sym <= XK_z) {
    event.mKeyCode = NS_VK_A + (sym - XK_a);
  } else if (sym >= XK_0 && sym <= XK_9) {
    event.mKeyCode = NS_VK_0 + (sym - XK_0);
  } else {
    switch (sym) {
      case XK_Return: event.mKeyCode = NS_VK_RETURN; break;
      case XK_Escape: event.mKeyCode = NS_VK_ESCAPE; break;
      case XK_BackSpace: event.mKeyCode = NS_VK_BACK; break;
      case XK_Tab: event.mKeyCode = NS_VK_TAB; break;
      case XK_Left: event.mKeyCode = NS_VK_LEFT; break;
      case XK_Right: event.mKeyCode = NS_VK_RIGHT; break;
      case XK_Up: event.mKeyCode = NS_VK_UP; break;
      case XK_Down: event.mKeyCode = NS_VK_DOWN; break;
      case XK_Delete: event.mKeyCode = NS_VK_DELETE; break;
      case XK_Home: event.mKeyCode = NS_VK_HOME; break;
      case XK_End: event.mKeyCode = NS_VK_END; break;
      case XK_Page_Up: event.mKeyCode = NS_VK_PAGE_UP; break;
      case XK_Page_Down: event.mKeyCode = NS_VK_PAGE_DOWN; break;
      default: break;
    }
  }

  nsEventStatus status;
  DispatchEvent(&event, status);
}

void
nsWindow::DispatchResized()
{
  if (mWidgetListener) {
    mWidgetListener->WindowResized(this, mBounds.width, mBounds.height);
  }
  if (mAttachedWidgetListener) {
    mAttachedWidgetListener->WindowResized(this, mBounds.width, mBounds.height);
  }
}

void
nsWindow::UpdateWMProtocols()
{
  if (!mDisplay || !mWidget || !XtIsRealized(mWidget) || !mTopLevel) {
    return;
  }
  Atom wmDelete = XInternAtom(mDisplay, "WM_DELETE_WINDOW", False);
  XSetWMProtocols(mDisplay, XtWindow(mWidget), &wmDelete, 1);
}
