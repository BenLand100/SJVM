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

#include "Thread.h"
#include "SJVM.h"

#include <iostream>
#include <pthread.h>
#include <cstring>

#define DEBUG_OUTPUT

#ifdef DEBUG_OUTPUT
#define debug(v) std::cout << v;
#else
#define debug(v)
#endif

Thread::Thread(SJVM *_sjvm) : sjvm(_sjvm), nativeException(0) {
    debug("Thread Entered\n");
    jnienv = new JNIEnv;
    JNINativeInterface_ *functions = new JNINativeInterface_;
    memcpy(functions, &SJVM_JNINativeInterface, sizeof(JNINativeInterface_));
    functions->reserved0 = sjvm;
    functions->reserved1 = this;
    jnienv->functions = functions;
    id = pthread_self();
    sjvm->threads[id] = this;
    nativeLocal = 0;
    new LocalGC(sjvm->gc,this); //Perisistes until gc is destroyed... not so smart
    sjvm->gc->objects.insert(this);
    nativeLocal->addLocal(this);
}

Thread::~Thread() {
    debug("Thread Exited\n");
    sjvm->threads.erase(id);
    delete jnienv->functions;
    delete jnienv;
}

Thread* Thread::getThread(SJVM *sjvm) {
    unsigned int id = pthread_self();
    if (sjvm->threads.find(id) == sjvm->threads.end()) {
        return new Thread(sjvm);
    }
    return sjvm->threads[id];
}

JNIEnv* Thread::getEnv() {
    return jnienv;
}

void Thread::start() {
    
}
