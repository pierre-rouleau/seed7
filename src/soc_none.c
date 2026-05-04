/********************************************************************/
/*                                                                  */
/*  soc_none.c    Dummy functions for the socket type.              */
/*  Copyright (C) 1989 - 2018  Thomas Mertes                        */
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
/*  File: seed7/src/soc_none.c                                      */
/*  Changes: 2011, 2018  Thomas Mertes                              */
/*  Content: Dummy functions for the socket type.                   */
/*                                                                  */
/********************************************************************/

#include "version.h"

#if SOCKET_LIB == NO_SOCKETS
#include "stdio.h"

#include "common.h"
#include "data_rtl.h"
#include "rtl_err.h"

#undef EXTERN
#define EXTERN
#include "soc_rtl.h"



socketType socAccept (const const_socketType listenerSocket,
    bstriType *const address)

  { /* socAccept */
    raise_error(FILE_ERROR);
    return 0;
  } /* socAccept */



intType socAddrFamily (const const_bstriType address)

  { /* socAddrFamily */
    raise_error(FILE_ERROR);
    return 0;
  } /* socAddrFamily */



striType socAddrNumeric (const const_bstriType address)

  { /* socAddrNumeric */
    raise_error(FILE_ERROR);
    return NULL;
  } /* socAddrNumeric */



striType socAddrService (const const_bstriType address)

  { /* socAddrService */
    raise_error(FILE_ERROR);
    return NULL;
  } /* socAddrService */



void socBind (const const_socketType listenerSocket,
    const const_bstriType address)

  { /* socBind */
    raise_error(FILE_ERROR);
  } /* socBind */



void socClose (const socketType aSocket)

  { /* socClose */
    raise_error(FILE_ERROR);
  } /* socClose */



void socConnect (const const_socketType aSocket,
    const const_bstriType address)

  { /* socConnect */
    raise_error(FILE_ERROR);
  } /* socConnect */



void socDestr (const socketType oldSocket)

  { /* socDestr */
    raise_error(FILE_ERROR);
  } /* socDestr */



void socFree (const socketType oldSocket)

  { /* socFree */
  } /* socFree */



charType socGetc (const const_socketType inSocket,
    charType *const eofIndicator)

  { /* socGetc */
    raise_error(FILE_ERROR);
    return (charType) EOF;
  } /* socGetc */



striType socGets (const const_socketType inSocket, intType length,
    charType *const eofIndicator)

  { /* socGets */
    raise_error(FILE_ERROR);
    return NULL;
  } /* socGets */



striType socGetHostname (void)

  { /* socGetHostname */
    raise_error(FILE_ERROR);
    return NULL;
  } /* socGetHostname */



bstriType socGetLocalAddr (const const_socketType aSocket)

  { /* socGetLocalAddr */
    raise_error(FILE_ERROR);
    return NULL;
  } /* socGetLocalAddr */



bstriType socGetPeerAddr (const const_socketType aSocket)

  { /* socGetPeerAddr */
    raise_error(FILE_ERROR);
    return NULL;
  } /* socGetPeerAddr */



boolType socHasNext (const const_socketType inSocket)

  { /* socHasNext */
    raise_error(FILE_ERROR);
    return TRUE;
  } /* socHasNext */



bstriType socInetAddr (const const_striType hostName, intType port)

  { /* socInetAddr */
    raise_error(FILE_ERROR);
    return NULL;
  } /* socInetAddr */



bstriType socInetLocalAddr (intType port)

  { /* socInetLocalAddr */
    raise_error(FILE_ERROR);
    return NULL;
  } /* socInetLocalAddr */



bstriType socInetServAddr (intType port)

  { /* socInetServAddr */
    raise_error(FILE_ERROR);
    return NULL;
  } /* socInetServAddr */



boolType socInputReady (const const_socketType inSocket,
    intType seconds, intType micro_seconds)

  { /* socInputReady */
    raise_error(FILE_ERROR);
    return FALSE;
  } /* socInputReady */



striType socLineRead (const const_socketType inSocket,
    charType *const terminationChar)

  { /* socLineRead */
    raise_error(FILE_ERROR);
    return NULL;
  } /* socLineRead */



void socListen (const const_socketType listenerSocket, intType backlog)

  { /* socListen */
    raise_error(FILE_ERROR);
  } /* socListen */



intType socRecv (const const_socketType inSocket, striType *const stri,
    intType length, intType flags)

  { /* socRecv */
    raise_error(FILE_ERROR);
    return 0;
  } /* socRecv */



intType socRecvfrom (const const_socketType inSocket, striType *const stri,
    intType length, intType flags, bstriType *const address)

  { /* socRecvfrom */
    raise_error(FILE_ERROR);
    return 0;
  } /* socRecvfrom */



intType socSend (const const_socketType outSocket,
    const const_striType stri, intType flags)

  { /* socSend */
    raise_error(FILE_ERROR);
    return 0;
  } /* socSend */



intType socSendto (const const_socketType outSocket,
    const const_striType stri, intType flags, const_bstriType address)

  { /* socSendto */
    raise_error(FILE_ERROR);
    return 0;
  } /* socSendto */



void socSetOptBool (const const_socketType aSocket, intType optname,
    boolType optval)

  { /* socSetOptBool */
    raise_error(FILE_ERROR);
  } /* socSetOptBool */



socketType socSocket (intType domain, intType type, intType protocol)

  { /* socSocket */
    raise_error(FILE_ERROR);
    return 0;
  } /* socSocket */



striType socWordRead (const const_socketType inSocket,
    charType *const terminationChar)

  { /* socWordRead */
    raise_error(FILE_ERROR);
    return NULL;
  } /* socWordRead */



void socWrite (const const_socketType outSocket,
    const const_striType stri)

  { /* socWrite */
    raise_error(FILE_ERROR);
  } /* socWrite */

#endif
