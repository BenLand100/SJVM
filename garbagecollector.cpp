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

#include "ClassFile.h"

#include <set>
#include <iostream>
#include <cstring>
#include <vector>

#include "ClassFile.h"
#include "GarbageCollector.h"
#include "Object.h"
#include "Thread.h"
#include "Arrays.h"

#define WARNING_OUTPUT
//#define DEBUG_OUTPUT

#ifdef DEBUG_OUTPUT
#define debug(v) std::cout << v;
#else
#define debug(v)
#endif

#ifdef WARNING_OUTPUT
#define warn(v) std::cout << "WARNING: " << v;
#else
#define warn(v)
#endif


LocalGC::LocalGC(GlobalGC *gc, Thread *thread) {
    parent = gc;
    gc->mutex.lock();
    oldLocal = thread->nativeLocal;
    thread->nativeLocal = this;
    this->thread = thread;
    gc->locals.insert(this);
    gc->mutex.unlock();
    debug("Local GC Init\n");
}

LocalGC::~LocalGC() {
    parent->mutex.lock();
    thread->nativeLocal = oldLocal;
    parent->locals.erase(this);
    for (std::set<Object*>::iterator iter = roots.begin(); iter != roots.end(); iter++) {
        Object *obj = *iter;
        debug("GC: " << obj->refCount << ' ' << obj->staticCount << ' ' << obj->localCount << ' ' << (int)obj << '\n');
        obj->localCount--;
        if (obj->refCount || obj->staticCount || obj->localCount) 
            continue;
        parent->clean(obj);
    }
    roots.clear();
    parent->mutex.unlock();
    debug("Local GC Out of Focus\n");
}

void LocalGC::addLocal(Object* obj) {
    if (!obj) return;
    debug("Local Added: " << (int)obj << '\n');
    debug("GC: " << obj->refCount << ' ' << obj->staticCount << ' ' << obj->localCount << ' ' << (int)obj << '\n');
    parent->mutex.lock();
    if (roots.find(obj) == roots.end()) {
        roots.insert(obj);
        obj->localCount++;
    }
    parent->mutex.unlock();
}

void LocalGC::removeLocal(Object* obj) {
    if (!obj) return;
    debug("Local Removed: " << (int)obj << '\n');
    debug("GC: " << obj->refCount << ' ' << obj->staticCount << ' ' << obj->localCount << ' ' << (int)obj << '\n');
    parent->mutex.lock();
    if (roots.find(obj) != roots.end()) {
        debug("haha\n");
        roots.erase(obj);
        obj->localCount--;
    }
    parent->mutex.unlock();
}

GlobalGC::GlobalGC() {
    
}

GlobalGC::~GlobalGC() {
    sweep();
    mutex.lock();
    for (std::set<LocalGC*>::iterator iter = locals.begin(); iter != locals.end(); iter = locals.begin()) {
        delete (*iter); //dangerous to force, but thread safe
    }
    for (std::set<Object*>::iterator iter = objects.begin(); iter != objects.end(); iter++) {
        delete *iter;
    }
    mutex.unlock();
}

void GlobalGC::sweep() {
}

void GlobalGC::clean(Object* object) {
    if (!object) return;
    debug("Cleaning " << (int) object << ' ');
    if (object->type) {
        if (object->type->arrayType && object->type->componentType) {
            JOBJECTARRAY array = (JOBJECTARRAY)object;
            for (int i = 0; i < array->size; i++) {
                decrRef(array->array[i]);
            }
        }
        debug(object->type->name << '\n');
        std::vector<unsigned int> indexes = object->type->objectIndexes;
        Variable *fields = new Variable[object->type->instanceFields];
        memcpy(fields,object->fields,object->type->instanceFields*sizeof(Variable));
        for (int i = 0; i < indexes.size(); i++) {
            debug("decrref1\n");
            decrRef(fields[indexes[i]].l);
            debug("decrref2\n");
        }
    } else {
        debug("Untyped\n"); 
    }
    debug("cleaned\n");
    if (objects.find(object) != objects.end()) {
        objects.erase(object);
        debug("deleting\n");
        delete object;
        debug("deleted\n");
    }
}

void GlobalGC::incrPersist(Object* obj) {
    if (!obj) return;
    debug("Incr Persist: " << (int)obj << '\n');
    debug("GC: " << obj->refCount << ' ' << obj->staticCount << ' ' << obj->localCount << ' ' << (int)obj << '\n');
    mutex.lock();
    obj->staticCount++;
    mutex.unlock();
}

void GlobalGC::decrPersist(Object* obj) {
    if (!obj) return;
    mutex.lock();
    debug("Decr Persist: " << (int)obj << '\n');
    debug("GC: " << obj->refCount << ' ' << obj->staticCount << ' ' << obj->localCount << ' ' << (int)obj << '\n');
    mutex.lock();
    obj->staticCount--;
    if (!(obj->refCount || obj->staticCount || obj->localCount)) {
        clean(obj);
    }
    mutex.unlock();
}

void GlobalGC::incrRef(Object* obj) {
    if (!obj) return;
    debug("Incr Ref: " << (int)obj << '\n');
    debug("GC: " << obj->refCount << ' ' << obj->staticCount << ' ' << obj->localCount << ' ' << (int)obj << '\n');
    mutex.lock();
    obj->refCount++;
    mutex.unlock();
}

void GlobalGC::decrRef(Object* obj) {
    if (!obj) return;
    debug("Decr Ref: " << (int)obj << '\n');
    debug("GC: " << obj->refCount << ' ' << obj->staticCount << ' ' << obj->localCount << ' ' << (int)obj << '\n');
    mutex.lock();
    obj->refCount--;
    if (!(obj->refCount || obj->staticCount || obj->localCount)) {
        clean(obj);
    }
    mutex.unlock();
}

void GlobalGC::reg(Object *obj) {
    objects.insert(obj);
}
