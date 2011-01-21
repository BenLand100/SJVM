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

#include "Exception.h"
#include "Object.h"
#include "ClassFile.h"
#include <iostream>

//#define DEBUG_OUTPUT

#ifdef DEBUG_OUTPUT
#define debug(v) std::cout << v;
#else
#define debug(v)
#endif

Exception::Exception(ClassFile* clazz, MethodInfo* method, Exception* parent) {
    this->clazz = clazz;
    this->method = method;
    this->parent = parent;
    this->cause = parent->cause;
    debug("Pushed Exception " << (int)cause->type << ' ' << cause->type->name<< '\n');
}

Exception::Exception(ClassFile* clazz, MethodInfo* method, JOBJECT cause) {
    this->clazz = clazz;
    this->method = method;
    this->parent = 0;
    this->cause = cause;
    debug("New Exception " << (int)cause->type << ' ' << cause->type->name<< '\n');
}

Exception::~Exception() {
    if (parent != 0) delete parent;
}

JOBJECT Exception::getCause() {
    debug("Cause Type: " << (int) cause->type << '\n');
    return cause;
}
