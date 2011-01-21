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
