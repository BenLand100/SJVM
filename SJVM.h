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

