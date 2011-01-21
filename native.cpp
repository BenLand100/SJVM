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
 * File:   native.cpp
 * Author: Benjamin J. Land
 *
 * Created on December 17, 2008, 3:32 PM
 */

#include "Native.h"

#include <dlfcn.h>
#include <iostream>

#define DEBUG_OUTPUT

#ifdef DEBUG_OUTPUT
#define debug(v) std::cout << v;
#else
#define debug(v)
#endif

LibraryPool::LibraryPool() {
    libraries[""] = dlopen(0, RTLD_GLOBAL | RTLD_NOW);
}

LibraryPool::~LibraryPool() {
    for (std::map<std::string,void*>::iterator iter = libraries.begin(); iter != libraries.end(); iter++) {
        dlclose(iter->second);
    }
    libraries.clear();
}

int LibraryPool::loadlib(const char *library) {
    void* ptr = (void*) dlopen(library, RTLD_GLOBAL | RTLD_NOW);
    if (ptr) libraries[library] = ptr;
    return (int) ptr;
}

void* LibraryPool::address(const char *name) {
    debug("Requesting Symbol: " << name << '\n');
    for (std::map<std::string,void*>::iterator iter = libraries.begin(); iter != libraries.end(); iter++) {
        void* lib = iter->second;
        void *ptr = dlsym(lib, name);
        if (ptr) return ptr;
    }
    return 0;
}
