/*
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Author: Paulo Zanoni <pzanoni@mandriva.com>
 */

#include <stdlib.h>

#include "utils.h"

/* Here are the symbols that we don't care much about: implementations are just
 * enough to make our program work.
 * Please try to keep things sorted lexicographically */

#define PANORAMIX
#define XORG_VERSION_CURRENT 0

#include <xorg/cursor.h>
#include <xorg/dix.h>
#include <xorg/dixevents.h>
#include <xorg/dixstruct.h>
#include <xorg/edid.h>
#include <xorg/extension.h>
#include <xorg/extnsionst.h>
#include <xorg/globals.h>
#include <xorg/inputstr.h>
#include <xorg/mi.h>
#include <xorg/misc.h>
#include <xorg/migc.h>
#include <xorg/miline.h>
#include <xorg/opaque.h>
#include <xorg/os.h>
#include <xorg/panoramiXsrv.h>
#include <xorg/picturestr.h>
#include <xorg/randrstr.h>
#include <xorg/servermd.h>
#include <xorg/windowstr.h>
#include <xorg/xf86.h>
#include <xorg/xf86Crtc.h>
#include <xorg/xf86Module.h>
#include <xorg/xf86Priv.h>
#include <xorg/xf86str.h>

#if ABI_VIDEODRV_VERSION < SET_ABI_VERSION(6,0)
#error Video ABI is unsupported (too old)
#endif

#if ABI_VIDEODRV_VERSION >= SET_ABI_VERSION(8,0)
_X_EXPORT DevPrivateKeyRec      miZeroLineScreenKeyRec;
_X_EXPORT DevPrivateKeyRec      PictureScreenPrivateKeyRec;
_X_EXPORT DevPrivateKeyRec      rrPrivKeyRec;
_X_EXPORT DevPrivateKeyRec      xf86ScreenKeyRec;
#else
_X_EXPORT DevPrivateKey         miZeroLineScreenKey;
_X_EXPORT DevPrivateKey         PictureScreenPrivateKey;
_X_EXPORT DevPrivateKey         rrPrivKey;
_X_EXPORT DevPrivateKey         xf86ScreenKey;
#endif

_X_EXPORT const unsigned char   byte_reversed[256];
_X_EXPORT ClientPtr             clients[MAXCLIENTS];
_X_EXPORT CallbackListPtr       ClientStateCallback = NULL;
_X_EXPORT xf86MonPtr            ConfiguredMonitor = NULL;
_X_EXPORT TimeStamp             currentTime;
_X_EXPORT volatile char         dispatchException;
_X_EXPORT Bool                  DPMSEnabled = FALSE;
_X_EXPORT EventSwapPtr          EventSwapVector[128];
_X_EXPORT CallbackListPtr       FlushCallback = NULL;
_X_EXPORT unsigned long         globalSerialNumber = 0;
_X_EXPORT InputInfo             inputInfo;
_X_EXPORT BoxRec                miEmptyBox;
_X_EXPORT RegDataRec            miEmptyData;
_X_EXPORT int                   monitorResolution = 0;
_X_EXPORT Bool                  noCompositeExtension = TRUE;
_X_EXPORT Bool                  noPanoramiXExtension = TRUE;
_X_EXPORT Bool                  noRenderExtension = TRUE;
_X_EXPORT Bool                  noRRExtension = TRUE;
_X_EXPORT Bool                  noXFree86DRIExtension = TRUE;
#if ABI_VIDEODRV_VERSION < SET_ABI_VERSION(8,0)
_X_EXPORT PanoramiXData        *panoramiXdataPtr = NULL;
#endif
_X_EXPORT int                   PanoramiXNumScreens = 0;
_X_EXPORT PaddingInfo           PixmapWidthPaddingInfo[0];
#if ABI_VIDEODRV_VERSION >= SET_ABI_VERSION(8,0)
_X_EXPORT BoxRec                RegionEmptyBox;
_X_EXPORT RegDataRec            RegionEmptyData;
#endif
_X_EXPORT ScreenInfo            screenInfo;
_X_EXPORT int                   screenIsSaved = 0;
_X_EXPORT ClientPtr             serverClient = NULL;
_X_EXPORT unsigned long         serverGeneration = 0;
_X_EXPORT CallbackListPtr       ServerGrabCallback = NULL;
_X_EXPORT WindowPtr             WindowTable[MAXSCREENS];
_X_EXPORT unsigned long         XRC_DRAWABLE = 0;
_X_EXPORT confDRIRec            xf86ConfigDRI;
_X_EXPORT serverLayoutRec       xf86ConfigLayout;
_X_EXPORT int                   xf86CrtcConfigPrivateIndex = 0;
_X_EXPORT const DisplayModeRec  xf86DefaultModes[0];
_X_EXPORT xf86InfoRec           xf86Info;
_X_EXPORT ScrnInfoPtr          *xf86Screens = NULL;
_X_EXPORT unsigned long         XRT_WINDOW = 0;

/* This one is inside xf86Glocals.c (not .h!): */
_X_EXPORT XF86ConfigPtr         xf86configptr = NULL;


_X_EXPORT CursorPtr GetSpriteCursor(DeviceIntPtr pDev)
    { print_log("function %s called!\n", __FUNCTION__); return NULL; }

_X_EXPORT void GetSpritePosition(struct _DeviceIntRec *pDev, int *px, int *py)
    { print_log("function %s called!\n", __FUNCTION__); }

