/********************************************************************/
/*                                                                  */
/*  gkb_win.c     Keyboard and mouse access for windows.            */
/*  Copyright (C) 1989 - 2013, 2015, 2016, 2018  Thomas Mertes      */
/*  Copyright (C) 2020, 2021  Thomas Mertes                         */
/*                                                                  */
/*  This file is part of the Seed7 Runtime Library.                 */
/*                                                                  */
/*  The Seed7 Runtime Library is free software; you can             */
/*  redistribute it and/or modify it under the terms of the GNU     */
/*  Lesser General Public License as published by the Free Software */
/*  Foundation; either version 2.1 of the License, or (at your      */
/*  option) any later version.                                      */
/*                                                                  */
/*  The Seed7 Runtime Library is distributed in the hope that it    */
/*  will be useful, but WITHOUT ANY WARRANTY; without even the      */
/*  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR */
/*  PURPOSE.  See the GNU Lesser General Public License for more    */
/*  details.                                                        */
/*                                                                  */
/*  You should have received a copy of the GNU Lesser General       */
/*  Public License along with this program; if not, write to the    */
/*  Free Software Foundation, Inc., 51 Franklin Street,             */
/*  Fifth Floor, Boston, MA  02110-1301, USA.                       */
/*                                                                  */
/*  Module: Seed7 Runtime Library                                   */
/*  File: seed7/src/gkb_x11.c                                       */
/*  Changes: 2005 - 2007, 2013  Thomas Mertes                       */
/*  Content: Keyboard and mouse access for windows.                 */
/*                                                                  */
/********************************************************************/

#define LOG_FUNCTIONS 0
#define VERBOSE_EXCEPTIONS 0

#include "version.h"

#include "stdlib.h"
#include "stdio.h"
#include "windows.h"
#if WINDOWSX_H_PRESENT
#include "windowsx.h"
#else
#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))
#endif

#include "common.h"
#include "data_rtl.h"
#include "os_decls.h"
#include "hsh_rtl.h"
#include "rtl_err.h"
#include "kbd_drv.h"


#define TRACE_EVENTS 0
#if TRACE_EVENTS
#define traceEvent(traceStatements) traceStatements
#else
#define traceEvent(traceStatements)
#endif
#define traceEventX(traceStatements) traceStatements

#define TRACE_SET_WINDOW_POS 0

static intType clicked_x = 0;
static intType clicked_y = 0;
static HWND clickedWindow = 0;
static rtlHashType window_hash = NULL;
static WPARAM resizeMode = 0;
static int resizeLeft;
static int resizeTop;
static int resizeStartWidth;
static int resizeStartHeight;
static int resizeWidthDelta;
static int resizeHeightDelta;
static boolType processDeadKeys = FALSE;
static boolType oem1_dead = FALSE;
static boolType oem2_dead = FALSE;
static boolType oem3_dead = FALSE;
static boolType oem4_dead = FALSE;
static boolType oem5_dead = FALSE;
static boolType oem6_dead = FALSE;
static boolType oem7_dead = FALSE;
static boolType oem8_dead = FALSE;
static boolType shift_oem1_dead = FALSE;
static boolType shift_oem2_dead = FALSE;
static boolType shift_oem3_dead = FALSE;
static boolType shift_oem4_dead = FALSE;
static boolType shift_oem5_dead = FALSE;
static boolType shift_oem6_dead = FALSE;
static boolType shift_oem7_dead = FALSE;
static boolType shift_oem8_dead = FALSE;
static WPARAM deadKeyActive = 0;


static const charType map_1252_to_unicode[] = {
/* 128 */ 0x20AC,    '?', 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,
/* 136 */ 0x02C6, 0x2030, 0x0160, 0x2039, 0x0152,    '?', 0x017D,    '?',
/* 144 */    '?', 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
/* 152 */ 0x02DC, 0x2122, 0x0161, 0x203A, 0x0153,    '?', 0x017E, 0x0178};

extern int getCloseAction (winType actual_window);
extern void setResizeReturnsKey (winType resizeWindow, boolType active);
extern boolType getResizeReturnsKey (winType resizeWindow);
extern void drwSetCloseAction (winType actual_window, intType closeAction);
extern intType drwHeight (const_winType actual_window);
extern intType drwWidth (const_winType actual_window);


#ifndef GET_XBUTTON_WPARAM
#define GET_XBUTTON_WPARAM(wParam)      (HIWORD(wParam))
#endif

#ifndef VK_XBUTTON1
#define VK_XBUTTON1 5
#endif

#ifndef VK_XBUTTON2
#define VK_XBUTTON2 6
#endif

#ifndef XBUTTON1
#define XBUTTON1 0x0001
#endif

#ifndef XBUTTON2
#define XBUTTON2 0x0002
#endif

#ifndef WM_XBUTTONDOWN
#define WM_XBUTTONDOWN 0x020B
#endif

#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL  0x020A
#endif

#ifndef GET_WHEEL_DELTA_WPARAM
#define GET_WHEEL_DELTA_WPARAM(wparam) ((short)HIWORD (wparam))
#endif

#ifndef MAPVK_VK_TO_VSC
#define MAPVK_VK_TO_VSC 0
#endif

#ifndef VK_OEM_1
#define VK_OEM_1     0xBA
#define VK_OEM_2     0xBF
#define VK_OEM_3     0xC0
#define VK_OEM_4     0xDB
#define VK_OEM_5     0xDC
#define VK_OEM_6     0xDD
#define VK_OEM_7     0xDE
#define VK_OEM_8     0xDF
#define VK_OEM_PLUS  0xBB
#define VK_OEM_MINUS 0xBD
#endif



winType find_window (HWND sys_window)

  {
    winType window;

  /* find_window */
    if (window_hash == NULL) {
      window = NULL;
    } else {
      window = (winType) (memSizeType)
          hshIdxDefault0(window_hash,
                         (genericType) (memSizeType) sys_window,
                         (intType) ((memSizeType) sys_window) >> 6,
                         (compareType) &genericCmp);
    } /* if */
    logFunction(printf("find_window(" FMT_X_MEM ") --> " FMT_X_MEM "\n",
                       (memSizeType) sys_window, (memSizeType) window););
    return window;
  } /* find_window */



void enter_window (winType curr_window, HWND sys_window)

  { /* enter_window */
    logFunction(printf("enter_window(" FMT_X_MEM ", " FMT_X_MEM ")\n",
                       (memSizeType) curr_window,
                       (memSizeType) sys_window););
    if (window_hash == NULL) {
      window_hash = hshEmpty();
    } /* if */
    (void) hshIdxEnterDefault(window_hash,
                              (genericType) (memSizeType) sys_window,
                              (genericType) (memSizeType) curr_window,
                              (intType) ((memSizeType) sys_window) >> 6);
  } /* enter_window */



void remove_window (HWND sys_window)

  { /* remove_window */
    logFunction(printf("remove_window(" FMT_X_MEM ")\n",
                       (memSizeType) sys_window););
    if (window_hash != NULL) {
      hshExcl(window_hash,
              (genericType) (memSizeType) sys_window,
              (intType) ((memSizeType) sys_window) >> 6,
              (compareType) &genericCmp,
              (destrFuncType) &genericDestr,
              (destrFuncType) &genericDestr);
    } /* if */
  } /* remove_window */



static void resizeBottomAndRight (MSG *msg)

  {
    POINT point;
    RECT rect;

  /* resizeBottomAndRight */
    logFunction(printf("resizeBottomAndRight(" FMT_U_MEM ", %d, %d)\n",
                       (memSizeType) msg->hwnd,
                       GET_X_LPARAM(msg->lParam), GET_Y_LPARAM(msg->lParam)););
    resizeMode = msg->wParam;
    point.x = GET_X_LPARAM(msg->lParam);
    point.y = GET_Y_LPARAM(msg->lParam);
    ScreenToClient(msg->hwnd, &point);
    SetCapture(msg->hwnd);
    GetWindowRect(msg->hwnd, &rect);
    resizeStartWidth = (int) (rect.right - rect.left);
    resizeStartHeight = (int) (rect.bottom - rect.top);
    GetClientRect(msg->hwnd, &rect);
    resizeWidthDelta = resizeStartWidth - (int) (rect.right - rect.left);
    resizeHeightDelta = resizeStartHeight - (int) (rect.bottom - rect.top);
    resizeWidthDelta -= point.x - rect.right;
    resizeHeightDelta -= point.y - rect.bottom;
    BringWindowToTop(msg->hwnd);
  } /* resizeBottomAndRight */



static void resizeTopAndLeft (MSG *msg)

  {
    POINT point;
    RECT rect;

  /* resizeTopAndLeft */
    logFunction(printf("resizeTopAndLeft(" FMT_U_MEM ", %d, %d)\n",
                       (memSizeType) msg->hwnd,
                       GET_X_LPARAM(msg->lParam), GET_Y_LPARAM(msg->lParam)););
    resizeMode = msg->wParam;
    point.x = GET_X_LPARAM(msg->lParam);
    point.y = GET_Y_LPARAM(msg->lParam);
    ScreenToClient(msg->hwnd, &point);
    SetCapture(msg->hwnd);
    GetWindowRect(msg->hwnd, &rect);
    resizeLeft = rect.left;
    resizeTop = rect.top;
    resizeStartWidth = (int) (rect.right - rect.left);
    resizeStartHeight = (int) (rect.bottom - rect.top);
    GetClientRect(msg->hwnd, &rect);
    resizeWidthDelta = point.x - rect.left;
    resizeHeightDelta = point.y - rect.top;
    BringWindowToTop(msg->hwnd);
  } /* resizeTopAndLeft */



static void startMoveWindow (MSG *msg)

  {
    POINT point;
    RECT rect;

  /* startMoveWindow */
    logFunction(printf("startMoveWindow(" FMT_U_MEM ", %d, %d)\n",
                       (memSizeType) msg->hwnd,
                       GET_X_LPARAM(msg->lParam), GET_Y_LPARAM(msg->lParam)););
    resizeMode = msg->wParam;
    point.x = GET_X_LPARAM(msg->lParam);
    point.y = GET_Y_LPARAM(msg->lParam);
    ScreenToClient(msg->hwnd, &point);
    SetCapture(msg->hwnd);
    GetWindowRect(msg->hwnd, &rect);
    resizeLeft = rect.left;
    resizeTop = rect.top;
    GetClientRect(msg->hwnd, &rect);
    resizeWidthDelta = point.x - rect.left;
    resizeHeightDelta = point.y - rect.top;
    BringWindowToTop(msg->hwnd);
  } /* startMoveWindow */



static void systemSizeCommand (MSG *msg)

  {
    RECT windowRect;
    RECT clientRect;
    POINT clientBottomRight;

  /* systemSizeCommand */
    GetWindowRect(msg->hwnd, &windowRect);
    GetClientRect(msg->hwnd, &clientRect);
    clientBottomRight.x = clientRect.right;
    clientBottomRight.y = clientRect.bottom;
    ClientToScreen(msg->hwnd, &clientBottomRight);
    SetCursorPos(clientBottomRight.x + (windowRect.right - clientBottomRight.x) / 2,
                 clientBottomRight.y + (windowRect.bottom - clientBottomRight.y) / 2);
  } /* systemSizeCommand */



static void systemMoveCommand (MSG *msg)

  {
    RECT windowRect;
    RECT clientRect;
    POINT clientTopLeft;

  /* systemMoveCommand */
    GetWindowRect(msg->hwnd, &windowRect);
    GetClientRect(msg->hwnd, &clientRect);
    clientTopLeft.x = clientRect.left;
    clientTopLeft.y = clientRect.top;
    ClientToScreen(msg->hwnd, &clientTopLeft);
    SetCursorPos(windowRect.left + (windowRect.right - windowRect.left) / 2,
                 windowRect.top + (clientTopLeft.y - windowRect.top) / 2);
  } /* systemMoveCommand */



