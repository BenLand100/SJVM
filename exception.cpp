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
