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

