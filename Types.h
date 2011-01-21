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

#ifndef _TYPES_H
#define	_TYPES_H

class Object;
class ClassFile;

template <class T> class Array;

typedef unsigned char	JBOOLEAN;
typedef unsigned short	JCHAR;
typedef short		JSHORT;
typedef float		JFLOAT;
typedef double		JDOUBLE;
typedef long            JINT;
typedef long long       JLONG;
typedef signed char     JBYTE;

typedef Object*         JOBJECT;
typedef ClassFile*      JCLASS;

typedef Array<JBOOLEAN>*JBOOLEANARRAY;
typedef Array<JCHAR>*   JCHARARRAY;
typedef Array<JBYTE>*   JBYTEARRAY;
typedef Array<JSHORT>*  JSHORTARRAY;
typedef Array<JINT>*    JINTARRAY;
typedef Array<JLONG>*   JLONGARRAY;
typedef Array<JFLOAT>*  JFLOATARRAY;
typedef Array<JDOUBLE>* JDOUBLEARRAY;
typedef Array<JOBJECT>* JOBJECTARRAY;

typedef union {
    JBOOLEAN z;
    JBYTE    b;
    JCHAR    c;
    JSHORT   s;
    JFLOAT   f;
    JDOUBLE  d;
    JINT     i;
    JLONG    j;
    JOBJECT  l;
} Variable;

#endif	/* _TYPES_H */

