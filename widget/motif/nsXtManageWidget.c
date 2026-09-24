/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * Mozilla's original Motif manager widget, ported from Mozilla 0.9.
 *
 * The browser used a custom XmManager subclass instead of XmDrawingArea
 * because DrawingArea may resize itself to fit its contents.  Gecko owns
 * child geometry, so this manager accepts requested geometry directly.
 */

#include "nsXtManageWidget.h"
#include "nsXtManageWidgetP.h"

static XtGeometryResult
GeometryManager(Widget w, XtWidgetGeometry* request, XtWidgetGeometry* reply);

static void
ChangeManaged(Widget w);

extern void nsWindow_ResizeWidget(Widget w);

NewManageClassRec newManageClassRec = {
  {
    (WidgetClass)&xmManagerClassRec,
    "NewManage",
    sizeof(NewManageRec),
    NULL,
    NULL,
    FALSE,
    NULL,
    NULL,
    XtInheritRealize,
    NULL,
    0,
    NULL,
    0,
    NULLQUARK,
    TRUE,
    TRUE,
    TRUE,
    FALSE,
    NULL,
    nsWindow_ResizeWidget,
    XtInheritExpose,
    NULL,
    NULL,
    XtInheritSetValuesAlmost,
    NULL,
    NULL,
    XtVersion,
    NULL,
    XtInheritTranslations,
    NULL,
    NULL,
    NULL
  },
  {
    GeometryManager,
    ChangeManaged,
    XtInheritInsertChild,
    XtInheritDeleteChild,
    NULL
  },
  {
    NULL,
    0,
    0,
    NULL,
    NULL,
    NULL,
    NULL
  },
  {
    XmInheritTranslations,
    NULL,
    0,
    NULL,
    0,
    XmInheritParentProcess,
    NULL
  },
  {
    0
  }
};

WidgetClass newManageClass = (WidgetClass)&newManageClassRec;

static XtGeometryResult
GeometryManager(Widget w, XtWidgetGeometry* request, XtWidgetGeometry* reply)
{
  (void)reply;

  if (request->request_mode & XtCWQueryOnly) {
    return XtGeometryYes;
  }

  if (request->request_mode & CWX) {
    XtX(w) = request->x;
  }
  if (request->request_mode & CWY) {
    XtY(w) = request->y;
  }
  if (request->request_mode & CWWidth) {
    XtWidth(w) = request->width;
  }
  if (request->request_mode & CWHeight) {
    XtHeight(w) = request->height;
  }
  if (request->request_mode & CWBorderWidth) {
    XtBorderWidth(w) = request->border_width;
  }

  return XtGeometryYes;
}

static void
ChangeManaged(Widget w)
{
  (void)w;
}
