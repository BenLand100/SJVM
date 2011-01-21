/* 
 * File:   GarbageColector.h
 * Author: Benjamin J. Land
 *
 * Created on December 18, 2008, 11:40 AM
 */

#ifndef _GARBAGECOLLECTOR_H
#define	_GARBAGECOLLECTOR_H

#include <set>
#include <map>
#include "Mutex.h"

class Object;
class Thread;
class GlobalGC;

class LocalGC {
    friend class GlobalGC;
private:
    std::set<Object*> roots;
    GlobalGC *parent;
    LocalGC *oldLocal;
    Thread *thread;
public:
    LocalGC(GlobalGC *gc, Thread *thread);
    ~LocalGC();
    void addLocal(Object* object);
    void removeLocal(Object* object); //does not clean, prevents death
};

class GlobalGC {
    friend class LocalGC;
private:
    Mutex mutex;
    std::set<LocalGC*> locals;
    void clean(Object* object);
public:
    std::set<Object*> objects;
    GlobalGC();
    ~GlobalGC();
    void sweep();
    void incrPersist(Object* object);
    void decrPersist(Object* object);
    void incrRef(Object* object);
    void decrRef(Object* object);
    void reg(Object* object);
};

#endif	/* _GARBAGECOLLECTOR_H */

