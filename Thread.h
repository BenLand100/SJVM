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
