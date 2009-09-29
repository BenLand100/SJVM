/* 
 * File:   SJVM.h
 * Author: Benjamin J. Land
 *
 * Created on November 26, 2008, 5:02 PM
 */

#ifndef _SJVM_H
#define	_SJVM_H

#include "Types.h"
#include "jni.h"
#include "Native.h"
#include "GarbageCollector.h"

#include <map>

class GlobalGC;
class Exception;
class ClassLoader;
class ClassPath;
class MethodInfo;
class ClassFile;
class Thread;
class LibraryPool;


extern struct JNIInvokeInterface_ SJVM_JNIInvokeInterface;

class SJVM {
    friend class Thread;
private:
    JavaVM* vm;
    LibraryPool libpool;
    ClassLoader *loader;
    std::map<unsigned int,Thread*> threads;
    std::map<std::string,JOBJECT> strings;
    std::map<std::string,void*> natives;
public:
    SJVM(ClassPath *path);
    virtual ~SJVM();
    GlobalGC *gc;
    void mapNative(char *fullname, void *address);
    void* getNative(char *fullname);
    Variable execute(MethodInfo *meth, ClassFile *cur, Thread *thread, long *args, long argsc, Exception **exception);

    JavaVM* getJavaVM();
    ClassFile* loadClass(const char* name);

};

#endif	/* _SJVM_H */

