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

#ifndef _THREAD_H
#define	_THREAD_H

#include "jni.h"

#include "Object.h"
#include "GarbageCollector.h"

class SJVM;
class Exception;
class Object;

extern struct JNINativeInterface_ SJVM_JNINativeInterface;

//FIXME needs type set
class Thread : public Object {
    friend class SJVM;
private:
    SJVM *sjvm;
    JNIEnv* jnienv;
    unsigned int id;
    
    Thread(SJVM *sjvm);
    virtual ~Thread();
public:
    JNIEnv* getEnv();
    void start();
    Exception *nativeException;
    LocalGC *nativeLocal;
    
    static Thread* getThread(SJVM *sjvm);
};

#endif	/* _THREAD_H */
