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


#include <set>

#include <set>

#include "Object.h"
#include "Arrays.h"
#include "ClassFile.h"

#include <iostream>
#include <set>

//#define DEBUG_OUTPUT

#ifdef DEBUG_OUTPUT
#define debug(v) std::cout << v;
#else
#define debug(v)
#endif

Object::Object() {
    debug("Object Constructor: " << (int)this << '\n');
    type = 0;
    fields = 0;
    staticCount = 0;
    refCount = 0;
    localCount = 0;
}

Object::~Object() {
    debug("Object Destructor: " << (int)this << '\n');
    if (fields) delete [] fields;
}