_X_EXPORT pointer LoaderSymbol(const char *symbol)
    { print_log("function %s called!\n", __FUNCTION__); return NULL; }

_X_EXPORT void miChangeClip(GCPtr pGC, int type, pointer pvalue, int nrects)
    { print_log("function %s called!\n", __FUNCTION__); }

_X_EXPORT void miChangeGC(GCPtr pGC, unsigned long mask)
    { print_log("function %s called!\n", __FUNCTION__); }

_X_EXPORT void miCopyClip(GCPtr pgcDst, GCPtr pgcSrc)
    { print_log("function %s called!\n", __FUNCTION__); }

_X_EXPORT void miCopyGC(GCPtr pGCSrc, unsigned long changes, GCPtr pGCDst)
    { print_log("function %s called!\n", __FUNCTION__); }

_X_EXPORT void miDestroyClip(GCPtr pGC)
    { print_log("function %s called!\n", __FUNCTION__); }

_X_EXPORT void miDestroyGC(GCPtr pGC)
    { print_log("function %s called!\n", __FUNCTION__); }

_X_EXPORT void miFillPolygon(DrawablePtr dst, GCPtr pgc, int shape, int mode,
			     int count, DDXPointPtr pPts)
    { print_log("function %s called!\n", __FUNCTION__); }

_X_EXPORT void miImageText8(DrawablePtr pDraw, GCPtr pGC, int x, int y,
			    int count, char *chars)
    { print_log("function %s called!\n", __FUNCTION__); }

_X_EXPORT void miImageText16(DrawablePtr pDraw, GCPtr pGC, int x, int y,
			     int count, unsigned short *chars)
    { print_log("function %s called!\n", __FUNCTION__); }

_X_EXPORT void miPolyFillArc(DrawablePtr pDraw, GCPtr pGC, int narcs,
			     xArc *parcs)
    { print_log("function %s called!\n", __FUNCTION__); }

_X_EXPORT void miPolyPoint(DrawablePtr pDrawable, GCPtr pGC, int mode, int npt,
			   xPoint *pptInit)
    { print_log("function %s called!\n", __FUNCTION__); }

_X_EXPORT void miPolyRectangle(DrawablePtr pDraw, GCPtr pGC, int nrects,
			       xRectangle *pRects)
    { print_log("function %s called!\n", __FUNCTION__); }

_X_EXPORT int miPolyText8(DrawablePtr pDraw, GCPtr pGC, int x, int y, int count,
			  char *chars)
    { print_log("function %s called!\n", __FUNCTION__); return 0; }

_X_EXPORT int miPolyText16(DrawablePtr pDraw, GCPtr pGC, int x, int y,
			   int count, unsigned short *chars)
    { print_log("function %s called!\n", __FUNCTION__); return 0; }

_X_EXPORT void miZeroPolyArc(DrawablePtr pDraw, GCPtr pGC, int narcs,
			     xArc *parcs)
    { print_log("function %s called!\n", __FUNCTION__); }

_X_EXPORT void NoopDDA(void)
    { print_log("function %s called!\n", __FUNCTION__); }

_X_EXPORT unsigned short StandardMinorOpcode(ClientPtr client)
    { print_log("function %s called!\n", __FUNCTION__); return 0; }

_X_EXPORT void TimerFree(OsTimerPtr pTimer)
    { print_log("function %s called!\n", __FUNCTION__); }

_X_EXPORT OsTimerPtr TimerSet(OsTimerPtr timer, int flags, CARD32 millis,
			      OsTimerCallback func, pointer arg)
    { print_log("function %s called!\n", __FUNCTION__); return NULL; }

_X_EXPORT void Xfree(pointer p)
    { print_log("function %s called!\n", __FUNCTION__); free(p); }

_X_EXPORT void xf86DPMSSet(ScrnInfoPtr pScrn, int PowerManagementMode,
			   int flags)
    { print_log("function %s called!\n", __FUNCTION__); }

_X_EXPORT Bool xf86SaveScreen(ScreenPtr pScreen, int mode)
    { print_log("function %s called!\n", __FUNCTION__); return FALSE; }

_X_EXPORT Bool xf86ServerIsOnlyDetecting(void)
    { print_log("function %s called!\n", __FUNCTION__); return FALSE; }

/* Stuff with non-default implementation/definition: */

#if ABI_VIDEODRV_VERSION >= SET_ABI_VERSION(8,0)
_X_EXPORT void xf86ErrorFVerb(int verb, const char *format, ...) _X_ATTRIBUTE_PRINTF(2,3);
_X_EXPORT void xf86Msg(MessageType type, const char *format, ...) _X_ATTRIBUTE_PRINTF(2,3);
#else
_X_EXPORT void xf86ErrorFVerb(int verb, const char *format, ...) _printf_attribute(2,3);
_X_EXPORT void xf86Msg(MessageType type, const char *format, ...) _printf_attribute(2,3);
#endif

void xf86ErrorFVerb(int verb, const char *format, ...)
    { print_log("function %s called!\n", __FUNCTION__); }

void xf86Msg(MessageType type, const char *format, ...)
    { print_log("function %s called!\n", __FUNCTION__); }

_X_EXPORT void LoaderGetOS(const char **name, int *major, int *minor,
			   int *teeny)
{
    print_log("function %s called!\n", __FUNCTION__);
    if (name) {
#if defined(__linux__)
	*name = "linux";
#else
	*name = "unknown";
#endif
    }
}
