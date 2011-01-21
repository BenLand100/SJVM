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

