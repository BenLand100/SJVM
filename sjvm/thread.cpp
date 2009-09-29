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
