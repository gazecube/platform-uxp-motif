/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * Ported from Mozilla 0.9 widget/src/motif/nsXtManageWidgetP.h.
 */

#ifndef nsXtManageWidgetP_h__
#define nsXtManageWidgetP_h__

#include "nsXtManageWidget.h"
#include <Xm/ManagerP.h>

typedef struct {
  int unused;
} NewManageClassPart;

typedef struct _NewManageClassRec {
  CoreClassPart core_class;
  CompositeClassPart composite_class;
  ConstraintClassPart constraint_class;
  XmManagerClassPart manager_class;
  NewManageClassPart newManage_class;
} NewManageClassRec;

extern NewManageClassRec newManageClassRec;

typedef struct {
  void* why;
  XtCallbackList input_callback;
} NewManagePart;

typedef struct _NewManageRec {
  CorePart core;
  CompositePart composite;
  ConstraintPart constraint;
  XmManagerPart manager;
  NewManagePart newManage;
} NewManageRec;

#endif /* nsXtManageWidgetP_h__ */
