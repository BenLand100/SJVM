/* 
 * File:   Exception.h
 * Author: Benjamin J. Land
 *
 * Created on December 1, 2008, 5:07 PM
 */

#ifndef _EXCEPTION_H
#define	_EXCEPTION_H

#include "Types.h"

class MethodInfo;
class ClassFile;

class Exception {
public:
    JOBJECT cause;
    Exception* parent; //deleted on delete
    MethodInfo* method;
    ClassFile* clazz;
    
    Exception(ClassFile* clazz, MethodInfo* method, Exception* parent);
    Exception(ClassFile* clazz, MethodInfo* method, JOBJECT cause);
    ~Exception();
            
    JOBJECT getCause();
};

#endif	/* _EXCEPTION_H */