#if TRACE_SET_WINDOW_POS
static BOOL mySetWindowPos (HWND hWnd, HWND hWndInsertAfter, int X, int Y,
    int cx, int cy, UINT uFlags)

  { /* mySetWindowPos */
    printf("SetWindowPos(" FMT_U_MEM ", " FMT_U_MEM ", %d, %d, %d, %d, %x)\n",
           (memSizeType) hWnd, (memSizeType) hWndInsertAfter,
           X, Y, cx, cy, uFlags);
    return SetWindowPos(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
  } /* mySetWindowPos */

#define SetWindowPos mySetWindowPos
#endif



static void processMouseMove (MSG *msg)

  { /* processMouseMove */
    logFunction(printf("processMouseMove(" FMT_U_MEM ", "
                       "(resizeMode=" FMT_U_MEM "))\n",
                       (memSizeType) msg->hwnd, (memSizeType) resizeMode););
    switch (resizeMode) {
      case HTBOTTOMRIGHT:
        SetWindowPos(msg->hwnd, 0, 0, 0,
            GET_X_LPARAM(msg->lParam) + resizeWidthDelta,
            GET_Y_LPARAM(msg->lParam) + resizeHeightDelta,
            /* SWP_NOSENDCHANGING | */ SWP_NOZORDER | SWP_NOMOVE);
        break;
      case HTRIGHT:
        SetWindowPos(msg->hwnd, 0, 0, 0,
            GET_X_LPARAM(msg->lParam) + resizeWidthDelta,
            resizeStartHeight,
            /* SWP_NOSENDCHANGING | */ SWP_NOZORDER | SWP_NOMOVE);
        break;
      case HTBOTTOM:
        SetWindowPos(msg->hwnd, 0, 0, 0,
            resizeStartWidth,
            GET_Y_LPARAM(msg->lParam) + resizeHeightDelta,
            /* SWP_NOSENDCHANGING | */ SWP_NOZORDER | SWP_NOMOVE);
        break;
      case HTTOPLEFT:
        if ((int) resizeStartWidth - GET_X_LPARAM(msg->lParam) + resizeWidthDelta >= GetSystemMetrics(SM_CXMINTRACK) &&
            (int) resizeStartHeight - GET_Y_LPARAM(msg->lParam) + resizeHeightDelta >= GetSystemMetrics(SM_CYMINTRACK)) {
          SetWindowPos(msg->hwnd, 0,
              resizeLeft + GET_X_LPARAM(msg->lParam) - resizeWidthDelta,
              resizeTop + GET_Y_LPARAM(msg->lParam) - resizeHeightDelta,
              resizeStartWidth - GET_X_LPARAM(msg->lParam) + resizeWidthDelta,
              resizeStartHeight - GET_Y_LPARAM(msg->lParam) + resizeHeightDelta,
              SWP_NOSENDCHANGING | SWP_NOZORDER);
          resizeLeft += GET_X_LPARAM(msg->lParam) - resizeWidthDelta;
          resizeTop += GET_Y_LPARAM(msg->lParam) - resizeHeightDelta;
          resizeStartWidth -= GET_X_LPARAM(msg->lParam) - resizeWidthDelta;
          resizeStartHeight -= GET_Y_LPARAM(msg->lParam) - resizeHeightDelta;
        } else if ((int) resizeStartWidth - GET_X_LPARAM(msg->lParam) + resizeWidthDelta >= GetSystemMetrics(SM_CXMINTRACK) &&
            (int) resizeStartHeight >= GetSystemMetrics(SM_CYMINTRACK)) {
          SetWindowPos(msg->hwnd, 0,
              resizeLeft + GET_X_LPARAM(msg->lParam) - resizeWidthDelta,
              resizeTop + resizeStartHeight - GetSystemMetrics(SM_CYMINTRACK),
              resizeStartWidth - GET_X_LPARAM(msg->lParam) + resizeWidthDelta,
              GetSystemMetrics(SM_CYMINTRACK),
              SWP_NOSENDCHANGING | SWP_NOZORDER);
          resizeLeft += GET_X_LPARAM(msg->lParam) - resizeWidthDelta;
          resizeTop += resizeStartHeight - GetSystemMetrics(SM_CYMINTRACK);
          resizeStartWidth -= GET_X_LPARAM(msg->lParam) - resizeWidthDelta;
          resizeStartHeight = GetSystemMetrics(SM_CYMINTRACK);
        } else if ((int) resizeStartWidth >= GetSystemMetrics(SM_CXMINTRACK) &&
            (int) resizeStartHeight - GET_Y_LPARAM(msg->lParam) + resizeHeightDelta >= GetSystemMetrics(SM_CYMINTRACK)) {
          SetWindowPos(msg->hwnd, 0,
              resizeLeft + resizeStartWidth - GetSystemMetrics(SM_CXMINTRACK),
              resizeTop + GET_Y_LPARAM(msg->lParam) - resizeHeightDelta,
              GetSystemMetrics(SM_CXMINTRACK),
              resizeStartHeight - GET_Y_LPARAM(msg->lParam) + resizeHeightDelta,
              SWP_NOSENDCHANGING | SWP_NOZORDER);
          resizeLeft += resizeStartWidth - GetSystemMetrics(SM_CXMINTRACK);
          resizeTop += GET_Y_LPARAM(msg->lParam) - resizeHeightDelta;
          resizeStartWidth = GetSystemMetrics(SM_CXMINTRACK);
          resizeStartHeight -= GET_Y_LPARAM(msg->lParam) - resizeHeightDelta;
        } else if ((int) resizeStartWidth >= GetSystemMetrics(SM_CXMINTRACK) &&
            (int) resizeStartHeight >= GetSystemMetrics(SM_CYMINTRACK)) {
          SetWindowPos(msg->hwnd, 0,
              resizeLeft + resizeStartWidth - GetSystemMetrics(SM_CXMINTRACK),
              resizeTop + resizeStartHeight - GetSystemMetrics(SM_CYMINTRACK),
              GetSystemMetrics(SM_CXMINTRACK),
              GetSystemMetrics(SM_CYMINTRACK),
              SWP_NOSENDCHANGING | SWP_NOZORDER);
          resizeLeft += resizeStartWidth - GetSystemMetrics(SM_CXMINTRACK);
          resizeTop += resizeStartHeight - GetSystemMetrics(SM_CYMINTRACK);
          resizeStartWidth = GetSystemMetrics(SM_CXMINTRACK);
          resizeStartHeight = GetSystemMetrics(SM_CYMINTRACK);
        } /* if */
        break;
      case HTLEFT:
        if ((int) resizeStartWidth - GET_X_LPARAM(msg->lParam) + resizeWidthDelta >= GetSystemMetrics(SM_CXMINTRACK)) {
          SetWindowPos(msg->hwnd, 0,
              resizeLeft + GET_X_LPARAM(msg->lParam) - resizeWidthDelta,
              resizeTop,
              resizeStartWidth - GET_X_LPARAM(msg->lParam) + resizeWidthDelta,
              resizeStartHeight,
              SWP_NOSENDCHANGING | SWP_NOZORDER);
          resizeLeft += GET_X_LPARAM(msg->lParam) - resizeWidthDelta;
          resizeStartWidth -= GET_X_LPARAM(msg->lParam) - resizeWidthDelta;
        } else if ((int) resizeStartWidth >= GetSystemMetrics(SM_CXMINTRACK)) {
          SetWindowPos(msg->hwnd, 0,
              resizeLeft + resizeStartWidth - GetSystemMetrics(SM_CXMINTRACK),
              resizeTop,
              GetSystemMetrics(SM_CXMINTRACK),
              resizeStartHeight,
              SWP_NOSENDCHANGING | SWP_NOZORDER);
          resizeLeft += resizeStartWidth - GetSystemMetrics(SM_CXMINTRACK);
          resizeStartWidth = GetSystemMetrics(SM_CXMINTRACK);
        } /* if */
        break;
      case HTTOP:
        if ((int) resizeStartHeight - GET_Y_LPARAM(msg->lParam) + resizeHeightDelta >= GetSystemMetrics(SM_CYMINTRACK)) {
          SetWindowPos(msg->hwnd, 0,
              resizeLeft,
              resizeTop + GET_Y_LPARAM(msg->lParam) - resizeHeightDelta,
              resizeStartWidth,
              resizeStartHeight - GET_Y_LPARAM(msg->lParam) + resizeHeightDelta,
              SWP_NOSENDCHANGING | SWP_NOZORDER);
          resizeTop += GET_Y_LPARAM(msg->lParam) - resizeHeightDelta;
          resizeStartHeight -= GET_Y_LPARAM(msg->lParam) - resizeHeightDelta;
        } else if ((int) resizeStartHeight >= GetSystemMetrics(SM_CYMINTRACK)) {
          SetWindowPos(msg->hwnd, 0,
              resizeLeft,
              resizeTop + resizeStartHeight - GetSystemMetrics(SM_CYMINTRACK),
              resizeStartWidth,
              GetSystemMetrics(SM_CYMINTRACK),
              SWP_NOSENDCHANGING | SWP_NOZORDER);
          resizeTop += resizeStartHeight - GetSystemMetrics(SM_CYMINTRACK);
          resizeStartHeight = GetSystemMetrics(SM_CYMINTRACK);
        } /* if */
        break;
      case HTTOPRIGHT:
        if ((int) resizeStartHeight - GET_Y_LPARAM(msg->lParam) + resizeHeightDelta >= GetSystemMetrics(SM_CYMINTRACK)) {
          SetWindowPos(msg->hwnd, 0,
              resizeLeft,
              resizeTop + GET_Y_LPARAM(msg->lParam) - resizeHeightDelta,
              resizeStartWidth + GET_X_LPARAM(msg->lParam) - resizeWidthDelta,
              resizeStartHeight - GET_Y_LPARAM(msg->lParam) + resizeHeightDelta,
              /* SWP_NOSENDCHANGING | */ SWP_NOZORDER);
          resizeTop += GET_Y_LPARAM(msg->lParam) - resizeHeightDelta;
          resizeStartHeight -= GET_Y_LPARAM(msg->lParam) - resizeHeightDelta;
        } else if ((int) resizeStartHeight >= GetSystemMetrics(SM_CYMINTRACK)) {
          SetWindowPos(msg->hwnd, 0,
              resizeLeft,
              resizeTop + resizeStartHeight - GetSystemMetrics(SM_CYMINTRACK),
              resizeStartWidth + GET_X_LPARAM(msg->lParam) - resizeWidthDelta,
              GetSystemMetrics(SM_CYMINTRACK),
              /* SWP_NOSENDCHANGING | */ SWP_NOZORDER);
          resizeTop += resizeStartHeight - GetSystemMetrics(SM_CYMINTRACK);
          resizeStartHeight = GetSystemMetrics(SM_CYMINTRACK);
        } /* if */
        break;
      case HTBOTTOMLEFT:
        if ((int) resizeStartWidth - GET_X_LPARAM(msg->lParam) + resizeWidthDelta >= GetSystemMetrics(SM_CXMINTRACK)) {
          SetWindowPos(msg->hwnd, 0,
              resizeLeft + GET_X_LPARAM(msg->lParam) - resizeWidthDelta,
              resizeTop,
              resizeStartWidth - GET_X_LPARAM(msg->lParam) + resizeWidthDelta,
              resizeStartHeight + GET_Y_LPARAM(msg->lParam) - resizeHeightDelta,
              /* SWP_NOSENDCHANGING | */ SWP_NOZORDER);
          resizeLeft += GET_X_LPARAM(msg->lParam) - resizeWidthDelta;
          resizeStartWidth -= GET_X_LPARAM(msg->lParam) - resizeWidthDelta;
        } else if ((int) resizeStartWidth >= GetSystemMetrics(SM_CXMINTRACK)) {
          SetWindowPos(msg->hwnd, 0,
              resizeLeft + resizeStartWidth - GetSystemMetrics(SM_CXMINTRACK),
              resizeTop,
              GetSystemMetrics(SM_CXMINTRACK),
              resizeStartHeight + GET_Y_LPARAM(msg->lParam) - resizeHeightDelta,
              /* SWP_NOSENDCHANGING | */ SWP_NOZORDER);
          resizeLeft += resizeStartWidth - GetSystemMetrics(SM_CXMINTRACK);
          resizeStartWidth = GetSystemMetrics(SM_CXMINTRACK);
        } /* if */
        break;
      case HTCAPTION:
        SetWindowPos(msg->hwnd, 0,
            resizeLeft + GET_X_LPARAM(msg->lParam) - resizeWidthDelta,
            resizeTop + GET_Y_LPARAM(msg->lParam) - resizeHeightDelta,
            0, 0,
            SWP_NOSENDCHANGING | SWP_NOZORDER | SWP_NOSIZE);
          resizeLeft += GET_X_LPARAM(msg->lParam) - resizeWidthDelta;
          resizeTop += GET_Y_LPARAM(msg->lParam) - resizeHeightDelta;
        break;
    } /* switch */
  } /* processMouseMove */



boolType isDeadKey (WPARAM virtualKey)

  {
    boolType deadKey = FALSE;

  /* isDeadKey */
    if (GetKeyState(VK_SHIFT) & 0x8000) {
      if ((virtualKey == VK_OEM_1 && shift_oem1_dead) ||
          (virtualKey == VK_OEM_2 && shift_oem2_dead) ||
          (virtualKey == VK_OEM_3 && shift_oem3_dead) ||
          (virtualKey == VK_OEM_4 && shift_oem4_dead) ||
          (virtualKey == VK_OEM_5 && shift_oem5_dead) ||
          (virtualKey == VK_OEM_6 && shift_oem6_dead) ||
          (virtualKey == VK_OEM_7 && shift_oem7_dead) ||
          (virtualKey == VK_OEM_8 && shift_oem8_dead)) {
        deadKey = TRUE;
      } /* if */
    } else {
      if ((virtualKey == VK_OEM_1 && oem1_dead) ||
          (virtualKey == VK_OEM_2 && oem2_dead) ||
          (virtualKey == VK_OEM_3 && oem3_dead) ||
          (virtualKey == VK_OEM_4 && oem4_dead) ||
          (virtualKey == VK_OEM_5 && oem5_dead) ||
          (virtualKey == VK_OEM_6 && oem6_dead) ||
          (virtualKey == VK_OEM_7 && oem7_dead) ||
          (virtualKey == VK_OEM_8 && oem8_dead)) {
        deadKey = TRUE;
      } /* if */
    } /* if */
    return deadKey;
  } /* isDeadKey */



boolType determineDeadKey (uint32Type virtualKey, boolType shift)

  {
    BYTE keyboardState[256];
    WCHAR unicodeChar[5];
    boolType deadKey = FALSE;

  /* determineDeadKey */
    memset(keyboardState, 0, 256);
    if (shift) {
      keyboardState[VK_SHIFT] = 128;
      keyboardState[VK_LSHIFT] = 128;
      keyboardState[VK_RSHIFT] = 128;
    } /* if */
    if (ToUnicode(virtualKey, MapVirtualKeyW(virtualKey, MAPVK_VK_TO_VSC),
                  keyboardState, unicodeChar, 4, 0) == -1) {
      deadKey = TRUE;
      /* To consume the dead key the function must be called again. */
      ToUnicode(virtualKey, MapVirtualKeyW(virtualKey, MAPVK_VK_TO_VSC),
                keyboardState, unicodeChar, 4, 0);
    } /* if */
    return deadKey;
  } /* determineDeadKey */



void gkbInitKeyboard (void)

  { /* gkbInitKeyboard */
    logFunction(printf("gkbInitKeyboard()\n"););
    oem1_dead = determineDeadKey(VK_OEM_1, FALSE);
    oem2_dead = determineDeadKey(VK_OEM_2, FALSE);
    oem3_dead = determineDeadKey(VK_OEM_3, FALSE);
    oem4_dead = determineDeadKey(VK_OEM_4, FALSE);
    oem5_dead = determineDeadKey(VK_OEM_5, FALSE);
    oem6_dead = determineDeadKey(VK_OEM_6, FALSE);
    oem7_dead = determineDeadKey(VK_OEM_7, FALSE);
    oem8_dead = determineDeadKey(VK_OEM_8, FALSE);
    shift_oem1_dead = determineDeadKey(VK_OEM_1, TRUE);
    shift_oem2_dead = determineDeadKey(VK_OEM_2, TRUE);
    shift_oem3_dead = determineDeadKey(VK_OEM_3, TRUE);
    shift_oem4_dead = determineDeadKey(VK_OEM_4, TRUE);
    shift_oem5_dead = determineDeadKey(VK_OEM_5, TRUE);
    shift_oem6_dead = determineDeadKey(VK_OEM_6, TRUE);
    shift_oem7_dead = determineDeadKey(VK_OEM_7, TRUE);
    shift_oem8_dead = determineDeadKey(VK_OEM_8, TRUE);
  } /* gkbInitKeyboard */



void gkbCloseKeyboard (void)

  { /* gkbCloseKeyboard */
    if (window_hash != NULL) {
      freeGenericHash(window_hash);
      window_hash = NULL;
    } /* if */
  } /* gkbCloseKeyboard */



charType gkbGetc (void)

  {
    BOOL bRet;
    MSG msg;
    charType result = K_NONE;

  /* gkbGetc */
    logFunction(printf("gkbGetc\n"););
    /* printf("before GetMessageW\n"); */
    bRet = GetMessageW(&msg, NULL, 0, 0);
    /* printf("after GetMessageW\n"); */
    while (result == K_NONE && bRet != 0) {
      if (bRet == -1) {
        logError(printf("GetMessageW(&msg, NULL, 0, 0)=-1\n"););
      } else {
        /* printf("gkbGetc message=%d %lu, %d, %x\n", msg.message, msg.hwnd, msg.wParam, msg.lParam); */
        switch (msg.message) {
          case WM_KEYDOWN:
            traceEvent(printf("gkbGetc: WM_KEYDOWN hwnd=" FMT_U_MEM
                              ", wParam=" FMT_U_MEM ", lParam=" FMT_X_MEM
                              ", SHIFT=%hx, CONTROL=%hx, MENU=%hx\n",
                              (memSizeType) msg.hwnd, msg.wParam, msg.lParam,
                              GetKeyState(VK_SHIFT), GetKeyState(VK_CONTROL),
                              GetKeyState(VK_MENU)););
            if (GetKeyState(VK_SHIFT) & 0x8000) {
              /* printf("VK_SHIFT\n"); */
              switch (msg.wParam) {
                case VK_RETURN:   result = K_SFT_NL;         break;
                case VK_BACK:     result = K_SFT_BS;         break;
                case VK_TAB:      result = K_BACKTAB;        break;
                case VK_ESCAPE:   result = K_SFT_ESC;        break;
                case VK_F1:       result = K_SFT_F1;         break;
                case VK_F2:       result = K_SFT_F2;         break;
                case VK_F3:       result = K_SFT_F3;         break;
                case VK_F4:       result = K_SFT_F4;         break;
                case VK_F5:       result = K_SFT_F5;         break;
                case VK_F6:       result = K_SFT_F6;         break;
                case VK_F7:       result = K_SFT_F7;         break;
                case VK_F8:       result = K_SFT_F8;         break;
                case VK_F9:       result = K_SFT_F9;         break;
                case VK_F10:      result = K_SFT_F10;        break;
                case VK_F11:      result = K_SFT_F11;        break;
                case VK_F12:      result = K_SFT_F12;        break;
                case VK_LEFT:     result = K_SFT_LEFT;       break;
                case VK_RIGHT:    result = K_SFT_RIGHT;      break;
                case VK_UP:       result = K_SFT_UP;         break;
                case VK_DOWN:     result = K_SFT_DOWN;       break;
                case VK_HOME:     result = K_SFT_HOME;       break;
                case VK_END:      result = K_SFT_END;        break;
                case VK_PRIOR:    result = K_SFT_PGUP;       break;
                case VK_NEXT:     result = K_SFT_PGDN;       break;
                case VK_CLEAR:    result = K_SFT_PAD_CENTER; break;
                case VK_INSERT:   result = K_SFT_INS;        break;
                case VK_DELETE:   result = K_SFT_DEL;        break;
                case VK_APPS:     result = K_SFT_MENU;       break;
                case VK_PRINT:    result = K_SFT_PRINT;      break;
                case VK_PAUSE:    result = K_SFT_PAUSE;      break;
                case VK_SHIFT:
                case VK_CONTROL:
                case VK_MENU:
                case VK_LWIN:
                case VK_RWIN:
                case VK_CAPITAL:
                case VK_NUMLOCK:
                case VK_SCROLL:   result = K_NONE;           break;
                case VK_OEM_1:
                case VK_OEM_2:
                case VK_OEM_3:
                case VK_OEM_4:
                case VK_OEM_5:
                case VK_OEM_6:
                case VK_OEM_7:
                case VK_OEM_8:
                  if (isDeadKey(msg.wParam)) {
                    if (processDeadKeys) {
                      if (deadKeyActive == 0) {
                        deadKeyActive = msg.wParam;
                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
                        result = K_NONE;
                      } else if (deadKeyActive == msg.wParam) {
                        result = K_UNDEF;
                      } else {
                        deadKeyActive = msg.wParam;
                        PostMessageW(msg.hwnd, WM_KEYDOWN, VK_SPACE, msg.lParam);
                        result = K_NONE;
                      } /* if */
                    } else {
                      TranslateMessage(&msg);
                      DispatchMessage(&msg);
                      PostMessageW(msg.hwnd, WM_KEYDOWN, VK_SPACE, msg.lParam);
                      result = K_NONE;
                    } /* if */
                  } else {
                    result = K_UNDEF;
                  } /* if */
                  break;
                default:
                  if (((GetKeyState(VK_CONTROL)  & 0x8000) != 0 &&
                       (GetKeyState(VK_RMENU)    & 0x8000) == 0) ||
                       (GetKeyState(VK_RCONTROL) & 0x8000) != 0) {
                    /* This condition checks for VK_CONTROL. */
                    /* Unfortunately Alt Gr is encoded as if */
                    /* left-control + right-alt are pressed. */
                    /* The Alt Gr situation is filtered out. */
                    switch (msg.wParam) {
                      case 'A':   result = K_CTL_A;          break;
                      case 'B':   result = K_CTL_B;          break;
                      case 'C':   result = K_CTL_C;          break;
                      case 'D':   result = K_CTL_D;          break;
                      case 'E':   result = K_CTL_E;          break;
                      case 'F':   result = K_CTL_F;          break;
                      case 'G':   result = K_CTL_G;          break;
                      case 'H':   result = K_CTL_H;          break;
                      case 'I':   result = K_CTL_I;          break;
                      case 'J':   result = K_CTL_J;          break;
                      case 'K':   result = K_CTL_K;          break;
                      case 'L':   result = K_CTL_L;          break;
                      case 'M':   result = K_CTL_M;          break;
                      case 'N':   result = K_CTL_N;          break;
                      case 'O':   result = K_CTL_O;          break;
                      case 'P':   result = K_CTL_P;          break;
                      case 'Q':   result = K_CTL_Q;          break;
                      case 'R':   result = K_CTL_R;          break;
                      case 'S':   result = K_CTL_S;          break;
                      case 'T':   result = K_CTL_T;          break;
                      case 'U':   result = K_CTL_U;          break;
                      case 'V':   result = K_CTL_V;          break;
                      case 'W':   result = K_CTL_W;          break;
                      case 'X':   result = K_CTL_X;          break;
                      case 'Y':   result = K_CTL_Y;          break;
                      case 'Z':   result = K_CTL_Z;          break;
                      case '0':   result = K_CTL_0;          break;
                      case '1':   result = K_CTL_1;          break;
                      case '2':   result = K_CTL_2;          break;
                      case '3':   result = K_CTL_3;          break;
                      case '4':   result = K_CTL_4;          break;
                      case '5':   result = K_CTL_5;          break;
                      case '6':   result = K_CTL_6;          break;
                      case '7':   result = K_CTL_7;          break;
                      case '8':   result = K_CTL_8;          break;
                      case '9':   result = K_CTL_9;          break;
                      default:    result = K_UNDEF;          break;
                    } /* switch */
                  } else {
                    result = K_UNDEF;
                  } /* if */
                  break;
              } /* switch */
            } else if (((GetKeyState(VK_CONTROL)  & 0x8000) != 0 &&
                        (GetKeyState(VK_RMENU)    & 0x8000) == 0) ||
                        (GetKeyState(VK_RCONTROL) & 0x8000) != 0) {
              /* This condition checks for VK_CONTROL. */
              /* Unfortunately Alt Gr is encoded as if */
              /* left-control + right-alt are pressed. */
              /* The Alt Gr situation is filtered out. */
              /* printf("VK_CONTROL\n"); */
              switch (msg.wParam) {
                case 'A':         result = K_CTL_A;          break;
                case 'B':         result = K_CTL_B;          break;
                case 'C':         result = K_CTL_C;          break;
                case 'D':         result = K_CTL_D;          break;
                case 'E':         result = K_CTL_E;          break;
                case 'F':         result = K_CTL_F;          break;
                case 'G':         result = K_CTL_G;          break;
                case 'H':         result = K_CTL_H;          break;
                case 'I':         result = K_CTL_I;          break;
                case 'J':         result = K_CTL_J;          break;
                case 'K':         result = K_CTL_K;          break;
                case 'L':         result = K_CTL_L;          break;
                case 'M':         result = K_CTL_M;          break;
                case 'N':         result = K_CTL_N;          break;
                case 'O':         result = K_CTL_O;          break;
                case 'P':         result = K_CTL_P;          break;
                case 'Q':         result = K_CTL_Q;          break;
                case 'R':         result = K_CTL_R;          break;
                case 'S':         result = K_CTL_S;          break;
                case 'T':         result = K_CTL_T;          break;
                case 'U':         result = K_CTL_U;          break;
                case 'V':         result = K_CTL_V;          break;
                case 'W':         result = K_CTL_W;          break;
                case 'X':         result = K_CTL_X;          break;
                case 'Y':         result = K_CTL_Y;          break;
                case 'Z':         result = K_CTL_Z;          break;
                case '0':         result = K_CTL_0;          break;
                case '1':         result = K_CTL_1;          break;
                case '2':         result = K_CTL_2;          break;
                case '3':         result = K_CTL_3;          break;
                case '4':         result = K_CTL_4;          break;
                case '5':         result = K_CTL_5;          break;
                case '6':         result = K_CTL_6;          break;
                case '7':         result = K_CTL_7;          break;
                case '8':         result = K_CTL_8;          break;
                case '9':         result = K_CTL_9;          break;
                case VK_RETURN:   result = K_CTL_NL;         break;
                case VK_BACK:     result = K_CTL_BS;         break;
                case VK_TAB:      result = K_CTL_TAB;        break;
                case VK_ESCAPE:   result = K_CTL_ESC;        break;
                case VK_F1:       result = K_CTL_F1;         break;
                case VK_F2:       result = K_CTL_F2;         break;
                case VK_F3:       result = K_CTL_F3;         break;
                case VK_F4:       result = K_CTL_F4;         break;
                case VK_F5:       result = K_CTL_F5;         break;
                case VK_F6:       result = K_CTL_F6;         break;
                case VK_F7:       result = K_CTL_F7;         break;
                case VK_F8:       result = K_CTL_F8;         break;
                case VK_F9:       result = K_CTL_F9;         break;
                case VK_F10:      result = K_CTL_F10;        break;
                case VK_F11:      result = K_CTL_F11;        break;
                case VK_F12:      result = K_CTL_F12;        break;
                case VK_NUMPAD0:  result = K_CTL_INS;        break;
                case VK_NUMPAD1:  result = K_CTL_END;        break;
                case VK_NUMPAD2:  result = K_CTL_DOWN;       break;
                case VK_NUMPAD3:  result = K_CTL_PGDN;       break;
                case VK_NUMPAD4:  result = K_CTL_LEFT;       break;
                case VK_NUMPAD5:  result = K_CTL_PAD_CENTER; break;
                case VK_NUMPAD6:  result = K_CTL_RIGHT;      break;
                case VK_NUMPAD7:  result = K_CTL_HOME;       break;
                case VK_NUMPAD8:  result = K_CTL_UP;         break;
                case VK_NUMPAD9:  result = K_CTL_PGUP;       break;
                case VK_DECIMAL:  result = K_CTL_DEL;        break;
                case VK_LEFT:     result = K_CTL_LEFT;       break;
                case VK_RIGHT:    result = K_CTL_RIGHT;      break;
                case VK_UP:       result = K_CTL_UP;         break;
                case VK_DOWN:     result = K_CTL_DOWN;       break;
                case VK_HOME:     result = K_CTL_HOME;       break;
                case VK_END:      result = K_CTL_END;        break;
                case VK_PRIOR:    result = K_CTL_PGUP;       break;
                case VK_NEXT:     result = K_CTL_PGDN;       break;
                case VK_CLEAR:    result = K_CTL_PAD_CENTER; break;
                case VK_INSERT:   result = K_CTL_INS;        break;
                case VK_DELETE:   result = K_CTL_DEL;        break;
                case VK_APPS:     result = K_CTL_MENU;       break;
                case VK_PRINT:    result = K_CTL_PRINT;      break;
                case VK_PAUSE:    result = K_CTL_PAUSE;      break;
                case VK_SHIFT:
                case VK_CONTROL:
                case VK_MENU:
                case VK_LWIN:
                case VK_RWIN:
                case VK_CAPITAL:
                case VK_NUMLOCK:
                case VK_SCROLL:   result = K_NONE;           break;
                default:          result = K_UNDEF;          break;
              } /* switch */
            } else if (GetKeyState(VK_MENU) & 0x8000) {
              /* This condition is true for the Alt key and the Alt Gr key. */
              /* printf("VK_MENU\n"); */
              switch (msg.wParam) {
                case VK_RETURN:   result = K_ALT_NL;         break;
                case VK_BACK:     result = K_ALT_BS;         break;
                case VK_TAB:      result = K_ALT_TAB;        break;
                case VK_ESCAPE:   result = K_ALT_ESC;        break;
                case VK_F1:       result = K_ALT_F1;         break;
                case VK_F2:       result = K_ALT_F2;         break;
                case VK_F3:       result = K_ALT_F3;         break;
                case VK_F4:       result = K_ALT_F4;         break;
                case VK_F5:       result = K_ALT_F5;         break;
                case VK_F6:       result = K_ALT_F6;         break;
                case VK_F7:       result = K_ALT_F7;         break;
                case VK_F8:       result = K_ALT_F8;         break;
                case VK_F9:       result = K_ALT_F9;         break;
                case VK_F10:      result = K_ALT_F10;        break;
                case VK_F11:      result = K_ALT_F11;        break;
                case VK_F12:      result = K_ALT_F12;        break;
                case VK_NUMPAD0:  result = K_ALT_INS;        break;
                case VK_NUMPAD1:  result = K_ALT_END;        break;
                case VK_NUMPAD2:  result = K_ALT_DOWN;       break;
                case VK_NUMPAD3:  result = K_ALT_PGDN;       break;
                case VK_NUMPAD4:  result = K_ALT_LEFT;       break;
                case VK_NUMPAD5:  result = K_ALT_PAD_CENTER; break;
                case VK_NUMPAD6:  result = K_ALT_RIGHT;      break;
                case VK_NUMPAD7:  result = K_ALT_HOME;       break;
                case VK_NUMPAD8:  result = K_ALT_UP;         break;
                case VK_NUMPAD9:  result = K_ALT_PGUP;       break;
                case VK_DECIMAL:  result = K_ALT_DEL;        break;
                case VK_LEFT:     result = K_ALT_LEFT;       break;
                case VK_RIGHT:    result = K_ALT_RIGHT;      break;
                case VK_UP:       result = K_ALT_UP;         break;
                case VK_DOWN:     result = K_ALT_DOWN;       break;
                case VK_HOME:     result = K_ALT_HOME;       break;
                case VK_END:      result = K_ALT_END;        break;
                case VK_PRIOR:    result = K_ALT_PGUP;       break;
                case VK_NEXT:     result = K_ALT_PGDN;       break;
                case VK_CLEAR:    result = K_ALT_PAD_CENTER; break;
                case VK_INSERT:   result = K_ALT_INS;        break;
                case VK_DELETE:   result = K_ALT_DEL;        break;
                case VK_APPS:     result = K_ALT_MENU;       break;
                case VK_PRINT:    result = K_ALT_PRINT;      break;
                case VK_PAUSE:    result = K_ALT_PAUSE;      break;
                case VK_SHIFT:
                case VK_CONTROL:
                case VK_MENU:
                case VK_LWIN:
                case VK_RWIN:
                case VK_CAPITAL:
                case VK_NUMLOCK:
                case VK_SCROLL:   result = K_NONE;           break;
                default:          result = K_UNDEF;          break;
              } /* switch */
            } else if ((GetKeyState(VK_NUMLOCK) & 0x0001) != 0) {
              /* printf("VK_NUMLOCK\n"); */
              if (msg.lParam & 0x1000000) {
                /* The key is a cursor key but not from the numeric keypad. */
                switch (msg.wParam) {
                  case VK_RETURN:   result = K_NL;         break;
                  case VK_BACK:     result = K_BS;         break;
                  case VK_TAB:      result = K_TAB;        break;
                  case VK_ESCAPE:   result = K_ESC;        break;
                  case VK_LEFT:     result = K_LEFT;       break;
                  case VK_RIGHT:    result = K_RIGHT;      break;
                  case VK_UP:       result = K_UP;         break;
                  case VK_DOWN:     result = K_DOWN;       break;
                  case VK_HOME:     result = K_HOME;       break;
                  case VK_END:      result = K_END;        break;
                  case VK_PRIOR:    result = K_PGUP;       break;
                  case VK_NEXT:     result = K_PGDN;       break;
                  case VK_CLEAR:    result = K_PAD_CENTER; break;
                  case VK_INSERT:   result = K_INS;        break;
                  case VK_DELETE:   result = K_DEL;        break;
                  case VK_APPS:     result = K_MENU;       break;
                  case VK_PRINT:    result = K_PRINT;      break;
                  case VK_PAUSE:    result = K_PAUSE;      break;
                  case VK_SHIFT:
                  case VK_CONTROL:
                  case VK_MENU:
                  case VK_LWIN:
                  case VK_RWIN:
                  case VK_CAPITAL:
                  case VK_NUMLOCK:
                  case VK_SCROLL:   result = K_NONE;       break;
                  default:          result = K_UNDEF;      break;
                } /* switch */
              } else {
                switch (msg.wParam) {
                  case VK_RETURN:   result = K_NL;             break;
                  case VK_BACK:     result = K_BS;             break;
                  case VK_TAB:      result = K_TAB;            break;
                  case VK_ESCAPE:   result = K_ESC;            break;
                  case VK_F1:       result = K_F1;             break;
                  case VK_F2:       result = K_F2;             break;
                  case VK_F3:       result = K_F3;             break;
                  case VK_F4:       result = K_F4;             break;
                  case VK_F5:       result = K_F5;             break;
                  case VK_F6:       result = K_F6;             break;
                  case VK_F7:       result = K_F7;             break;
                  case VK_F8:       result = K_F8;             break;
                  case VK_F9:       result = K_F9;             break;
                  case VK_F10:      result = K_F10;            break;
                  case VK_F11:      result = K_F11;            break;
                  case VK_F12:      result = K_F12;            break;
                  case VK_LEFT:     result = K_SFT_LEFT;       break;
                  case VK_RIGHT:    result = K_SFT_RIGHT;      break;
                  case VK_UP:       result = K_SFT_UP;         break;
                  case VK_DOWN:     result = K_SFT_DOWN;       break;
                  case VK_HOME:     result = K_SFT_HOME;       break;
                  case VK_END:      result = K_SFT_END;        break;
                  case VK_PRIOR:    result = K_SFT_PGUP;       break;
                  case VK_NEXT:     result = K_SFT_PGDN;       break;
                  case VK_CLEAR:    result = K_SFT_PAD_CENTER; break;
                  case VK_INSERT:   result = K_SFT_INS;        break;
                  case VK_DELETE:   result = K_SFT_DEL;        break;
                  case VK_APPS:     result = K_MENU;           break;
                  case VK_PRINT:    result = K_PRINT;          break;
                  case VK_PAUSE:    result = K_PAUSE;          break;
                  case VK_SHIFT:
                  case VK_CONTROL:
                  case VK_MENU:
                  case VK_LWIN:
                  case VK_RWIN:
                  case VK_CAPITAL:
                  case VK_NUMLOCK:
                  case VK_SCROLL:   result = K_NONE;           break;
                  default:          result = K_UNDEF;          break;
                } /* switch */
              } /* if */
            } else {
              /* printf("no key state\n"); */
              switch (msg.wParam) {
                case VK_RETURN:   result = K_NL;         break;
                case VK_ESCAPE:   result = K_ESC;        break;
                case VK_F1:       result = K_F1;         break;
                case VK_F2:       result = K_F2;         break;
                case VK_F3:       result = K_F3;         break;
                case VK_F4:       result = K_F4;         break;
                case VK_F5:       result = K_F5;         break;
                case VK_F6:       result = K_F6;         break;
                case VK_F7:       result = K_F7;         break;
                case VK_F8:       result = K_F8;         break;
                case VK_F9:       result = K_F9;         break;
                case VK_F10:      result = K_F10;        break;
                case VK_F11:      result = K_F11;        break;
                case VK_F12:      result = K_F12;        break;
                case VK_LEFT:     result = K_LEFT;       break;
                case VK_RIGHT:    result = K_RIGHT;      break;
                case VK_UP:       result = K_UP;         break;
                case VK_DOWN:     result = K_DOWN;       break;
                case VK_HOME:     result = K_HOME;       break;
                case VK_END:      result = K_END;        break;
                case VK_PRIOR:    result = K_PGUP;       break;
                case VK_NEXT:     result = K_PGDN;       break;
                case VK_CLEAR:    result = K_PAD_CENTER; break;
                case VK_INSERT:   result = K_INS;        break;
                case VK_DELETE:   result = K_DEL;        break;
                case VK_APPS:     result = K_MENU;       break;
                case VK_PRINT:    result = K_PRINT;      break;
                case VK_PAUSE:    result = K_PAUSE;      break;
                case VK_SHIFT:
                case VK_CONTROL:
                case VK_MENU:
                case VK_LWIN:
                case VK_RWIN:
                case VK_CAPITAL:
                case VK_NUMLOCK:
                case VK_SCROLL:   result = K_NONE;       break;
                case VK_OEM_1:
                case VK_OEM_2:
                case VK_OEM_3:
                case VK_OEM_4:
                case VK_OEM_5:
                case VK_OEM_6:
                case VK_OEM_7:
                case VK_OEM_8:
                  if (isDeadKey(msg.wParam)) {
                    if (processDeadKeys) {
                      if (deadKeyActive == 0) {
                        deadKeyActive = msg.wParam;
                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
                        result = K_NONE;
                      } else if (deadKeyActive == msg.wParam) {
                        result = K_UNDEF;
                      } else {
                        deadKeyActive = msg.wParam;
                        PostMessageW(msg.hwnd, WM_KEYDOWN, VK_SPACE, msg.lParam);
                        result = K_NONE;
                      } /* if */
                    } else {
                      TranslateMessage(&msg);
                      DispatchMessage(&msg);
                      PostMessageW(msg.hwnd, WM_KEYDOWN, VK_SPACE, msg.lParam);
                      result = K_NONE;
                    } /* if */
                  } else {
                    result = K_UNDEF;
                  } /* if */
                  break;
                default:          result = K_UNDEF;      break;
              } /* switch */
            } /* if */
            if (result == K_UNDEF) {
              TranslateMessage(&msg);
              if (PeekMessageW(&msg, NULL, 0, 0, PM_NOREMOVE)) {
                if (msg.message == WM_CHAR) {
                  result = K_NONE;
                } /* if */
              } /* if */
            } /* if */
            break;
          case WM_LBUTTONDOWN:
            traceEvent(printf("gkbGetc: WM_LBUTTONDOWN hwnd=" FMT_U_MEM
                              ", wParam=" FMT_X64 ", lParam=" FMT_X_MEM "\n",
                              (memSizeType) msg.hwnd, msg.wParam, msg.lParam););
            clicked_x = GET_X_LPARAM(msg.lParam);
            clicked_y = GET_Y_LPARAM(msg.lParam);
            clickedWindow = msg.hwnd;
            if (msg.wParam & 0x04) {
              result = K_SFT_MOUSE1;
            } else if (((msg.wParam & 0x08) != 0 &&
                        (GetKeyState(VK_RMENU)    & 0x8000) == 0) ||
                        (GetKeyState(VK_RCONTROL) & 0x8000) != 0) {
              /* This condition checks for VK_CONTROL. */
              /* Unfortunately Alt Gr is encoded as if */
              /* left-control + right-alt are pressed. */
              /* The Alt Gr situation is filtered out. */
              result = K_CTL_MOUSE1;
            } else if (GetKeyState(VK_MENU) & 0x8000) {
              result = K_ALT_MOUSE1;
            } else {
              result = K_MOUSE1;
            } /* if */
            break;
          case WM_MBUTTONDOWN:
            traceEvent(printf("gkbGetc: WM_MBUTTONDOWN hwnd=" FMT_U_MEM
                              ", wParam=" FMT_X64 ", lParam=" FMT_X_MEM "\n",
                              (memSizeType) msg.hwnd, msg.wParam, msg.lParam););
            clicked_x = GET_X_LPARAM(msg.lParam);
            clicked_y = GET_Y_LPARAM(msg.lParam);
            clickedWindow = msg.hwnd;
            if (msg.wParam & 0x04) {
              result = K_SFT_MOUSE2;
            } else if (((msg.wParam & 0x08) != 0 &&
                        (GetKeyState(VK_RMENU)    & 0x8000) == 0) ||
                        (GetKeyState(VK_RCONTROL) & 0x8000) != 0) {
              /* This condition checks for VK_CONTROL. */
              /* Unfortunately Alt Gr is encoded as if */
              /* left-control + right-alt are pressed. */
              /* The Alt Gr situation is filtered out. */
              result = K_CTL_MOUSE2;
            } else if (GetKeyState(VK_MENU) & 0x8000) {
              result = K_ALT_MOUSE2;
            } else {
              result = K_MOUSE2;
            } /* if */
            break;
          case WM_RBUTTONDOWN:
            traceEvent(printf("gkbGetc: WM_RBUTTONDOWN hwnd=" FMT_U_MEM
                              ", wParam=" FMT_X64 ", lParam=" FMT_X_MEM "\n",
                              (memSizeType) msg.hwnd, msg.wParam, msg.lParam););
            clicked_x = GET_X_LPARAM(msg.lParam);
            clicked_y = GET_Y_LPARAM(msg.lParam);
            clickedWindow = msg.hwnd;
            if (msg.wParam & 0x04) {
              result = K_SFT_MOUSE3;
            } else if (((msg.wParam & 0x08) != 0 &&
                        (GetKeyState(VK_RMENU)    & 0x8000) == 0) ||
                        (GetKeyState(VK_RCONTROL) & 0x8000) != 0) {
              /* This condition checks for VK_CONTROL. */
              /* Unfortunately Alt Gr is encoded as if */
              /* left-control + right-alt are pressed. */
              /* The Alt Gr situation is filtered out. */
              result = K_CTL_MOUSE3;
            } else if (GetKeyState(VK_MENU) & 0x8000) {
              result = K_ALT_MOUSE3;
            } else {
              result = K_MOUSE3;
            } /* if */
            break;
          case WM_XBUTTONDOWN:
            traceEvent(printf("gkbGetc: WM_XBUTTONDOWN hwnd=" FMT_U_MEM
                              ", wParam=" FMT_X64 ", lParam=" FMT_X_MEM "\n",
                              (memSizeType) msg.hwnd, msg.wParam, msg.lParam););
            clicked_x = GET_X_LPARAM(msg.lParam);
            clicked_y = GET_Y_LPARAM(msg.lParam);
            clickedWindow = msg.hwnd;
            if (GET_XBUTTON_WPARAM(msg.wParam) == XBUTTON2) {
              if (msg.wParam & 0x04) {
                result = K_SFT_MOUSE_FWD;
              } else if (((msg.wParam & 0x08) != 0 &&
                          (GetKeyState(VK_RMENU)    & 0x8000) == 0) ||
                          (GetKeyState(VK_RCONTROL) & 0x8000) != 0) {
                /* This condition checks for VK_CONTROL. */
                /* Unfortunately Alt Gr is encoded as if */
                /* left-control + right-alt are pressed. */
                /* The Alt Gr situation is filtered out. */
                result = K_CTL_MOUSE_FWD;
              } else if (GetKeyState(VK_MENU) & 0x8000) {
                result = K_ALT_MOUSE_FWD;
              } else {
                result = K_MOUSE_FWD;
              } /* if */
            } else if (GET_XBUTTON_WPARAM(msg.wParam) == XBUTTON1) {
              if (msg.wParam & 0x04) {
                result = K_SFT_MOUSE_BACK;
              } else if (((msg.wParam & 0x08) != 0 &&
                          (GetKeyState(VK_RMENU)    & 0x8000) == 0) ||
                          (GetKeyState(VK_RCONTROL) & 0x8000) != 0) {
                /* This condition checks for VK_CONTROL. */
                /* Unfortunately Alt Gr is encoded as if */
                /* left-control + right-alt are pressed. */
                /* The Alt Gr situation is filtered out. */
                result = K_CTL_MOUSE_BACK;
              } else if (GetKeyState(VK_MENU) & 0x8000) {
                result = K_ALT_MOUSE_BACK;
              } else {
                result = K_MOUSE_BACK;
              } /* if */
            } else {
              result = K_UNDEF;
            } /* if */
            break;
          case WM_MOUSEWHEEL:
            traceEvent(printf("gkbGetc: WM_MOUSEWHEEL hwnd=" FMT_U_MEM
                              ", wParam=" FMT_X64 ", lParam=" FMT_X_MEM "\n",
                              (memSizeType) msg.hwnd, msg.wParam, msg.lParam););
            if (IsWindow(msg.hwnd)) {
              POINT point;
              winType win;

              point.x = GET_X_LPARAM(msg.lParam);
              point.y = GET_Y_LPARAM(msg.lParam);
              ScreenToClient(msg.hwnd, &point);
              win = find_window(msg.hwnd);
              if (point.x >= 0 && point.x < drwWidth(win) &&
                  point.y >= 0 && point.y < drwHeight(win)) {
                clicked_x = point.x;
                clicked_y = point.y;
                clickedWindow = msg.hwnd;
                if (GET_WHEEL_DELTA_WPARAM(msg.wParam) > 0) {
                  if (msg.wParam & 0x04) {
                    result = K_SFT_MOUSE4;
                  } else if (((msg.wParam & 0x08) != 0 &&
                              (GetKeyState(VK_RMENU)    & 0x8000) == 0) ||
                              (GetKeyState(VK_RCONTROL) & 0x8000) != 0) {
                    /* This condition checks for VK_CONTROL. */
                    /* Unfortunately Alt Gr is encoded as if */
                    /* left-control + right-alt are pressed. */
                    /* The Alt Gr situation is filtered out. */
                    result = K_CTL_MOUSE4;
                  } else if (GetKeyState(VK_MENU) & 0x8000) {
                    result = K_ALT_MOUSE4;
                  } else {
                    result = K_MOUSE4;
                  } /* if */
                } else {
                  if (msg.wParam & 0x04) {
                    result = K_SFT_MOUSE5;
                  } else if (((msg.wParam & 0x08) != 0 &&
                              (GetKeyState(VK_RMENU)    & 0x8000) == 0) ||
                              (GetKeyState(VK_RCONTROL) & 0x8000) != 0) {
                    /* This condition checks for VK_CONTROL. */
                    /* Unfortunately Alt Gr is encoded as if */
                    /* left-control + right-alt are pressed. */
                    /* The Alt Gr situation is filtered out. */
                    result = K_CTL_MOUSE5;
                  } else if (GetKeyState(VK_MENU) & 0x8000) {
                    result = K_ALT_MOUSE5;
                  } else {
                    result = K_MOUSE5;
                  } /* if */
                } /* if */
              } else {
                result = K_NONE;
              } /* if */
            } else {
              result = K_NONE;
            } /* if */
            break;
          case WM_SYSKEYDOWN:
            traceEvent(printf("gkbGetc: WM_SYSKEYDOWN hwnd=" FMT_U_MEM
                              ", wParam=" FMT_U_MEM ", lParam=" FMT_X_MEM
                              ", SHIFT=%hx, CONTROL=%hx, MENU=%hx\n",
                              (memSizeType) msg.hwnd, msg.wParam, msg.lParam,
                              GetKeyState(VK_SHIFT), GetKeyState(VK_CONTROL),
                              GetKeyState(VK_MENU)););
            if (GetKeyState(VK_SHIFT) & 0x8000) {
              /* Shift is pressed together with alt. */
              switch (msg.wParam) {
                case VK_RETURN:   result = K_SFT_NL;         break;
                case VK_BACK:     result = K_SFT_BS;         break;
                case VK_TAB:      result = K_BACKTAB;        break;
                case VK_ESCAPE:   result = K_SFT_ESC;        break;
                case VK_F1:       result = K_SFT_F1;         break;
                case VK_F2:       result = K_SFT_F2;         break;
                case VK_F3:       result = K_SFT_F3;         break;
                case VK_F4:       result = K_SFT_F4;         break;
                case VK_F5:       result = K_SFT_F5;         break;
                case VK_F6:       result = K_SFT_F6;         break;
                case VK_F7:       result = K_SFT_F7;         break;
                case VK_F8:       result = K_SFT_F8;         break;
                case VK_F9:       result = K_SFT_F9;         break;
                case VK_F10:      result = K_SFT_F10;        break;
                case VK_F11:      result = K_SFT_F11;        break;
                case VK_F12:      result = K_SFT_F12;        break;
                case VK_LEFT:     result = K_SFT_LEFT;       break;
                case VK_RIGHT:    result = K_SFT_RIGHT;      break;
                case VK_UP:       result = K_SFT_UP;         break;
                case VK_DOWN:     result = K_SFT_DOWN;       break;
                case VK_HOME:     result = K_SFT_HOME;       break;
                case VK_END:      result = K_SFT_END;        break;
                case VK_PRIOR:    result = K_SFT_PGUP;       break;
                case VK_NEXT:     result = K_SFT_PGDN;       break;
                case VK_CLEAR:    result = K_SFT_PAD_CENTER; break;
                case VK_INSERT:   result = K_SFT_INS;        break;
                case VK_DELETE:   result = K_SFT_DEL;        break;
                case VK_APPS:     result = K_SFT_MENU;       break;
                case VK_PRINT:    result = K_SFT_PRINT;      break;
                case VK_PAUSE:    result = K_SFT_PAUSE;      break;
                case VK_SHIFT:
                case VK_CONTROL:
                case VK_MENU:
                case VK_LWIN:
                case VK_RWIN:
                case VK_CAPITAL:
                case VK_NUMLOCK:
                case VK_SCROLL:   result = K_NONE;           break;
                default:
                  /* printf("WM_SYSKEYDOWN %lu, %d %x\n", msg.hwnd, msg.wParam, msg.lParam); */
                  result = K_UNDEF;
                  break;
              } /* switch */
            } else if (msg.lParam & 0x1000000) {
              /* The key is a cursor key but not from the numeric keypad. */
              switch (msg.wParam) {
                case VK_RETURN:   result = K_ALT_NL;         break;
                case VK_BACK:     result = K_ALT_BS;         break;
                case VK_TAB:      result = K_ALT_TAB;        break;
                case VK_ESCAPE:   result = K_ALT_ESC;        break;
                case VK_LEFT:     result = K_ALT_LEFT;       break;
                case VK_RIGHT:    result = K_ALT_RIGHT;      break;
                case VK_UP:       result = K_ALT_UP;         break;
                case VK_DOWN:     result = K_ALT_DOWN;       break;
                case VK_HOME:     result = K_ALT_HOME;       break;
                case VK_END:      result = K_ALT_END;        break;
                case VK_PRIOR:    result = K_ALT_PGUP;       break;
                case VK_NEXT:     result = K_ALT_PGDN;       break;
                case VK_CLEAR:    result = K_ALT_PAD_CENTER; break;
                case VK_INSERT:   result = K_ALT_INS;        break;
                case VK_DELETE:   result = K_ALT_DEL;        break;
                case VK_APPS:     result = K_ALT_MENU;       break;
                case VK_PRINT:    result = K_ALT_PRINT;      break;
                case VK_PAUSE:    result = K_ALT_PAUSE;      break;
                case VK_SHIFT:
                case VK_CONTROL:
                case VK_MENU:
                case VK_LWIN:
                case VK_RWIN:
                case VK_CAPITAL:
                case VK_NUMLOCK:
                case VK_SCROLL:   result = K_NONE;           break;
                default:
                  /* printf("WM_SYSKEYDOWN %lu, %d %x\n", msg.hwnd, msg.wParam, msg.lParam); */
                  result = K_UNDEF;
                  break;
              } /* switch */
            } else {
              switch (msg.wParam) {
                case 'A':         result = K_ALT_A;          break;
                case 'B':         result = K_ALT_B;          break;
                case 'C':         result = K_ALT_C;          break;
                case 'D':         result = K_ALT_D;          break;
                case 'E':         result = K_ALT_E;          break;
                case 'F':         result = K_ALT_F;          break;
                case 'G':         result = K_ALT_G;          break;
                case 'H':         result = K_ALT_H;          break;
                case 'I':         result = K_ALT_I;          break;
                case 'J':         result = K_ALT_J;          break;
                case 'K':         result = K_ALT_K;          break;
                case 'L':         result = K_ALT_L;          break;
                case 'M':         result = K_ALT_M;          break;
                case 'N':         result = K_ALT_N;          break;
                case 'O':         result = K_ALT_O;          break;
                case 'P':         result = K_ALT_P;          break;
                case 'Q':         result = K_ALT_Q;          break;
                case 'R':         result = K_ALT_R;          break;
                case 'S':         result = K_ALT_S;          break;
                case 'T':         result = K_ALT_T;          break;
                case 'U':         result = K_ALT_U;          break;
                case 'V':         result = K_ALT_V;          break;
                case 'W':         result = K_ALT_W;          break;
                case 'X':         result = K_ALT_X;          break;
                case 'Y':         result = K_ALT_Y;          break;
                case 'Z':         result = K_ALT_Z;          break;
                case '0':         result = K_ALT_0;          break;
                case '1':         result = K_ALT_1;          break;
                case '2':         result = K_ALT_2;          break;
                case '3':         result = K_ALT_3;          break;
                case '4':         result = K_ALT_4;          break;
                case '5':         result = K_ALT_5;          break;
                case '6':         result = K_ALT_6;          break;
                case '7':         result = K_ALT_7;          break;
                case '8':         result = K_ALT_8;          break;
                case '9':         result = K_ALT_9;          break;
                case VK_RETURN:   result = K_ALT_NL;         break;
                case VK_BACK:     result = K_ALT_BS;         break;
                case VK_TAB:      result = K_ALT_TAB;        break;
                case VK_ESCAPE:   result = K_ALT_ESC;        break;
                case VK_NUMPAD0:  result = K_ALT_INS;        break;
                case VK_NUMPAD1:  result = K_ALT_END;        break;
                case VK_NUMPAD2:  result = K_ALT_DOWN;       break;
                case VK_NUMPAD3:  result = K_ALT_PGDN;       break;
                case VK_NUMPAD4:  result = K_ALT_LEFT;       break;
                case VK_NUMPAD5:  result = K_ALT_PAD_CENTER; break;
                case VK_NUMPAD6:  result = K_ALT_RIGHT;      break;
                case VK_NUMPAD7:  result = K_ALT_HOME;       break;
                case VK_NUMPAD8:  result = K_ALT_UP;         break;
                case VK_NUMPAD9:  result = K_ALT_PGUP;       break;
                case VK_DECIMAL:  result = K_ALT_DEL;        break;
                case VK_LEFT:     result = K_ALT_LEFT;       break;
                case VK_RIGHT:    result = K_ALT_RIGHT;      break;
                case VK_UP:       result = K_ALT_UP;         break;
                case VK_DOWN:     result = K_ALT_DOWN;       break;
                case VK_HOME:     result = K_ALT_HOME;       break;
                case VK_END:      result = K_ALT_END;        break;
                case VK_PRIOR:    result = K_ALT_PGUP;       break;
                case VK_NEXT:     result = K_ALT_PGDN;       break;
                case VK_CLEAR:    result = K_ALT_PAD_CENTER; break;
                case VK_INSERT:   result = K_ALT_INS;        break;
                case VK_DELETE:   result = K_ALT_DEL;        break;
                case VK_APPS:     result = K_ALT_MENU;       break;
                case VK_PRINT:    result = K_ALT_PRINT;      break;
                case VK_PAUSE:    result = K_ALT_PAUSE;      break;
                case VK_F1:       result = K_ALT_F1;         break;
                case VK_F2:       result = K_ALT_F2;         break;
                case VK_F3:       result = K_ALT_F3;         break;
                case VK_F4:       result = K_ALT_F4;         break;
                case VK_F5:       result = K_ALT_F5;         break;
                case VK_F6:       result = K_ALT_F6;         break;
                case VK_F7:       result = K_ALT_F7;         break;
                case VK_F8:       result = K_ALT_F8;         break;
                case VK_F9:       result = K_ALT_F9;         break;
                case VK_F10:
                  if (msg.lParam & 0x20000000) {
                    result = K_ALT_F10;
                  } else if (GetKeyState(VK_SHIFT) & 0x8000) {
                    result = K_SFT_F10;
                  } else if (GetKeyState(VK_CONTROL) & 0x8000) {
                    result = K_CTL_F10;
                  } else {
                    result = K_F10;
                  } /* if */
                  break;
                case VK_F11:      result = K_ALT_F11;        break;
                case VK_F12:      result = K_ALT_F12;        break;
                case VK_SHIFT:
                case VK_CONTROL:
                case VK_MENU:
                case VK_LWIN:
                case VK_RWIN:
                case VK_CAPITAL:
                case VK_NUMLOCK:
                case VK_SCROLL:   result = K_NONE;           break;
                default:
                  /* printf("WM_SYSKEYDOWN %lu, %d %x\n", msg.hwnd, msg.wParam, msg.lParam); */
                  result = K_UNDEF;
                  break;
              } /* switch */
            } /* if */
            break;
          case WM_NCLBUTTONDOWN:
            traceEvent(printf("gkbGetc: WM_NCLBUTTONDOWN hwnd=" FMT_U_MEM
                              ", wParam=" FMT_U_MEM ", lParam=" FMT_X_MEM
                              ", SHIFT=%hx, CONTROL=%hx, MENU=%hx\n",
                              (memSizeType) msg.hwnd, msg.wParam, msg.lParam,
                              GetKeyState(VK_SHIFT), GetKeyState(VK_CONTROL),
                              GetKeyState(VK_MENU)););
            if (msg.wParam == HTCLOSE && IsWindow(msg.hwnd)) {
              /* printf("HTCLOSE\n"); */
              switch (getCloseAction(find_window(msg.hwnd))) {
                case CLOSE_BUTTON_CLOSES_PROGRAM:
                  os_exit(0);
                  break;
                case CLOSE_BUTTON_RETURNS_KEY:
                  result = K_CLOSE;
                  clickedWindow = msg.hwnd;
                  break;
                case CLOSE_BUTTON_RAISES_EXCEPTION:
                  raise_error(GRAPHIC_ERROR);
                  result = K_CLOSE;
                  break;
              } /* switch */
            } else if (msg.wParam == HTBOTTOMRIGHT || msg.wParam == HTRIGHT || msg.wParam == HTBOTTOM) {
              resizeBottomAndRight(&msg);
            } else if (msg.wParam == HTTOPLEFT || msg.wParam == HTLEFT || msg.wParam == HTTOP ||
                msg.wParam == HTTOPRIGHT || msg.wParam == HTBOTTOMLEFT) {
              resizeTopAndLeft(&msg);
            } else if (msg.wParam == HTCAPTION) {
              startMoveWindow(&msg);
            } else {
              TranslateMessage(&msg);
              DispatchMessage(&msg);
            } /* if */
            break;
          case WM_SYSCOMMAND:
            traceEvent(printf("gkbGetc: WM_SYSCOMMAND hwnd=" FMT_U_MEM
                              ", wParam=" FMT_U_MEM ", lParam=" FMT_X_MEM
                              ", SHIFT=%hx, CONTROL=%hx, MENU=%hx\n",
                              (memSizeType) msg.hwnd, msg.wParam, msg.lParam,
                              GetKeyState(VK_SHIFT), GetKeyState(VK_CONTROL),
                              GetKeyState(VK_MENU)););
            if ((msg.wParam & 0xfff0) == SC_CLOSE && IsWindow(msg.hwnd)) {
              /* printf("SC_CLOSE\n"); */
              switch (getCloseAction(find_window(msg.hwnd))) {
                case CLOSE_BUTTON_CLOSES_PROGRAM:
                  os_exit(0);
                  break;
                case CLOSE_BUTTON_RETURNS_KEY:
                  result = K_CLOSE;
                  clickedWindow = msg.hwnd;
                  break;
                case CLOSE_BUTTON_RAISES_EXCEPTION:
                  raise_error(GRAPHIC_ERROR);
                  result = K_CLOSE;
                  break;
              } /* switch */
            } else if ((msg.wParam & 0xfff0) == SC_SIZE) {
              /* printf("SC_SIZE\n"); */
              systemSizeCommand(&msg);
            } else if ((msg.wParam & 0xfff0) == SC_MOVE) {
              /* printf("SC_MOVE\n"); */
              systemMoveCommand(&msg);
            } else {
              TranslateMessage(&msg);
              DispatchMessage(&msg);
            } /* if */
            break;
          case WM_MOUSEMOVE:
            traceEvent(printf("gkbGetc: WM_MOUSEMOVE hwnd=" FMT_U_MEM
                              ", wParam=" FMT_U_MEM ", lParam=" FMT_X_MEM
                              ", SHIFT=%hx, CONTROL=%hx, MENU=%hx\n",
                              (memSizeType) msg.hwnd, msg.wParam, msg.lParam,
                              GetKeyState(VK_SHIFT), GetKeyState(VK_CONTROL),
                              GetKeyState(VK_MENU)););
            if (resizeMode != 0) {
              processMouseMove(&msg);
            } else {
              TranslateMessage(&msg);
              DispatchMessage(&msg);
            } /* if */
            break;
          case WM_LBUTTONUP:
            traceEvent(printf("gkbGetc: WM_LBUTTONUP hwnd=" FMT_U_MEM
                              ", wParam=" FMT_U_MEM ", lParam=" FMT_X_MEM
                              ", SHIFT=%hx, CONTROL=%hx, MENU=%hx\n",
                              (memSizeType) msg.hwnd, msg.wParam, msg.lParam,
                              GetKeyState(VK_SHIFT), GetKeyState(VK_CONTROL),
                              GetKeyState(VK_MENU)););
            if (resizeMode != 0) {
              /* printf("resizeMode: %d\n", (int) resizeMode); */
              ReleaseCapture();
              resizeMode = 0;
            } else {
              TranslateMessage(&msg);
              DispatchMessage(&msg);
            } /* if */
            break;
          case WM_CHAR:
            traceEvent(printf("gkbGetc: WM_CHAR hwnd=" FMT_U_MEM
                              ", wParam=" FMT_U_MEM ", lParam=" FMT_X_MEM "\n",
                              (memSizeType) msg.hwnd, msg.wParam, msg.lParam););
            result = (charType) msg.wParam;
            if (result >= 128 && result <= 159) {
              result = map_1252_to_unicode[result - 128];
            } /* if */
            deadKeyActive = 0;
            break;
          case WM_USER:
            traceEvent(printf("gkbGetc: WM_USER hwnd=" FMT_U_MEM
                              ", wParam=" FMT_U_MEM ", lParam=" FMT_X_MEM "\n",
                              (memSizeType) msg.hwnd, msg.wParam, msg.lParam););
            result = K_RESIZE;
            clickedWindow = msg.hwnd;
            break;
          default:
            traceEvent(printf("gkbGetc: message=%d, hwnd=" FMT_U_MEM
                              ", wParam=" FMT_U_MEM ", lParam=" FMT_X_MEM "\n",
                              msg.message, (memSizeType) msg.hwnd, msg.wParam,
                              msg.lParam););
            /* E.g.: WM_NCMOUSELEAVE */
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            break;
        } /* switch */
      } /* if */
      if (result == K_NONE) {
        /* printf("before GetMessageW\n"); */
        bRet = GetMessageW(&msg, NULL, 0, 0);
        /* printf("after GetMessageW\n"); */
      } /* if */
    } /* while */
    logFunction(printf("gkbGetc --> %d\n", result););
    return result;
  } /* gkbGetc */



boolType gkbInputReady (void)

  {
    BOOL msg_present;
    BOOL bRet;
    MSG msg;
    MSG mouseMoveMsg;
    boolType result;

  /* gkbInputReady */
    logFunction(printf("gkbInputReady\n"););
    result = FALSE;
    msg_present = PeekMessageW(&msg, NULL, 0, 0, PM_NOREMOVE);
    while (msg_present) {
      /* printf("gkbInputReady: message=%d %lu, %d, %x\n", msg.message, msg.hwnd, msg.wParam, msg.lParam); */
      switch (msg.message) {
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
          traceEvent(printf("gkbInputReady: WM_KEYDOWN/WM_SYSKEYDOWN hwnd=" FMT_U_MEM
                            ", wParam=" FMT_U_MEM ", lParam=" FMT_X_MEM "\n",
                            (memSizeType) msg.hwnd, msg.wParam, msg.lParam););
          if (msg.wParam == VK_SHIFT   || msg.wParam == VK_CONTROL ||
              msg.wParam == VK_MENU    || msg.wParam == VK_LWIN    ||
              msg.wParam == VK_RWIN    || msg.wParam == VK_CAPITAL ||
              msg.wParam == VK_NUMLOCK || msg.wParam == VK_SCROLL) {
            bRet = GetMessageW(&msg, NULL, 0, 0);
            if (bRet == 0 || bRet == -1) {
              logError(printf("GetMessageW(&msg, NULL, 0, 0)=%d\n", (int) bRet););
            } else {
              TranslateMessage(&msg);
              DispatchMessage(&msg);
            } /* if */
            msg_present = PeekMessageW(&msg, NULL, 0, 0, PM_NOREMOVE);
          } else {
            if (isDeadKey(msg.wParam)) {
              if (processDeadKeys) {
                if (deadKeyActive == 0) {
                  deadKeyActive = msg.wParam;
                  bRet = GetMessageW(&msg, NULL, 0, 0);
                  if (bRet == 0 || bRet == -1) {
                    logError(printf("GetMessageW(&msg, NULL, 0, 0)=%d\n", (int) bRet););
                  } else {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                  } /* if */
                  msg_present = PeekMessageW(&msg, NULL, 0, 0, PM_NOREMOVE);
                } else if (deadKeyActive == msg.wParam) {
                  msg_present = 0;
                  result = TRUE;
                } else {
                  deadKeyActive = msg.wParam;
                  bRet = GetMessageW(&msg, NULL, 0, 0);
                  if (bRet == 0 || bRet == -1) {
                    logError(printf("GetMessageW(&msg, NULL, 0, 0)=%d\n", (int) bRet););
                  } /* if */
                  PostMessageW(msg.hwnd, WM_KEYDOWN, VK_SPACE, msg.lParam);
                  msg_present = 0;
                  result = TRUE;
                } /* if */
              } else {
                bRet = GetMessageW(&msg, NULL, 0, 0);
                if (bRet == 0 || bRet == -1) {
                  logError(printf("GetMessageW(&msg, NULL, 0, 0)=%d\n", (int) bRet););
                } else {
                  TranslateMessage(&msg);
                  DispatchMessage(&msg);
                } /* if */
                PostMessageW(msg.hwnd, WM_KEYDOWN, VK_SPACE, msg.lParam);
                msg_present = 0;
                result = TRUE;
              } /* if */
            } else {
              msg_present = 0;
              result = TRUE;
            } /* if */
          } /* if */
          break;
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_XBUTTONDOWN:
          traceEvent(printf("gkbInputReady: WM_L/M/R/XBUTTONDOWN hwnd=" FMT_U_MEM
                            ", wParam=" FMT_X64 ", lParam=" FMT_X_MEM "\n",
                            (memSizeType) msg.hwnd, msg.wParam, msg.lParam););
          msg_present = 0;
          result = TRUE;
          break;
        case WM_MOUSEWHEEL:
          traceEvent(printf("gkbInputReady: WM_MOUSEWHEEL hwnd=" FMT_U_MEM
                            ", wParam=" FMT_U_MEM ", lParam=" FMT_X_MEM "\n",
                            (memSizeType) msg.hwnd, msg.wParam, msg.lParam););
          if (IsWindow(msg.hwnd)) {
            POINT point;
            winType win;

            point.x = GET_X_LPARAM(msg.lParam);
            point.y = GET_Y_LPARAM(msg.lParam);
            ScreenToClient(msg.hwnd, &point);
            win = find_window(msg.hwnd);
            if (point.x >= 0 && point.x < drwWidth(win) &&
                point.y >= 0 && point.y < drwHeight(win)) {
              msg_present = 0;
              result = TRUE;
            } else {
              bRet = GetMessageW(&msg, NULL, 0, 0);
              if (bRet == 0 || bRet == -1) {
                logError(printf("GetMessageW(&msg, NULL, 0, 0)=%d\n", (int) bRet););
                msg_present = PeekMessageW(&msg, NULL, 0, 0, PM_NOREMOVE);
              } else if (msg.message == WM_MOUSEWHEEL) {
                msg_present = PeekMessageW(&msg, NULL, 0, 0, PM_NOREMOVE);
              } /* if */
            } /* if */
          } else {
            bRet = GetMessageW(&msg, NULL, 0, 0);
            if (bRet == 0 || bRet == -1) {
              logError(printf("GetMessageW(&msg, NULL, 0, 0)=%d\n", (int) bRet););
              msg_present = PeekMessageW(&msg, NULL, 0, 0, PM_NOREMOVE);
            } else if (msg.message == WM_MOUSEWHEEL) {
              msg_present = PeekMessageW(&msg, NULL, 0, 0, PM_NOREMOVE);
            } /* if */
          } /* if */
          break;
        case WM_NCLBUTTONDOWN:
          traceEvent(printf("gkbInputReady: WM_NCLBUTTONDOWN hwnd=" FMT_U_MEM
                            ", wParam=" FMT_U_MEM ", lParam=" FMT_X_MEM "\n",
                            (memSizeType) msg.hwnd, msg.wParam, msg.lParam););
          if (msg.wParam == HTCLOSE && IsWindow(msg.hwnd)) {
            /* printf("HTCLOSE\n"); */
            if (getCloseAction(find_window(msg.hwnd)) == CLOSE_BUTTON_CLOSES_PROGRAM) {
              os_exit(0);
            } else {
              msg_present = 0;
              result = TRUE;
            } /* if */
          } else {
            bRet = GetMessageW(&msg, NULL, 0, 0);
            if (bRet == 0 || bRet == -1) {
              logError(printf("GetMessageW(&msg, NULL, 0, 0)=%d\n", (int) bRet););
            } else if (msg.message != WM_NCLBUTTONDOWN) {
              logError(printf("Expected WM_NCLBUTTONDOWN but GetMessageW() returned %d\n",
                              msg.message););
              TranslateMessage(&msg);
              DispatchMessage(&msg);
            } else {
              if (msg.wParam == HTBOTTOMRIGHT || msg.wParam == HTRIGHT || msg.wParam == HTBOTTOM) {
                resizeBottomAndRight(&msg);
              } else if (msg.wParam == HTTOPLEFT || msg.wParam == HTLEFT || msg.wParam == HTTOP ||
                  msg.wParam == HTTOPRIGHT || msg.wParam == HTBOTTOMLEFT) {
                resizeTopAndLeft(&msg);
              } else if (msg.wParam == HTCAPTION) {
                startMoveWindow(&msg);
              } else {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
              } /* if */
            } /* if */
            msg_present = PeekMessageW(&msg, NULL, 0, 0, PM_NOREMOVE);
          } /* if */
          break;
        case WM_SYSCOMMAND:
          traceEvent(printf("gkbInputReady: WM_SYSCOMMAND hwnd=" FMT_U_MEM
                             ", wParam=" FMT_U_MEM ", lParam=" FMT_X_MEM "\n",
                            (memSizeType) msg.hwnd, msg.wParam, msg.lParam););
          if ((msg.wParam & 0xfff0) == SC_CLOSE && IsWindow(msg.hwnd)) {
            /* printf("SC_CLOSE\n"); */
            msg_present = 0;
            result = TRUE;
          } else {
            bRet = GetMessageW(&msg, NULL, 0, 0);
            if (bRet == 0 || bRet == -1) {
              logError(printf("GetMessageW(&msg, NULL, 0, 0)=%d\n", (int) bRet););
            } else if (msg.message != WM_SYSCOMMAND) {
              logError(printf("Expected WM_SYSCOMMAND but GetMessageW() returned %d\n",
                              msg.message););
              TranslateMessage(&msg);
              DispatchMessage(&msg);
            } else {
              if ((msg.wParam & 0xfff0) == SC_SIZE) {
                /* printf("SC_SIZE\n"); */
                systemSizeCommand(&msg);
              } else if ((msg.wParam & 0xfff0) == SC_MOVE) {
                /* printf("SC_MOVE\n"); */
                systemMoveCommand(&msg);
              } else {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
              } /* if */
            } /* if */
            msg_present = PeekMessageW(&msg, NULL, 0, 0, PM_NOREMOVE);
          } /* if */
          break;
        case WM_MOUSEMOVE:
          traceEvent(printf("gkbInputReady: WM_MOUSEMOVE hwnd=" FMT_U_MEM
                             ", wParam=" FMT_U_MEM ", lParam=" FMT_X_MEM "\n",
                            (memSizeType) msg.hwnd, msg.wParam, msg.lParam););
          bRet = GetMessageW(&mouseMoveMsg, NULL, 0, 0);
          if (bRet == 0 || bRet == -1) {
            logError(printf("GetMessageW(&msg, NULL, 0, 0)=%d\n", (int) bRet););
            msg_present = PeekMessageW(&msg, NULL, 0, 0, PM_NOREMOVE);
          } else if (mouseMoveMsg.message == WM_MOUSEMOVE) {
            if (resizeMode != 0) {
              processMouseMove(&mouseMoveMsg);
            } else {
              TranslateMessage(&mouseMoveMsg);
              DispatchMessage(&mouseMoveMsg);
            } /* if */
            msg_present = PeekMessageW(&msg, NULL, 0, 0, PM_NOREMOVE);
          } else {
            /* GetMessageW() did not read a WM_MOUSEMOVE message. */
            /* Process the WM_MOUSEMOVE message returned by       */
            /* PeekMessageW() instead. The loop is not left and   */
            /* the message read by GetMessageW() is processed in  */
            /* the next loop pass.                                */
            if (resizeMode != 0) {
              processMouseMove(&msg);
            } else {
              TranslateMessage(&msg);
              DispatchMessage(&msg);
            } /* if */
            traceEvent(printf("gkbInputReady: WM_MOUSEMOVE GetMessageW: message=%d, hwnd="
                              FMT_U_MEM ", wParam=" FMT_U_MEM ", lParam=" FMT_X_MEM "\n",
                              mouseMoveMsg.message, (memSizeType) mouseMoveMsg.hwnd,
                              mouseMoveMsg.wParam, mouseMoveMsg.lParam););
            memcpy(&msg, &mouseMoveMsg, sizeof(MSG));
          } /* if */
          break;
        case WM_LBUTTONUP:
          traceEvent(printf("gkbInputReady: WM_LBUTTONUP hwnd=" FMT_U_MEM
                             ", wParam=" FMT_U_MEM ", lParam=" FMT_X_MEM "\n",
                            (memSizeType) msg.hwnd, msg.wParam, msg.lParam););
          bRet = GetMessageW(&msg, NULL, 0, 0);
          if (bRet == 0 || bRet == -1) {
            logError(printf("GetMessageW(&msg, NULL, 0, 0)=%d\n", (int) bRet););
          } else {
            if (resizeMode != 0) {
              /* printf("resizeMode: %d\n", (int) resizeMode); */
              ReleaseCapture();
              resizeMode = 0;
            } else {
              TranslateMessage(&msg);
              DispatchMessage(&msg);
            } /* if */
          } /* if */
          msg_present = PeekMessageW(&msg, NULL, 0, 0, PM_NOREMOVE);
          break;
        case WM_SYSKEYUP:
          traceEvent(printf("gkbInputReady: WM_SYSKEYUP hwnd=" FMT_U_MEM
                            ", wParam=" FMT_U_MEM ", lParam=" FMT_X_MEM "\n",
                            (memSizeType) msg.hwnd, msg.wParam, msg.lParam););
          bRet = GetMessageW(&msg, NULL, 0, 0);
          if (bRet == 0 || bRet == -1) {
            logError(printf("GetMessageW(&msg, NULL, 0, 0)=%d\n", (int) bRet););
          } /* if */
          msg_present = PeekMessageW(&msg, NULL, 0, 0, PM_NOREMOVE);
          break;
        case WM_CHAR:
          traceEvent(printf("gkbInputReady: WM_CHAR hwnd=" FMT_U_MEM
                            ", wParam=" FMT_X64 ", lParam=" FMT_X_MEM "\n",
                            (memSizeType) msg.hwnd, msg.wParam, msg.lParam););
          msg_present = 0;
          result = TRUE;
          break;
        case WM_USER:
          traceEvent(printf("gkbInputReady: WM_USER hwnd=" FMT_U_MEM
                            ", wParam=" FMT_U_MEM ", lParam=" FMT_X_MEM "\n",
                            (memSizeType) msg.hwnd, msg.wParam, msg.lParam););
          msg_present = 0;
          result = TRUE;
          break;
        default:
          traceEvent(printf("gkbInputReady: message=%d, hwnd=" FMT_U_MEM
                            ", wParam=" FMT_U_MEM ", lParam=" FMT_X_MEM "\n",
                            msg.message, (memSizeType) msg.hwnd, msg.wParam,
                            msg.lParam););
          bRet = GetMessageW(&msg, NULL, 0, 0);
          if (bRet == 0 || bRet == -1) {
            logError(printf("GetMessageW(&msg, NULL, 0, 0)=%d\n", (int) bRet););
          } else {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
          } /* if */
          msg_present = PeekMessageW(&msg, NULL, 0, 0, PM_NOREMOVE);
          break;
      } /* switch */
    } /* while */
    logFunction(printf("gkbInputReady --> %d\n", result););
    return result;
  } /* gkbInputReady */



boolType gkbButtonPressed (charType button)

  {
    int vkey1 = 0;
    int vkey2 = 0;
    int not_vkey = 0;
    int state_vkey = 0;
    boolType okay = TRUE;
    boolType result;

  /* gkbButtonPressed */
    gkbInputReady();
    switch (button) {
      case K_CTL_A: case K_ALT_A: case 'A': case 'a': vkey1 = 'A'; break;
      case K_CTL_B: case K_ALT_B: case 'B': case 'b': vkey1 = 'B'; break;
      case K_CTL_C: case K_ALT_C: case 'C': case 'c': vkey1 = 'C'; break;
      case K_CTL_D: case K_ALT_D: case 'D': case 'd': vkey1 = 'D'; break;
      case K_CTL_E: case K_ALT_E: case 'E': case 'e': vkey1 = 'E'; break;
      case K_CTL_F: case K_ALT_F: case 'F': case 'f': vkey1 = 'F'; break;
      case K_CTL_G: case K_ALT_G: case 'G': case 'g': vkey1 = 'G'; break;
      /* K_CTL_H */ case K_ALT_H: case 'H': case 'h': vkey1 = 'H'; break;
      /* K_CTL_I */ case K_ALT_I: case 'I': case 'i': vkey1 = 'I'; break;
      /* K_CTL_J */ case K_ALT_J: case 'J': case 'j': vkey1 = 'J'; break;
      case K_CTL_K: case K_ALT_K: case 'K': case 'k': vkey1 = 'K'; break;
      case K_CTL_L: case K_ALT_L: case 'L': case 'l': vkey1 = 'L'; break;
      case K_CTL_M: case K_ALT_M: case 'M': case 'm': vkey1 = 'M'; break;
      case K_CTL_N: case K_ALT_N: case 'N': case 'n': vkey1 = 'N'; break;
      case K_CTL_O: case K_ALT_O: case 'O': case 'o': vkey1 = 'O'; break;
      case K_CTL_P: case K_ALT_P: case 'P': case 'p': vkey1 = 'P'; break;
      case K_CTL_Q: case K_ALT_Q: case 'Q': case 'q': vkey1 = 'Q'; break;
      case K_CTL_R: case K_ALT_R: case 'R': case 'r': vkey1 = 'R'; break;
      case K_CTL_S: case K_ALT_S: case 'S': case 's': vkey1 = 'S'; break;
      case K_CTL_T: case K_ALT_T: case 'T': case 't': vkey1 = 'T'; break;
      case K_CTL_U: case K_ALT_U: case 'U': case 'u': vkey1 = 'U'; break;
      case K_CTL_V: case K_ALT_V: case 'V': case 'v': vkey1 = 'V'; break;
      case K_CTL_W: case K_ALT_W: case 'W': case 'w': vkey1 = 'W'; break;
      case K_CTL_X: case K_ALT_X: case 'X': case 'x': vkey1 = 'X'; break;
      case K_CTL_Y: case K_ALT_Y: case 'Y': case 'y': vkey1 = 'Y'; break;
      case K_CTL_Z: case K_ALT_Z: case 'Z': case 'z': vkey1 = 'Z'; break;

      case K_CTL_0: case K_ALT_0: vkey1 = '0'; break;
      case K_CTL_1: case K_ALT_1: vkey1 = '1'; break;
      case K_CTL_2: case K_ALT_2: vkey1 = '2'; break;
      case K_CTL_3: case K_ALT_3: vkey1 = '3'; break;
      case K_CTL_4: case K_ALT_4: vkey1 = '4'; break;
      case K_CTL_5: case K_ALT_5: vkey1 = '5'; break;
      case K_CTL_6: case K_ALT_6: vkey1 = '6'; break;
      case K_CTL_7: case K_ALT_7: vkey1 = '7'; break;
      case K_CTL_8: case K_ALT_8: vkey1 = '8'; break;
      case K_CTL_9: case K_ALT_9: vkey1 = '9'; break;

      case '0': vkey1 = '0';
                if ((GetKeyState(VK_NUMLOCK) & 0x0001) != 0 &&
                    (GetKeyState(VK_CONTROL) & 0x8000) == 0 &&
                    (GetKeyState(VK_MENU) & 0x8000) == 0) {
                  vkey2 = VK_NUMPAD0;
                } break;
      case '1': vkey1 = '1';
                if ((GetKeyState(VK_NUMLOCK) & 0x0001) != 0 &&
                    (GetKeyState(VK_CONTROL) & 0x8000) == 0 &&
                    (GetKeyState(VK_MENU) & 0x8000) == 0) {
                  vkey2 = VK_NUMPAD1;
                } break;
      case '2': vkey1 = '2';
                if ((GetKeyState(VK_NUMLOCK) & 0x0001) != 0 &&
                    (GetKeyState(VK_CONTROL) & 0x8000) == 0 &&
                    (GetKeyState(VK_MENU) & 0x8000) == 0) {
                  vkey2 = VK_NUMPAD2;
                } break;
      case '3': vkey1 = '3';
                if ((GetKeyState(VK_NUMLOCK) & 0x0001) != 0 &&
                    (GetKeyState(VK_CONTROL) & 0x8000) == 0 &&
                    (GetKeyState(VK_MENU) & 0x8000) == 0) {
                  vkey2 = VK_NUMPAD3;
                } break;
      case '4': vkey1 = '4';
                if ((GetKeyState(VK_NUMLOCK) & 0x0001) != 0 &&
                    (GetKeyState(VK_CONTROL) & 0x8000) == 0 &&
                    (GetKeyState(VK_MENU) & 0x8000) == 0) {
                  vkey2 = VK_NUMPAD4;
                } break;
      case '5': vkey1 = '5';
                if ((GetKeyState(VK_NUMLOCK) & 0x0001) != 0 &&
                    (GetKeyState(VK_CONTROL) & 0x8000) == 0 &&
                    (GetKeyState(VK_MENU) & 0x8000) == 0) {
                  vkey2 = VK_NUMPAD5;
                } break;
      case '6': vkey1 = '6';
                if ((GetKeyState(VK_NUMLOCK) & 0x0001) != 0 &&
                    (GetKeyState(VK_CONTROL) & 0x8000) == 0 &&
                    (GetKeyState(VK_MENU) & 0x8000) == 0) {
                  vkey2 = VK_NUMPAD6;
                } break;
      case '7': vkey1 = '7';
                if ((GetKeyState(VK_NUMLOCK) & 0x0001) != 0 &&
                    (GetKeyState(VK_CONTROL) & 0x8000) == 0 &&
                    (GetKeyState(VK_MENU) & 0x8000) == 0) {
                  vkey2 = VK_NUMPAD7;
                } break;
      case '8': vkey1 = '8';
                if ((GetKeyState(VK_NUMLOCK) & 0x0001) != 0 &&
                    (GetKeyState(VK_CONTROL) & 0x8000) == 0 &&
                    (GetKeyState(VK_MENU) & 0x8000) == 0) {
                  vkey2 = VK_NUMPAD8;
                } break;
      case '9': vkey1 = '9';
                if ((GetKeyState(VK_NUMLOCK) & 0x0001) != 0 &&
                    (GetKeyState(VK_CONTROL) & 0x8000) == 0 &&
                    (GetKeyState(VK_MENU) & 0x8000) == 0) {
                  vkey2 = VK_NUMPAD9;
                } break;

      case K_F1:  case K_SFT_F1:  case K_CTL_F1:  case K_ALT_F1:  vkey1 = VK_F1;  break;
      case K_F2:  case K_SFT_F2:  case K_CTL_F2:  case K_ALT_F2:  vkey1 = VK_F2;  break;
      case K_F3:  case K_SFT_F3:  case K_CTL_F3:  case K_ALT_F3:  vkey1 = VK_F3;  break;
      case K_F4:  case K_SFT_F4:  case K_CTL_F4:  case K_ALT_F4:  vkey1 = VK_F4;  break;
      case K_F5:  case K_SFT_F5:  case K_CTL_F5:  case K_ALT_F5:  vkey1 = VK_F5;  break;
      case K_F6:  case K_SFT_F6:  case K_CTL_F6:  case K_ALT_F6:  vkey1 = VK_F6;  break;
      case K_F7:  case K_SFT_F7:  case K_CTL_F7:  case K_ALT_F7:  vkey1 = VK_F7;  break;
      case K_F8:  case K_SFT_F8:  case K_CTL_F8:  case K_ALT_F8:  vkey1 = VK_F8;  break;
      case K_F9:  case K_SFT_F9:  case K_CTL_F9:  case K_ALT_F9:  vkey1 = VK_F9;  break;
      case K_F10: case K_SFT_F10: case K_CTL_F10: case K_ALT_F10: vkey1 = VK_F10; break;
      case K_F11: case K_SFT_F11: case K_CTL_F11: case K_ALT_F11: vkey1 = VK_F11; break;
      case K_F12: case K_SFT_F12: case K_CTL_F12: case K_ALT_F12: vkey1 = VK_F12; break;

      case K_LEFT:  case K_SFT_LEFT:  case K_CTL_LEFT:  case K_ALT_LEFT:
        vkey1 = VK_LEFT;
        if ((GetKeyState(VK_CONTROL) & 0x8000) != 0 ||
            (GetKeyState(VK_MENU) & 0x8000) != 0) {
          vkey2 = VK_NUMPAD4;
        } /* if */
        break;
      case K_RIGHT: case K_SFT_RIGHT: case K_CTL_RIGHT: case K_ALT_RIGHT:
        vkey1 = VK_RIGHT;
        if ((GetKeyState(VK_CONTROL) & 0x8000) != 0 ||
            (GetKeyState(VK_MENU) & 0x8000) != 0) {
          vkey2 = VK_NUMPAD6;
        } /* if */
        break;
      case K_UP:    case K_SFT_UP:    case K_CTL_UP:    case K_ALT_UP:
        vkey1 = VK_UP;
        if ((GetKeyState(VK_CONTROL) & 0x8000) != 0 ||
            (GetKeyState(VK_MENU) & 0x8000) != 0) {
          vkey2 = VK_NUMPAD8;
        } /* if */
        break;
      case K_DOWN:  case K_SFT_DOWN:  case K_CTL_DOWN:  case K_ALT_DOWN:
        vkey1 = VK_DOWN;
        if ((GetKeyState(VK_CONTROL) & 0x8000) != 0 ||
            (GetKeyState(VK_MENU) & 0x8000) != 0) {
          vkey2 = VK_NUMPAD2;
        } /* if */
        break;
      case K_HOME:  case K_SFT_HOME:  case K_CTL_HOME:  case K_ALT_HOME:
        vkey1 = VK_HOME;
        if ((GetKeyState(VK_CONTROL) & 0x8000) != 0 ||
            (GetKeyState(VK_MENU) & 0x8000) != 0) {
          vkey2 = VK_NUMPAD7;
        } /* if */
        break;
      case K_END:   case K_SFT_END:   case K_CTL_END:   case K_ALT_END:
        vkey1 = VK_END;
        if ((GetKeyState(VK_CONTROL) & 0x8000) != 0 ||
            (GetKeyState(VK_MENU) & 0x8000) != 0) {
          vkey2 = VK_NUMPAD1;
        } /* if */
        break;
      case K_PGUP:  case K_SFT_PGUP:  case K_CTL_PGUP:  case K_ALT_PGUP:
        vkey1 = VK_PRIOR;
        if ((GetKeyState(VK_CONTROL) & 0x8000) != 0 ||
            (GetKeyState(VK_MENU) & 0x8000) != 0) {
          vkey2 = VK_NUMPAD9;
        } /* if */
        break;
      case K_PGDN:  case K_SFT_PGDN:  case K_CTL_PGDN:  case K_ALT_PGDN:
        vkey1 = VK_NEXT;
        if ((GetKeyState(VK_CONTROL) & 0x8000) != 0 ||
            (GetKeyState(VK_MENU) & 0x8000) != 0) {
          vkey2 = VK_NUMPAD3;
        } /* if */
        break;
      case K_INS:   case K_SFT_INS:   case K_CTL_INS:   case K_ALT_INS:
        vkey1 = VK_INSERT;
        if ((GetKeyState(VK_CONTROL) & 0x8000) != 0 ||
            (GetKeyState(VK_MENU) & 0x8000) != 0) {
          vkey2 = VK_NUMPAD0;
        } /* if */
        break;
      case K_DEL:   case K_SFT_DEL:   case K_CTL_DEL:   case K_ALT_DEL:
        vkey1 = VK_DELETE;
        if ((GetKeyState(VK_CONTROL) & 0x8000) != 0 ||
            (GetKeyState(VK_MENU) & 0x8000) != 0) {
          vkey2 = VK_DECIMAL;
        } /* if */
        break;
      case K_PAD_CENTER:     case K_SFT_PAD_CENTER:
      case K_CTL_PAD_CENTER: case K_ALT_PAD_CENTER:
        vkey1 = VK_CLEAR;
        if ((GetKeyState(VK_CONTROL) & 0x8000) != 0 ||
            (GetKeyState(VK_MENU) & 0x8000) != 0) {
          vkey2 = VK_NUMPAD5;
        } /* if */
        break;

      case K_NL:
        vkey1 = VK_RETURN;
        if ((GetKeyState(VK_CONTROL) & 0x8000) != 0) {
          vkey2 = 'J';
        } break;
      case K_SFT_NL:  case K_CTL_NL:  case K_ALT_NL:
        vkey1 = VK_RETURN; break;
      case K_BS:
        vkey1 = VK_BACK;
        if ((GetKeyState(VK_CONTROL) & 0x8000) != 0) {
          vkey2 = 'H';
        } break;
      case K_SFT_BS:  case K_CTL_BS:  case K_ALT_BS:
        vkey1 = VK_BACK;   break;
      case K_TAB:
        vkey1 = VK_TAB;
        if ((GetKeyState(VK_CONTROL) & 0x8000) != 0) {
          vkey2 = 'I';
        } break;
      case K_BACKTAB: case K_CTL_TAB: case K_ALT_TAB:
        vkey1 = VK_TAB;    break;
      case K_ESC: case K_SFT_ESC: case K_CTL_ESC: case K_ALT_ESC:
        vkey1 = VK_ESCAPE; break;

      case K_MENU:   case K_SFT_MENU:   case K_CTL_MENU:   case K_ALT_MENU:
        vkey1 = VK_APPS;   break;
      case K_PRINT:  case K_SFT_PRINT:  case K_CTL_PRINT:  case K_ALT_PRINT:
        vkey1 = VK_PRINT;  break;
      case K_PAUSE:  case K_SFT_PAUSE:  case K_CTL_PAUSE:  case K_ALT_PAUSE:
        vkey1 = VK_PAUSE;  break;

      case ' ': vkey1 = VK_SPACE;    break;
      case '*': vkey1 = VK_MULTIPLY; vkey2 = VkKeyScanW('*') & 0xff; break;
      case '+': vkey1 = VK_ADD;      vkey2 = VK_OEM_PLUS;            break;
      case '-': vkey1 = VK_SUBTRACT; vkey2 = VK_OEM_MINUS;           break;
      case '/': vkey1 = VK_DIVIDE;   vkey2 = VkKeyScanW('/') & 0xff; break;

      case K_MOUSE1: case K_SFT_MOUSE1: case K_CTL_MOUSE1: case K_ALT_MOUSE1:
        if (resizeMode == 0) {
          vkey1 = VK_LBUTTON;
        } /* if */
        break;
      case K_MOUSE2: case K_SFT_MOUSE2: case K_CTL_MOUSE2: case K_ALT_MOUSE2: vkey1 = VK_MBUTTON; break;
      case K_MOUSE3: case K_SFT_MOUSE3: case K_CTL_MOUSE3: case K_ALT_MOUSE3: vkey1 = VK_RBUTTON; break;

      case K_MOUSE_FWD:      case K_SFT_MOUSE_FWD:
      case K_CTL_MOUSE_FWD:  case K_ALT_MOUSE_FWD:  vkey1 = VK_XBUTTON2; break;
      case K_MOUSE_BACK:     case K_SFT_MOUSE_BACK:
      case K_CTL_MOUSE_BACK: case K_ALT_MOUSE_BACK: vkey1 = VK_XBUTTON1; break;

      case K_SHIFT:          vkey1 = VK_SHIFT;    break;
      case K_LEFT_SHIFT:     vkey1 = VK_LSHIFT;   break;
      case K_RIGHT_SHIFT:    vkey1 = VK_RSHIFT;   break;
      case K_CONTROL:        vkey1 = VK_CONTROL;  not_vkey = VK_RMENU; vkey2 = VK_RCONTROL; break;
      case K_LEFT_CONTROL:   vkey1 = VK_LCONTROL; not_vkey = VK_RMENU; break;
      case K_RIGHT_CONTROL:  vkey1 = VK_RCONTROL; break;
      case K_ALT:            vkey1 = VK_MENU;     break;
      case K_LEFT_ALT:       vkey1 = VK_LMENU;    break;
      case K_RIGHT_ALT:      vkey1 = VK_RMENU;    break;
      case K_SUPER:          vkey1 = VK_LWIN;     vkey2 = VK_RWIN;     break;
      case K_LEFT_SUPER:     vkey1 = VK_LWIN;     break;
      case K_RIGHT_SUPER:    vkey1 = VK_RWIN;     break;
      case K_SHIFT_LOCK:     vkey1      = VK_CAPITAL; break;
      case K_SHIFT_LOCK_ON:  state_vkey = VK_CAPITAL; break;
      case K_NUM_LOCK:       vkey1      = VK_NUMLOCK; break;
      case K_NUM_LOCK_ON:    state_vkey = VK_NUMLOCK; break;
      case K_SCROLL_LOCK:    vkey1      = VK_SCROLL;  break;
      case K_SCROLL_LOCK_ON: state_vkey = VK_SCROLL;  break;

      default:
        if (button <= 0xffff) {
          vkey1 = VkKeyScanW((WCHAR) button) & 0xff;
        } else {
          result = FALSE;
          okay = FALSE;
        } /* if */
        break;
    } /* switch */

    if (okay) {
      if (vkey1 != 0) {
        result = (GetKeyState(vkey1) & 0x8000) != 0;
        if (result && not_vkey != 0) {
          result = (GetKeyState(not_vkey) & 0x8000) == 0;
        } /* if */
        if (!result && vkey2 != 0) {
          result = (GetKeyState(vkey2) & 0x8000) != 0;
        } /* if */
      } else {
        result = (GetKeyState(state_vkey) & 0x0001) != 0;
      } /* if */
    } /* if */
    return result;
  } /* gkbButtonPressed */



charType gkbRawGetc (void)

  { /* gkbRawGetc */
    return gkbGetc();
  } /* gkbRawGetc */



void gkbSelectInput (winType aWindow, charType aKey, boolType active)

  { /* gkbSelectInput */
    if (aKey == K_RESIZE) {
      setResizeReturnsKey(aWindow, active);
    } else if (aKey == K_CLOSE) {
      if (active) {
        drwSetCloseAction(aWindow, CLOSE_BUTTON_RETURNS_KEY);
      } else {
        drwSetCloseAction(aWindow, CLOSE_BUTTON_CLOSES_PROGRAM);
      } /* if */
    } else {
      raise_error(RANGE_ERROR);
    } /* if */
  } /* gkbSelectInput */



intType gkbClickedXpos (void)

  { /* gkbClickedXpos */
    logFunction(printf("gkbClickedXpos -> " FMT_D "\n", clicked_x););
    return clicked_x;
  } /* gkbClickedXpos */



intType gkbClickedYpos (void)

  { /* gkbClickedYpos */
    logFunction(printf("gkbClickedYpos -> " FMT_D "\n", clicked_y););
    return clicked_y;
  } /* gkbClickedYpos */



winType gkbWindow (void)

  {
    winType result;

  /* gkbWindow */
    logFunction(printf("gkbWindow\n"););
    result = find_window(clickedWindow);
    if (result != NULL && result->usage_count != 0) {
      result->usage_count++;
    } /* if */
    logFunction(printf("gkbWindow -> " FMT_U_MEM " (usage=" FMT_U ")\n",
                       (memSizeType) result,
                       result != NULL ? result->usage_count : (uintType) 0););
    return result;
  } /* gkbWindow */



void drwFlush (void)

  { /* drwFlush */
    logFunction(printf("drwFlush\n"););
    gkbInputReady();
    logFunction(printf("drwFlush -->\n"););
  } /* drwFlush */
