/* 
 * File:   Object.h
 * Author: Benjamin J. Land
 *
 * Created on November 26, 2008, 5:13 PM
 */

#ifndef _OBJECT_H
#define	_OBJECT_H

#include "Types.h"
#include "Mutex.h"

class ClassFile;
class Object;
class Mutex;

class Object {
    friend class ClassFile;
    friend class GlobalGC;
    friend class LocalGC;
    
private:
    unsigned int staticCount;
    unsigned int refCount;
    unsigned int localCount;
    
protected:
    Object();
    
public:
    virtual ~Object();
    
    ClassFile *type;
    Variable *fields;
    Mutex mutex;
    
};

#endif	/* _OBJECT_H */

