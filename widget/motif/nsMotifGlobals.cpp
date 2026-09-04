/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nsIRollupListener.h"

/*
 * Backend-local popup rollup pointer.  This intentionally does not alias the
 * private nsBaseWidget state; nsWindow currently uses it only to pair Motif's
 * X pointer grab with the active popup listener lifetime.
 */
nsIRollupListener* gRollupListener = nullptr;
