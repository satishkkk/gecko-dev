/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 *  Vladimir Vukicevic <vladimir@pobox.com>
 * Portions created by the Initial Developer are Copyright (C) 2004
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef NSCAIRODRAWINGSURFACE__H__
#define NSCAIRODRAWINGSURFACE__H__

#include "nsIDrawingSurface.h"

#include <cairo.h>

#if defined(MOZ_ENABLE_GTK2)
#include <gdk/gdkx.h>
#include <X11/extensions/XShm.h>
#endif

class nsIWidget;
class nsCairoDeviceContext;

class nsCairoDrawingSurface : public nsIDrawingSurface
{
public:
    nsCairoDrawingSurface ();
    virtual ~nsCairoDrawingSurface ();

    // create a image surface if aFastAccess == TRUE, otherwise create
    // a fast server pixmap
    nsresult Init (nsCairoDeviceContext *aDC, PRUint32 aWidth, PRUint32 aHeight, PRBool aFastAccess);

    // create a fast drawing surface for a native widget
    nsresult Init (nsCairoDeviceContext *aDC, nsIWidget *aWidget);

    // nsISupports interface
    NS_DECL_ISUPPORTS

    // nsIDrawingSurface interface

    NS_IMETHOD Lock(PRInt32 aX, PRInt32 aY, PRUint32 aWidth, PRUint32 aHeight,
                    void **aBits, PRInt32 *aStride, PRInt32 *aWidthBytes,
                    PRUint32 aFlags);
    NS_IMETHOD Unlock(void);
    NS_IMETHOD GetDimensions(PRUint32 *aWidth, PRUint32 *aHeight);
    NS_IMETHOD IsOffscreen(PRBool *aOffScreen);
    NS_IMETHOD IsPixelAddressable(PRBool *aAddressable);
    NS_IMETHOD GetPixelFormat(nsPixelFormat *aFormat);

    /* utility functions */
    cairo_surface_t *GetCairoSurface(void) { return mSurface; }
    PRInt32 GetDepth() { /* XXX */ return 32; }
private:
    cairo_surface_t *mSurface, *mImageSurface;

#if defined(MOZ_ENABLE_GTK2) || defined(MOZ_ENABLE_XLIB)
    Display *mXDisplay;
    Pixmap mPixmap;
    XShmSegmentInfo mShmInfo;
#endif

    PRUint32 mLockFlags;
    PRBool mFastAccess;
    PRUint32 mWidth, mHeight;

};

#endif /* NSCAIRODRAWINGSURFACE__H__ */
