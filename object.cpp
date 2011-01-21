
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

