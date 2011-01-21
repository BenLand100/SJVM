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

