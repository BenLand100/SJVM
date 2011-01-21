/**
 *  Copyright 2010 by Benjamin J. Land (a.k.a. BenLand100)
 *
 *  This file is part of SJVM the Simple Java Virtual Machine.
 *
 *  SJVM is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  SJVM is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with SJVM. If not, see <http://www.gnu.org/licenses/>.
 */

/* 
 * File:   Inflate.h
 * Author: Benjamin J. Land
 *
 * Created on December 4, 2008, 7:28 PM
 */

#ifndef _INFLATE_H
#define	_INFLATE_H

#ifdef __cplusplus
#include <cstring>
extern "C" {
#endif

struct _huft;

typedef struct {
	unsigned char*	slide;
	unsigned 	hufts;		/* track memory usage */
	struct _huft*	fixed_tl;
	struct _huft*	fixed_td;
	int		fixed_bl;
	int		fixed_bd;
	unsigned	wp;		/* current position in slide */
	unsigned int 	bb;		/* bit buffer */
	unsigned	bk;		/* bits in bit buffer */
	unsigned char*	inbuf;
	int		insz;
	unsigned char*	outbuf;
	int		outsz;
} inflateInfo;

inflateInfo* inflate_new(void);
int inflate_free(inflateInfo*);
int inflate_oneshot(unsigned char*, int, unsigned char*, int);

#ifdef __cplusplus
};
#endif

#endif	/* _INFLATE_H */

