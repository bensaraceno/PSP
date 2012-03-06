/*
 * PSPLINK
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in PSPLINK root for details.
 *
 * sio.h - PSPLINK kernel module sio code
 *
 * Copyright (c) 2005 James F <tyranid@gmail.com>
 *
 * $HeadURL: svn://svn.pspdev.org/psp/trunk/psplink/psplink/sio.h $
 * $Id: sio.h 1972 2006-07-17 19:44:32Z tyranid $
 */

#ifndef __SIO_H__
#define __SIO_H__

void _sioInit(void);
void sioInit(int baudrate, int kponly);
int sioReadChar(void);
int sioReadCharWithTimeout(void);
int sioPutText(const char *data, int len);
PspDebugPutChar sioDisableKprintf(void);
void sioEnableKprintf(PspDebugPutChar kp);

#endif
