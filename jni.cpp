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

#include "GarbageCollector.h"
#include "FieldInfo.h"
#include "SJVM.h"
#include "Thread.h"
#include "Exception.h"
#include "GarbageCollector.h"
#include "ClassFile.h"
#include "MethodInfo.h"
#include "Types.h"
#include "CPInfo.h"
#include "Arrays.h"
#include "Object.h"
#include "Thread.h"
#include "Jar.h"

#include "jni.h"
#include "SJVM.h"
#include "ClassLoader.h"

#include <iostream>
#include <string>
#include <cstring>
#include <stdlib.h>

//#define DEBUG_OUTPUT

#ifdef DEBUG_OUTPUT
#define debug(v) std::cout << v;
#else
#define debug(v)
#endif


static int java_major_version = 1;
static int java_minor_version = 1;
extern struct JNINativeInterface_ SJVM_JNINativeInterface;
extern JavaVM SJVM_JavaVM;
extern struct JNIEnv_ SJVM_JNIEnv;
static JNICALL jint SJVM_GetVersion(JNIEnv*);
static JNICALL jclass SJVM_FindClass(JNIEnv*, const char*);
static JNICALL jint SJVM_ThrowNew(JNIEnv*, jclass, const char*);
static JNICALL jint SJVM_Throw(JNIEnv* env, jobject obj);

inline Variable callV(SJVM* sjvm, Thread* thread, MethodInfo* meth, jobject obj, va_list args, Exception** thrown) {
    debug("JNI: CallV\n");
    long argsarr[meth->nargs + 1];
    argsarr[0] = (long) obj;
    for (int i = 0, c = 1; i < meth->argsc; i++) {
        if (meth->typeSz[i] == 2) {
            long long a = va_arg(args, long long);
            *(long long*) &argsarr[c] = a;
            c += 2;
        } else {
            long a = va_arg(args, long);
            *(long*) &argsarr[c] = a;
            c++;
        }
    }
    Variable res = sjvm->execute(meth, meth->clazz, thread, argsarr, meth->nargs + 1, thrown);
    if (meth->resobject) thread->nativeLocal->addLocal(res.l);
    return res;
}

inline Variable callVirtualV(SJVM* sjvm, Thread* thread, MethodInfo* meth, jobject obj, va_list args, Exception** thrown) {
    debug("JNI: CallV\n");
    long argsarr[meth->nargs + 1];
    argsarr[0] = (long) obj;
    for (int i = 0, c = 1; i < meth->argsc; i++) {
        if (meth->typeSz[i] == 2) {
            long long a = va_arg(args, long long);
            *(long long*) &argsarr[c] = a;
            c += 2;
        } else {
            long a = va_arg(args, long);
            *(long*) &argsarr[c] = a;
            c++;
        }
    }
    for (ClassFile *type = ((JOBJECT) obj)->type; type != 0; type = type->superClass) {
        if (type->methodMap.find(meth->full) != type->methodMap.end()) {
            MethodInfo *invoke = type->methodMap[meth->full];
            Variable res = sjvm->execute(invoke, invoke->clazz, thread, argsarr, meth->nargs + 1, thrown);
            if (invoke->resobject) thread->nativeLocal->addLocal(res.l);
            return res;
        }
    }
}

inline Variable callStaticV(SJVM* sjvm, Thread* thread, MethodInfo* meth, va_list args, Exception** thrown) {
    debug("JNI: CallStaticV\n");
    long argsarr[meth->nargs];
    for (int i = 0, c = 0; i < meth->argsc; i++) {
        if (meth->typeSz[i] == 2) {
            long long a = va_arg(args, long long);
            *(long long*) &argsarr[c] = a;
            c += 2;
        } else {
            long a = va_arg(args, long);
            *(long*) &argsarr[c] = a;
            c++;
        }
    }
    Variable res = sjvm->execute(meth, meth->clazz, thread, argsarr, meth->nargs, thrown);
    if (meth->resobject) thread->nativeLocal->addLocal(res.l);
    return res;
}

inline Variable callA(SJVM* sjvm, Thread* thread, MethodInfo* meth, jobject obj, const jvalue* args, Exception** thrown) {
    debug("JNI: CallA\n");
    long argsarr[meth->nargs + 1];
    argsarr[0] = (long) obj;
    for (int i = 0, c = 1; i < meth->argsc; i++) {
        if (meth->typeSz[i] == 2) {
            *(long long*) &argsarr[c] = args[i].j;
            c += 2;
        } else {
            *(long*) &argsarr[c] = args[i].i;
            c++;
        }
    }
    Variable res = sjvm->execute(meth, meth->clazz, thread, argsarr, meth->nargs + 1, thrown);
    if (meth->resobject) thread->nativeLocal->addLocal(res.l);
    return res;
}

inline Variable callVirtualA(SJVM* sjvm, Thread* thread, MethodInfo* meth, jobject obj, const jvalue* args, Exception** thrown) {
    debug("JNI: CallA\n");
    long argsarr[meth->nargs + 1];
    argsarr[0] = (long) obj;
    for (int i = 0, c = 1; i < meth->argsc; i++) {
        if (meth->typeSz[i] == 2) {
            *(long long*) &argsarr[c] = args[i].j;
            c += 2;
        } else {
            *(long*) &argsarr[c] = args[i].i;
            c++;
        }
    }
    for (ClassFile *type = ((JOBJECT) obj)->type; type != 0; type = type->superClass) {
        if (type->methodMap.find(meth->full) != type->methodMap.end()) {
            MethodInfo *invoke = type->methodMap[meth->full];
            Variable res = sjvm->execute(invoke, invoke->clazz, thread, argsarr, meth->nargs + 1, thrown);
            if (invoke->resobject) thread->nativeLocal->addLocal(res.l);
            return res;
        }
    }
}

inline Variable callStaticA(SJVM* sjvm, Thread* thread, MethodInfo* meth, const jvalue* args, Exception** thrown) {
    debug("JNI: CallStaticA\n");
    long argsarr[meth->nargs];
    for (int i = 0, c = 0; i < meth->argsc; i++) {
        if (meth->typeSz[i] == 2) {
            *(long long*) &argsarr[c] = args[i].j;
            c += 2;
        } else {
            *(long*) &argsarr[c] = args[i].i;
            c++;
        }
    }
    Variable res = sjvm->execute(meth, meth->clazz, thread, argsarr, meth->nargs, thrown);
    if (meth->resobject) thread->nativeLocal->addLocal(res.l);
    return res;
}

extern "C" jint JNICALL JNI_GetDefaultJavaVMInitArgs(void* args_) {
    JavaVMInitArgs *args = (JavaVMInitArgs*) args_;
    if (args->version != ((java_major_version << 16) | java_minor_version)) {
        return JNI_EVERSION;
    }
    args->version = (java_major_version << 16) | java_minor_version;
    return JNI_OK;
}

extern "C" jint JNICALL JNI_CreateJavaVM(JavaVM** vm, void** env_, void* args_) {
    JavaVMInitArgs *args = (JavaVMInitArgs*) args_;
    JNIEnv **env = (JNIEnv**) env_;
    if (args->version != ((java_major_version << 16) | java_minor_version)) {
        return JNI_EVERSION;
    }
    int count = args->nOptions;
    BootstrapClassPath *path = new BootstrapClassPath;
    path->add(new JarFile((char*)"/usr/share/classpath/glibj.zip"));
    path->add(new JarFile((char*)"/usr/share/classpath/tools.zip"));
    for (int i = 0; i < count; i++) {
        path->add(new FolderClassPath(args->options[i].optionString));
    }
    SJVM *sjvm = new SJVM(path);
    *vm = sjvm->getJavaVM();
    Thread *thread = Thread::getThread(sjvm);
    *env = thread->getEnv();
    return 0;
}

extern "C" jint JNICALL JNI_GetCreatedJavaVMs(JavaVM** vm, jsize buflen, jsize* nvm) {
    return JNI_ERR;
}

extern JNICALL void SJVM_FatalError(JNIEnv* env, const char* mess) {
    fprintf(stderr, "FATAL ERROR: %s\n", mess);
    exit(1);
}

extern JNICALL void SJVM_DeleteGlobalRef(JNIEnv* env, jref obj) {
    ((SJVM*) env->functions->reserved0)->gc->decrPersist((JOBJECT) obj);
}

extern JNICALL void SJVM_DeleteLocalRef(JNIEnv* env, jref obj) {
    //do nothing, force not the local deletion
}

extern JNICALL jboolean SJVM_IsSameObject(JNIEnv* env, jobject obj1, jobject obj2) {
    return obj1 == obj2 ? JNI_TRUE : JNI_FALSE;
}

extern JNICALL void SJVM_ReleaseStringChars(JNIEnv* env, jstring data, const jchar* chars) {
    //do nothing, always the chars returned they are
}

extern JNICALL jint SJVM_GetVersion(JNIEnv* env) {
    return ((java_major_version << 16) | java_minor_version);
}

extern JNICALL jref SJVM_NewGlobalRef(JNIEnv* env, jref obj) { //Create global ref
    ((SJVM*) env->functions->reserved0)->gc->incrPersist((JOBJECT) obj);
    return obj;
}

extern JNICALL jclass SJVM_DefineClass(JNIEnv* env, const char *name, jobject loader, const jbyte *buf, jsize len) { //Define the class
    //FIXME load it
    return 0;
}

extern JNICALL jclass SJVM_FindClass(JNIEnv* env, const char* name) { //Use Class.ForName
    return (jclass) ((SJVM*) env->functions->reserved0)->loadClass(name);
}

extern JNICALL jclass SJVM_GetSuperClass(JNIEnv* env, jclass cls) {
    return (jclass) ((JCLASS) cls)->superClass;
}

extern JNICALL jboolean SJVM_IsAssignableFrom(JNIEnv* env, jclass cls1, jclass cls2) {
    return 0; //FIXME what?
}

extern JNICALL jint SJVM_Throw(JNIEnv* env, jthrowable obj) { //Post Exception
    ((Thread*) env->functions->reserved1)->nativeException = new Exception(0, 0, (JOBJECT) obj);
}

extern JNICALL jint SJVM_ThrowNew(JNIEnv* env, jclass cls, const char* mess) { //Create exception & post it
    return 0; //FIXME what?
}

extern JNICALL jthrowable SJVM_ExceptionOccurred(JNIEnv* env) {
    return (jthrowable) ((Thread*) env->functions->reserved1)->nativeException;
}

extern JNICALL void SJVM_ExceptionDescribe(JNIEnv* env) {
    //FIXME print exception
}

extern JNICALL void SJVM_ExceptionClear(JNIEnv* env) {
    if (((Thread*) env->functions->reserved1)->nativeException)
        delete ((Thread*) env->functions->reserved1)->nativeException;
    ((Thread*) env->functions->reserved1)->nativeException = 0;
}

extern JNICALL jobject SJVM_AllocObject(JNIEnv* env, jclass cls) {
    JOBJECT obj = ((JCLASS) cls)->newInstance((SJVM*) env->functions->reserved0);
    ((Thread*) env->functions->reserved1)->nativeLocal->addLocal(obj);
    debug("JNI: Alocated Object\n");
    return (jobject) obj;
}

extern JNICALL jobject SJVM_NewObjectV(JNIEnv* env, jclass cls, jmethodID meth, va_list args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    jobject obj = env->AllocObject(cls);
    callV(jvm, thread, method, obj, args, &thread->nativeException);
    return obj;
}

extern JNICALL jobject SJVM_NewObject(JNIEnv* env, jclass cls, jmethodID meth, ...) {
    jobject obj;
    va_list args;
    va_start(args, meth);
    obj = SJVM_NewObjectV(env, cls, meth, args);
    va_end(args);
    return obj;
}

extern JNICALL jobject SJVM_NewObjectA(JNIEnv* env, jclass cls, jmethodID meth, const jvalue* args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    jobject obj = env->AllocObject(cls);
    callA(jvm, thread, method, obj, args, &thread->nativeException);
    return obj;
}

extern JNICALL jclass SJVM_GetObjectClass(JNIEnv* env, jobject obj) {
    return (jclass) ((JOBJECT) obj)->type;
}

extern JNICALL jboolean SJVM_IsInstanceOf(JNIEnv* env, jobject obj, jclass cls) {
    return ((JCLASS) cls)->instanceOf((JOBJECT) obj);
}

extern JNICALL jmethodID SJVM_GetMethodID(JNIEnv* env, jclass cls, const char* name, const char* sig) {
    std::string full(name);
    full.append(sig);
    debug("JNI: GetMethodID - " << full << '\n');
    return (jmethodID) ((JCLASS) cls)->methodMap[full];
}

extern JNICALL jobject SJVM_CallObjectMethodV(JNIEnv* env, jobject obj, jmethodID meth, va_list args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    return (jobject) callVirtualV(jvm, thread, method, obj, args, &thread->nativeException).l;
}

extern JNICALL jobject SJVM_CallObjectMethod(JNIEnv* env, jobject obj, jmethodID meth, ...) {
    va_list args;
    jobject ret;
    va_start(args, meth);
    ret = SJVM_CallObjectMethodV(env, obj, meth, args);
    va_end(args);
    return ret;
}

extern JNICALL jobject SJVM_CallObjectMethodA(JNIEnv* env, jobject obj, jmethodID meth, const jvalue* args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    return (jobject) callVirtualA(jvm, thread, method, obj, args, &thread->nativeException).l;
}

extern JNICALL jboolean SJVM_CallBooleanMethodV(JNIEnv* env, jobject obj, jmethodID meth, va_list args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    return (jboolean) callVirtualV(jvm, thread, method, obj, args, &thread->nativeException).z;
}

extern JNICALL jboolean SJVM_CallBooleanMethod(JNIEnv* env, jobject obj, jmethodID meth, ...) {
    va_list args;
    jboolean ret;
    va_start(args, meth);
    ret = SJVM_CallBooleanMethodV(env, obj, meth, args);
    va_end(args);
    return ret;
}

extern JNICALL jboolean SJVM_CallBooleanMethodA(JNIEnv* env, jobject obj, jmethodID meth, const jvalue* args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    return (jboolean) callVirtualA(jvm, thread, method, obj, args, &thread->nativeException).z;
}

extern JNICALL jbyte SJVM_CallByteMethodV(JNIEnv* env, jobject obj, jmethodID meth, va_list args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    return (jbyte) callVirtualV(jvm, thread, method, obj, args, &thread->nativeException).b;
}

extern JNICALL jbyte SJVM_CallByteMethod(JNIEnv* env, jobject obj, jmethodID meth, ...) {
    va_list args;
    jbyte ret;
    va_start(args, meth);
    ret = SJVM_CallByteMethodV(env, obj, meth, args);
    va_end(args);
    return ret;
}

extern JNICALL jbyte SJVM_CallByteMethodA(JNIEnv* env, jobject obj, jmethodID meth, const jvalue* args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    return (jbyte) callVirtualA(jvm, thread, method, obj, args, &thread->nativeException).b;
}

extern JNICALL jchar SJVM_CallCharMethodV(JNIEnv* env, jobject obj, jmethodID meth, va_list args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    return (jchar) callVirtualV(jvm, thread, method, obj, args, &thread->nativeException).c;
}

extern JNICALL jchar SJVM_CallCharMethod(JNIEnv* env, jobject obj, jmethodID meth, ...) {
    va_list args;
    jchar ret;
    va_start(args, meth);
    ret = SJVM_CallCharMethodV(env, obj, meth, args);
    va_end(args);
    return ret;
}

extern JNICALL jchar SJVM_CallCharMethodA(JNIEnv* env, jobject obj, jmethodID meth, const jvalue* args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    return (jchar) callVirtualA(jvm, thread, method, obj, args, &thread->nativeException).c;
}

extern JNICALL jshort SJVM_CallShortMethodV(JNIEnv* env, jobject obj, jmethodID meth, va_list args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    return (jshort) callVirtualV(jvm, thread, method, obj, args, &thread->nativeException).s;
}

extern JNICALL jshort SJVM_CallShortMethod(JNIEnv* env, jobject obj, jmethodID meth, ...) {
    va_list args;
    jshort ret;
    va_start(args, meth);
    ret = SJVM_CallShortMethodV(env, obj, meth, args);
    va_end(args);
    return ret;
}

extern JNICALL jshort SJVM_CallShortMethodA(JNIEnv* env, jobject obj, jmethodID meth, const jvalue* args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    return (jshort) callVirtualA(jvm, thread, method, obj, args, &thread->nativeException).s;
}

extern JNICALL jint SJVM_CallIntMethodV(JNIEnv* env, jobject obj, jmethodID meth, va_list args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    return (jint) callVirtualV(jvm, thread, method, obj, args, &thread->nativeException).i;
}

extern JNICALL jint SJVM_CallIntMethod(JNIEnv* env, jobject obj, jmethodID meth, ...) {
    va_list args;
    jint ret;
    va_start(args, meth);
    ret = SJVM_CallIntMethodV(env, obj, meth, args);
    va_end(args);
    return ret;
}

extern JNICALL jint SJVM_CallIntMethodA(JNIEnv* env, jobject obj, jmethodID meth, const jvalue* args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    return (jint) callVirtualA(jvm, thread, method, obj, args, &thread->nativeException).i;
}

extern JNICALL jlong SJVM_CallLongMethodV(JNIEnv* env, jobject obj, jmethodID meth, va_list args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    return (jlong) callVirtualV(jvm, thread, method, obj, args, &thread->nativeException).j;
}

extern JNICALL jlong SJVM_CallLongMethod(JNIEnv* env, jobject obj, jmethodID meth, ...) {
    va_list args;
    jlong ret;
    va_start(args, meth);
    ret = SJVM_CallLongMethodV(env, obj, meth, args);
    va_end(args);
    return ret;
}

extern JNICALL jlong SJVM_CallLongMethodA(JNIEnv* env, jobject obj, jmethodID meth, const jvalue* args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    return (jlong) callVirtualA(jvm, thread, method, obj, args, &thread->nativeException).j;
}

extern JNICALL jfloat SJVM_CallFloatMethodV(JNIEnv* env, jobject obj, jmethodID meth, va_list args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    return (jfloat) callVirtualV(jvm, thread, method, obj, args, &thread->nativeException).f;
}

extern JNICALL jfloat SJVM_CallFloatMethod(JNIEnv* env, jobject obj, jmethodID meth, ...) {
    va_list args;
    jfloat ret;
    va_start(args, meth);
    ret = SJVM_CallFloatMethodV(env, obj, meth, args);
    va_end(args);
    return ret;
}

extern JNICALL jfloat SJVM_CallFloatMethodA(JNIEnv* env, jobject obj, jmethodID meth, const jvalue* args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    return (jfloat) callVirtualA(jvm, thread, method, obj, args, &thread->nativeException).f;
}

extern JNICALL jdouble SJVM_CallDoubleMethodV(JNIEnv* env, jobject obj, jmethodID meth, va_list args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    return (jdouble) callVirtualV(jvm, thread, method, obj, args, &thread->nativeException).d;
}

extern JNICALL jdouble SJVM_CallDoubleMethod(JNIEnv* env, jobject obj, jmethodID meth, ...) {
    va_list args;
    jdouble ret;
    va_start(args, meth);
    ret = SJVM_CallDoubleMethodV(env, obj, meth, args);
    va_end(args);
    return ret;
}

extern JNICALL jdouble SJVM_CallDoubleMethodA(JNIEnv* env, jobject obj, jmethodID meth, const jvalue* args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    return (jdouble) callVirtualA(jvm, thread, method, obj, args, &thread->nativeException).d;
}

extern JNICALL void SJVM_CallVoidMethodV(JNIEnv* env, jobject obj, jmethodID meth, va_list args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    callVirtualV(jvm, thread, method, obj, args, &thread->nativeException);
}

extern JNICALL void SJVM_CallVoidMethod(JNIEnv* env, jobject obj, jmethodID meth, ...) {
    va_list args;
    va_start(args, meth);
    SJVM_CallVoidMethodV(env, obj, meth, args);
    va_end(args);
}

extern JNICALL void SJVM_CallVoidMethodA(JNIEnv* env, jobject obj, jmethodID meth, const jvalue* args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    callVirtualA(jvm, thread, method, obj, args, &thread->nativeException);
}

extern JNICALL jobject SJVM_CallNonvirtualObjectMethodV(JNIEnv* env, jobject obj, jclass cls, jmethodID meth, va_list args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    return (jobject) callV(jvm, thread, method, obj, args, &thread->nativeException).l;
}

extern JNICALL jobject SJVM_CallNonvirtualObjectMethod(JNIEnv* env, jobject obj, jclass cls, jmethodID meth, ...) {
    va_list args;
    jobject ret;
    va_start(args, meth);
    ret = SJVM_CallNonvirtualObjectMethodV(env, obj, cls, meth, args);
    va_end(args);
    return ret;
}

extern JNICALL jobject SJVM_CallNonvirtualObjectMethodA(JNIEnv* env, jobject obj, jclass cls, jmethodID meth, const jvalue* args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    return (jobject) callA(jvm, thread, method, obj, args, &thread->nativeException).l;
}

extern JNICALL jboolean SJVM_CallNonvirtualBooleanMethodV(JNIEnv* env, jobject obj, jclass cls, jmethodID meth, va_list args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    return (jboolean) callV(jvm, thread, method, obj, args, &thread->nativeException).z;
}

extern JNICALL jboolean SJVM_CallNonvirtualBooleanMethod(JNIEnv* env, jobject obj, jclass cls, jmethodID meth, ...) {
    va_list args;
    jboolean ret;
    va_start(args, meth);
    ret = SJVM_CallNonvirtualBooleanMethodV(env, obj, cls, meth, args);
    va_end(args);
    return ret;
}

extern JNICALL jboolean SJVM_CallNonvirtualBooleanMethodA(JNIEnv* env, jobject obj, jclass cls, jmethodID meth, const jvalue* args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    return (jboolean) callA(jvm, thread, method, obj, args, &thread->nativeException).z;
}

extern JNICALL jbyte SJVM_CallNonvirtualByteMethodV(JNIEnv* env, jobject obj, jclass cls, jmethodID meth, va_list args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    return (jbyte) callV(jvm, thread, method, obj, args, &thread->nativeException).b;
}

extern JNICALL jbyte SJVM_CallNonvirtualByteMethod(JNIEnv* env, jobject obj, jclass cls, jmethodID meth, ...) {
    va_list args;
    jbyte ret;
    va_start(args, meth);
    ret = SJVM_CallNonvirtualByteMethodV(env, obj, cls, meth, args);
    va_end(args);
    return ret;
}

extern JNICALL jbyte SJVM_CallNonvirtualByteMethodA(JNIEnv* env, jobject obj, jclass cls, jmethodID meth, const jvalue* args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    return (jbyte) callA(jvm, thread, method, obj, args, &thread->nativeException).b;
}

extern JNICALL jchar SJVM_CallNonvirtualCharMethodV(JNIEnv* env, jobject obj, jclass cls, jmethodID meth, va_list args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    return (jchar) callV(jvm, thread, method, obj, args, &thread->nativeException).c;
}

extern JNICALL jchar SJVM_CallNonvirtualCharMethod(JNIEnv* env, jobject obj, jclass cls, jmethodID meth, ...) {
    va_list args;
    jchar ret;
    va_start(args, meth);
    ret = SJVM_CallNonvirtualCharMethodV(env, obj, cls, meth, args);
    va_end(args);
    return ret;
}

extern JNICALL jchar SJVM_CallNonvirtualCharMethodA(JNIEnv* env, jobject obj, jclass cls, jmethodID meth, const jvalue* args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    return (jchar) callA(jvm, thread, method, obj, args, &thread->nativeException).c;
}

extern JNICALL jshort SJVM_CallNonvirtualShortMethodV(JNIEnv* env, jobject obj, jclass cls, jmethodID meth, va_list args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    return (jshort) callV(jvm, thread, method, obj, args, &thread->nativeException).s;
}

extern JNICALL jshort SJVM_CallNonvirtualShortMethod(JNIEnv* env, jobject obj, jclass cls, jmethodID meth, ...) {
    va_list args;
    jshort ret;
    va_start(args, meth);
    ret = SJVM_CallNonvirtualShortMethodV(env, obj, cls, meth, args);
    va_end(args);
    return ret;
}

extern JNICALL jshort SJVM_CallNonvirtualShortMethodA(JNIEnv* env, jobject obj, jclass cls, jmethodID meth, const jvalue* args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    return (jshort) callA(jvm, thread, method, obj, args, &thread->nativeException).s;
}

extern JNICALL jint SJVM_CallNonvirtualIntMethodV(JNIEnv* env, jobject obj, jclass cls, jmethodID meth, va_list args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    return (jint) callV(jvm, thread, method, obj, args, &thread->nativeException).i;
}

extern JNICALL jint SJVM_CallNonvirtualIntMethod(JNIEnv* env, jobject obj, jclass cls, jmethodID meth, ...) {
    va_list args;
    jint ret;
    va_start(args, meth);
    ret = SJVM_CallNonvirtualIntMethodV(env, obj, cls, meth, args);
    va_end(args);
    return ret;
}

extern JNICALL jint SJVM_CallNonvirtualIntMethodA(JNIEnv* env, jobject obj, jclass cls, jmethodID meth, const jvalue* args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    return (jint) callA(jvm, thread, method, obj, args, &thread->nativeException).i;
}

extern JNICALL jlong SJVM_CallNonvirtualLongMethodV(JNIEnv* env, jobject obj, jclass cls, jmethodID meth, va_list args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    return (jlong) callV(jvm, thread, method, obj, args, &thread->nativeException).j;
}

extern JNICALL jlong SJVM_CallNonvirtualLongMethod(JNIEnv* env, jobject obj, jclass cls, jmethodID meth, ...) {
    va_list args;
    jlong ret;
    va_start(args, meth);
    ret = SJVM_CallNonvirtualLongMethodV(env, obj, cls, meth, args);
    va_end(args);
    return ret;
}

extern JNICALL jlong SJVM_CallNonvirtualLongMethodA(JNIEnv* env, jobject obj, jclass cls, jmethodID meth, const jvalue* args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    return (jlong) callA(jvm, thread, method, obj, args, &thread->nativeException).j;
}

extern JNICALL jfloat SJVM_CallNonvirtualFloatMethodV(JNIEnv* env, jobject obj, jclass cls, jmethodID meth, va_list args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    return (jfloat) callV(jvm, thread, method, obj, args, &thread->nativeException).f;
}

extern JNICALL jfloat SJVM_CallNonvirtualFloatMethod(JNIEnv* env, jobject obj, jclass cls, jmethodID meth, ...) {
    va_list args;
    jfloat ret;
    va_start(args, meth);
    ret = SJVM_CallNonvirtualFloatMethodV(env, obj, cls, meth, args);
    va_end(args);
    return ret;
}

extern JNICALL jfloat SJVM_CallNonvirtualFloatMethodA(JNIEnv* env, jobject obj, jclass cls, jmethodID meth, const jvalue* args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    return (jfloat) callA(jvm, thread, method, obj, args, &thread->nativeException).f;
}

extern JNICALL jdouble SJVM_CallNonvirtualDoubleMethodV(JNIEnv* env, jobject obj, jclass cls, jmethodID meth, va_list args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    return (jdouble) callV(jvm, thread, method, obj, args, &thread->nativeException).d;
}

extern JNICALL jdouble SJVM_CallNonvirtualDoubleMethod(JNIEnv* env, jobject obj, jclass cls, jmethodID meth, ...) {
    va_list args;
    jdouble ret;
    va_start(args, meth);
    ret = SJVM_CallNonvirtualDoubleMethodV(env, obj, cls, meth, args);
    va_end(args);
    return ret;
}

extern JNICALL jdouble SJVM_CallNonvirtualDoubleMethodA(JNIEnv* env, jobject obj, jclass cls, jmethodID meth, const jvalue* args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    return (jdouble) callA(jvm, thread, method, obj, args, &thread->nativeException).d;
}

extern JNICALL void SJVM_CallNonvirtualVoidMethodV(JNIEnv* env, jobject obj, jclass cls, jmethodID meth, va_list args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    callV(jvm, thread, method, obj, args, &thread->nativeException);
}

extern JNICALL void SJVM_CallNonvirtualVoidMethod(JNIEnv* env, jobject obj, jclass cls, jmethodID meth, ...) {
    va_list args;
    va_start(args, meth);
    SJVM_CallNonvirtualVoidMethodV(env, obj, cls, meth, args);
    va_end(args);
}

extern JNICALL void SJVM_CallNonvirtualVoidMethodA(JNIEnv* env, jobject obj, jclass cls, jmethodID meth, const jvalue* args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    callA(jvm, thread, method, obj, args, &thread->nativeException);
}

extern JNICALL jfieldID SJVM_GetFieldID(JNIEnv* env, jclass cls, const char* name, const char* sig) {
    return (jfieldID) ((JCLASS) cls)->fieldMap[name];
}

extern JNICALL jobject SJVM_GetObjectField(JNIEnv* env, jobject obj, jfieldID fld) {
    JOBJECT got = ((JOBJECT) obj)->fields[((FieldInfo*) fld)->index].l;
    ((Thread*) env->functions->reserved1)->nativeLocal->addLocal(got);
    return (jobject) got;
}

extern JNICALL jboolean SJVM_GetBooleanField(JNIEnv* env, jobject obj, jfieldID fld) {
    return ((JOBJECT) obj)->fields[((FieldInfo*) fld)->index].z;
}

extern JNICALL jbyte SJVM_GetByteField(JNIEnv* env, jobject obj, jfieldID fld) {
    return ((JOBJECT) obj)->fields[((FieldInfo*) fld)->index].b;
}

extern JNICALL jchar SJVM_GetCharField(JNIEnv* env, jobject obj, jfieldID fld) {
    return ((JOBJECT) obj)->fields[((FieldInfo*) fld)->index].c;
}

extern JNICALL jshort SJVM_GetShortField(JNIEnv* env, jobject obj, jfieldID fld) {
    return ((JOBJECT) obj)->fields[((FieldInfo*) fld)->index].s;
}

extern JNICALL jint SJVM_GetIntField(JNIEnv* env, jobject obj, jfieldID fld) {
    return ((JOBJECT) obj)->fields[((FieldInfo*) fld)->index].i;
}

extern JNICALL jlong SJVM_GetLongField(JNIEnv* env, jobject obj, jfieldID fld) {
    return ((JOBJECT) obj)->fields[((FieldInfo*) fld)->index].j;
}

extern JNICALL jfloat SJVM_GetFloatField(JNIEnv* env, jobject obj, jfieldID fld) {
    return ((JOBJECT) obj)->fields[((FieldInfo*) fld)->index].f;
}

extern JNICALL jdouble SJVM_GetDoubleField(JNIEnv* env, jobject obj, jfieldID fld) {
    return ((JOBJECT) obj)->fields[((FieldInfo*) fld)->index].d;
}

extern JNICALL void SJVM_SetObjectField(JNIEnv* env, jobject obj, jfieldID fld, jobject val) {
    JOBJECT old = ((JOBJECT) obj)->fields[((FieldInfo*) fld)->index].l;
    if (old) ((SJVM*) env->functions->reserved0)->gc->decrRef(old);
    ((JOBJECT) obj)->fields[((FieldInfo*) fld)->index].l = (JOBJECT) val;
    ((SJVM*) env->functions->reserved0)->gc->incrRef((JOBJECT) val);
}

extern JNICALL void SJVM_SetBooleanField(JNIEnv* env, jobject obj, jfieldID fld, jboolean val) {
    ((JOBJECT) obj)->fields[((FieldInfo*) fld)->index].z = val;
}

extern JNICALL void SJVM_SetByteField(JNIEnv* env, jobject obj, jfieldID fld, jbyte val) {
    ((JOBJECT) obj)->fields[((FieldInfo*) fld)->index].b = val;
}

extern JNICALL void SJVM_SetCharField(JNIEnv* env, jobject obj, jfieldID fld, jchar val) {
    ((JOBJECT) obj)->fields[((FieldInfo*) fld)->index].c = val;
}

extern JNICALL void SJVM_SetShortField(JNIEnv* env, jobject obj, jfieldID fld, jshort val) {
    ((JOBJECT) obj)->fields[((FieldInfo*) fld)->index].s = val;
}

extern JNICALL void SJVM_SetIntField(JNIEnv* env, jobject obj, jfieldID fld, jint val) {
    ((JOBJECT) obj)->fields[((FieldInfo*) fld)->index].i = val;
}

extern JNICALL void SJVM_SetLongField(JNIEnv* env, jobject obj, jfieldID fld, jlong val) {
    ((JOBJECT) obj)->fields[((FieldInfo*) fld)->index].j = val;
}

extern JNICALL void SJVM_SetFloatField(JNIEnv* env, jobject obj, jfieldID fld, jfloat val) {
    ((JOBJECT) obj)->fields[((FieldInfo*) fld)->index].f = val;
}

extern JNICALL void SJVM_SetDoubleField(JNIEnv* env, jobject obj, jfieldID fld, jdouble val) {
    ((JOBJECT) obj)->fields[((FieldInfo*) fld)->index].d = val;
}

extern JNICALL jmethodID SJVM_GetStaticMethodID(JNIEnv* env, jclass cls, const char* name, const char* sig) {
    std::string full(name);
    full.append(sig);
    return (jmethodID) ((JCLASS) cls)->methodMap[full];
}

extern JNICALL jobject SJVM_CallStaticObjectMethodV(JNIEnv* env, jclass cls, jmethodID meth, va_list args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    return (jobject) callStaticV(jvm, thread, method, args, &thread->nativeException).l;
}

extern JNICALL jobject SJVM_CallStaticObjectMethod(JNIEnv* env, jclass cls, jmethodID meth, ...) {
    va_list args;
    jobject ret;
    va_start(args, meth);
    ret = SJVM_CallStaticObjectMethodV(env, cls, meth, args);
    va_end(args);
    return ret;
}

extern JNICALL jobject SJVM_CallStaticObjectMethodA(JNIEnv* env, jclass cls, jmethodID meth, const jvalue* args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    return (jobject) callStaticA(jvm, thread, method, args, &thread->nativeException).l;
}

extern JNICALL jboolean SJVM_CallStaticBooleanMethodV(JNIEnv* env, jclass cls, jmethodID meth, va_list args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    return (jboolean) callStaticV(jvm, thread, method, args, &thread->nativeException).z;
}

extern JNICALL jboolean SJVM_CallStaticBooleanMethod(JNIEnv* env, jclass cls, jmethodID meth, ...) {
    va_list args;
    jboolean ret;
    va_start(args, meth);
    ret = SJVM_CallStaticBooleanMethodV(env, cls, meth, args);
    va_end(args);
    return ret;
}

extern JNICALL jboolean SJVM_CallStaticBooleanMethodA(JNIEnv* env, jclass cls, jmethodID meth, const jvalue* args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    return (jboolean) callStaticA(jvm, thread, method, args, &thread->nativeException).z;
}

extern JNICALL jbyte SJVM_CallStaticByteMethodV(JNIEnv* env, jclass cls, jmethodID meth, va_list args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    return (jbyte) callStaticV(jvm, thread, method, args, &thread->nativeException).b;
}

extern JNICALL jbyte SJVM_CallStaticByteMethod(JNIEnv* env, jclass cls, jmethodID meth, ...) {
    va_list args;
    jbyte ret;
    va_start(args, meth);
    ret = SJVM_CallStaticByteMethodV(env, cls, meth, args);
    va_end(args);
    return ret;
}

extern JNICALL jbyte SJVM_CallStaticByteMethodA(JNIEnv* env, jclass cls, jmethodID meth, const jvalue* args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    return (jbyte) callStaticA(jvm, thread, method, args, &thread->nativeException).b;
}

extern JNICALL jchar SJVM_CallStaticCharMethodV(JNIEnv* env, jclass cls, jmethodID meth, va_list args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    return (jchar) callStaticV(jvm, thread, method, args, &thread->nativeException).c;
}

extern JNICALL jchar SJVM_CallStaticCharMethod(JNIEnv* env, jclass cls, jmethodID meth, ...) {
    va_list args;
    jchar ret;
    va_start(args, meth);
    ret = SJVM_CallStaticCharMethodV(env, cls, meth, args);
    va_end(args);
    return ret;
}

extern JNICALL jchar SJVM_CallStaticCharMethodA(JNIEnv* env, jclass cls, jmethodID meth, const jvalue* args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    return (jchar) callStaticA(jvm, thread, method, args, &thread->nativeException).c;
}

extern JNICALL jshort SJVM_CallStaticShortMethodV(JNIEnv* env, jclass cls, jmethodID meth, va_list args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    return (jshort) callStaticV(jvm, thread, method, args, &thread->nativeException).s;
}

extern JNICALL jshort SJVM_CallStaticShortMethod(JNIEnv* env, jclass cls, jmethodID meth, ...) {
    va_list args;
    jshort ret;
    va_start(args, meth);
    ret = SJVM_CallStaticShortMethodV(env, cls, meth, args);
    va_end(args);
    return ret;
}

extern JNICALL jshort SJVM_CallStaticShortMethodA(JNIEnv* env, jclass cls, jmethodID meth, const jvalue* args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    return (jshort) callStaticA(jvm, thread, method, args, &thread->nativeException).s;
}

extern JNICALL jint SJVM_CallStaticIntMethodV(JNIEnv* env, jclass cls, jmethodID meth, va_list args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    return (jint) callStaticV(jvm, thread, method, args, &thread->nativeException).i;
}

extern JNICALL jint SJVM_CallStaticIntMethod(JNIEnv* env, jclass cls, jmethodID meth, ...) {
    va_list args;
    jint ret;
    va_start(args, meth);
    ret = SJVM_CallStaticIntMethodV(env, cls, meth, args);
    va_end(args);
    return ret;
}

extern JNICALL jint SJVM_CallStaticIntMethodA(JNIEnv* env, jclass cls, jmethodID meth, const jvalue* args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    return (jint) callStaticA(jvm, thread, method, args, &thread->nativeException).i;
}

extern JNICALL jlong SJVM_CallStaticLongMethodV(JNIEnv* env, jclass cls, jmethodID meth, va_list args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    return (jlong) callStaticV(jvm, thread, method, args, &thread->nativeException).j;
}

extern JNICALL jlong SJVM_CallStaticLongMethod(JNIEnv* env, jclass cls, jmethodID meth, ...) {
    va_list args;
    jlong ret;
    va_start(args, meth);
    ret = SJVM_CallStaticLongMethodV(env, cls, meth, args);
    va_end(args);
    return ret;
}

extern JNICALL jlong SJVM_CallStaticLongMethodA(JNIEnv* env, jclass cls, jmethodID meth, const jvalue* args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    return (jlong) callStaticA(jvm, thread, method, args, &thread->nativeException).j;
}

extern JNICALL jfloat SJVM_CallStaticFloatMethodV(JNIEnv* env, jclass cls, jmethodID meth, va_list args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    return (jfloat) callStaticV(jvm, thread, method, args, &thread->nativeException).f;
}

extern JNICALL jfloat SJVM_CallStaticFloatMethod(JNIEnv* env, jclass cls, jmethodID meth, ...) {
    va_list args;
    jfloat ret;
    va_start(args, meth);
    ret = SJVM_CallStaticFloatMethodV(env, cls, meth, args);
    va_end(args);
    return ret;
}

extern JNICALL jfloat SJVM_CallStaticFloatMethodA(JNIEnv* env, jclass cls, jmethodID meth, const jvalue* args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    return (jfloat) callStaticA(jvm, thread, method, args, &thread->nativeException).f;
}

extern JNICALL jdouble SJVM_CallStaticDoubleMethodV(JNIEnv* env, jclass cls, jmethodID meth, va_list args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    return (jdouble) callStaticV(jvm, thread, method, args, &thread->nativeException).d;
}

extern JNICALL jdouble SJVM_CallStaticDoubleMethod(JNIEnv* env, jclass cls, jmethodID meth, ...) {
    va_list args;
    jdouble ret;
    va_start(args, meth);
    ret = SJVM_CallStaticDoubleMethodV(env, cls, meth, args);
    va_end(args);
    return ret;
}

extern JNICALL jdouble SJVM_CallStaticDoubleMethodA(JNIEnv* env, jclass cls, jmethodID meth, const jvalue* args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    return (jdouble) callStaticA(jvm, thread, method, args, &thread->nativeException).d;
}

extern JNICALL void SJVM_CallStaticVoidMethodV(JNIEnv* env, jclass cls, jmethodID meth, va_list args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    callStaticV(jvm, thread, method, args, &thread->nativeException);
}

extern JNICALL void SJVM_CallStaticVoidMethod(JNIEnv* env, jclass cls, jmethodID meth, ...) {
    va_list args;
    va_start(args, meth);
    SJVM_CallStaticVoidMethodV(env, cls, meth, args);
    va_end(args);
}

extern JNICALL void SJVM_CallStaticVoidMethodA(JNIEnv* env, jclass cls, jmethodID meth, const jvalue* args) {
    MethodInfo* method = (MethodInfo*) meth;
    SJVM* jvm = (SJVM*) env->functions->reserved0;
    Thread* thread = (Thread*) env->functions->reserved1;
    callStaticA(jvm, thread, method, args, &thread->nativeException);
}

extern JNICALL jfieldID SJVM_GetStaticFieldID(JNIEnv* env, jclass cls, const char* name, const char* sig) {
    return (jfieldID) ((JCLASS) cls)->fieldMap[name];
}

extern JNICALL jobject SJVM_GetStaticObjectField(JNIEnv* env, jclass cls, jfieldID fld) {
    JOBJECT obj = ((FieldInfo*) fld)->staticValue.l;
    ((Thread*) env->functions->reserved1)->nativeLocal->addLocal(obj);
    return (jobject) obj;
}

extern JNICALL jboolean SJVM_GetStaticBooleanField(JNIEnv* env, jclass cls, jfieldID fld) {
    return ((FieldInfo*) fld)->staticValue.z;
}

extern JNICALL jbyte SJVM_GetStaticByteField(JNIEnv* env, jclass cls, jfieldID fld) {
    return ((FieldInfo*) fld)->staticValue.b;
}

extern JNICALL jchar SJVM_GetStaticCharField(JNIEnv* env, jclass cls, jfieldID fld) {
    return ((FieldInfo*) fld)->staticValue.c;
}

extern JNICALL jshort SJVM_GetStaticShortField(JNIEnv* env, jclass cls, jfieldID fld) {
    return ((FieldInfo*) fld)->staticValue.s;
}

extern JNICALL jint SJVM_GetStaticIntField(JNIEnv* env, jclass cls, jfieldID fld) {
    return ((FieldInfo*) fld)->staticValue.i;
}

extern JNICALL jlong SJVM_GetStaticLongField(JNIEnv* env, jclass cls, jfieldID fld) {
    return ((FieldInfo*) fld)->staticValue.j;
}

extern JNICALL jfloat SJVM_GetStaticFloatField(JNIEnv* env, jclass cls, jfieldID fld) {
    return ((FieldInfo*) fld)->staticValue.f;
}

extern JNICALL jdouble SJVM_GetStaticDoubleField(JNIEnv* env, jclass cls, jfieldID fld) {
    return ((FieldInfo*) fld)->staticValue.d;
}

extern JNICALL void SJVM_SetStaticObjectField(JNIEnv* env, jclass cls, jfieldID fld, jobject val) {
    JOBJECT old = ((FieldInfo*) fld)->staticValue.l;
    if (old) ((SJVM*) env->functions->reserved0)->gc->decrRef(old);
    ((FieldInfo*) fld)->staticValue.l = (JOBJECT) val;
    ((Thread*) env->functions->reserved1)->nativeLocal->addLocal((JOBJECT) val);
}

extern JNICALL void SJVM_SetStaticBooleanField(JNIEnv* env, jclass cls, jfieldID fld, jboolean val) {
    ((FieldInfo*) fld)->staticValue.z = val;
}

extern JNICALL void SJVM_SetStaticByteField(JNIEnv* env, jclass cls, jfieldID fld, jbyte val) {
    ((FieldInfo*) fld)->staticValue.b = val;
}

extern JNICALL void SJVM_SetStaticCharField(JNIEnv* env, jclass cls, jfieldID fld, jchar val) {
    ((FieldInfo*) fld)->staticValue.c = val;
}

extern JNICALL void SJVM_SetStaticShortField(JNIEnv* env, jclass cls, jfieldID fld, jshort val) {
    ((FieldInfo*) fld)->staticValue.s = val;
}

extern JNICALL void SJVM_SetStaticIntField(JNIEnv* env, jclass cls, jfieldID fld, jint val) {
    ((FieldInfo*) fld)->staticValue.i = val;
}

extern JNICALL void SJVM_SetStaticLongField(JNIEnv* env, jclass cls, jfieldID fld, jlong val) {
    ((FieldInfo*) fld)->staticValue.j = val;
}

extern JNICALL void SJVM_SetStaticFloatField(JNIEnv* env, jclass cls, jfieldID fld, jfloat val) {
    ((FieldInfo*) fld)->staticValue.f = val;
}

extern JNICALL void SJVM_SetStaticDoubleField(JNIEnv* env, jclass cls, jfieldID fld, jdouble val) {
    ((FieldInfo*) fld)->staticValue.d = val;
}

extern JNICALL jstring SJVM_NewString(JNIEnv* env, const jchar* data, jsize len) {
    JCLASS sclass = ((SJVM*) env->functions->reserved0)->loadClass("java/lang/String");
    JOBJECT sobj = sclass->newInstance((SJVM*) env->functions->reserved0);
    ((Thread*) env->functions->reserved1)->nativeLocal->addLocal(sobj);
    JCHARARRAY chars = ClassFile::newArray<JCHAR>((SJVM*)env->functions->reserved0,(char*)"[C",len);
    ((SJVM*) env->functions->reserved0)->gc->incrRef(chars);
    sobj->fields[sclass->fieldMap["value"]->index].l = chars;
    for (int i = 0; i < len; i++)
        chars->array[i] = data[i];
    sobj->fields[sclass->fieldMap["offset"]->index].i = 0;
    sobj->fields[sclass->fieldMap["count"]->index].i = len;
}

extern JNICALL jsize SJVM_GetStringLength(JNIEnv* env, jstring data) {
    return ((JOBJECT) data)->fields[((JOBJECT) data)->type->fieldMap["count"]->index].i;
}

extern JNICALL const jchar* SJVM_GetStringChars(JNIEnv* env, jstring data, jboolean* copy) {
    if (copy != NULL) *copy = JNI_FALSE;
    return ((JCHARARRAY) ((JOBJECT) data)->fields[((JOBJECT) data)->type->fieldMap["value"]->index].l)->array;
}

extern JNICALL jstring SJVM_NewStringUTF(JNIEnv* env, const char* data) {
    JCLASS sclass = ((SJVM*) env->functions->reserved0)->loadClass("java/lang/String");
    JOBJECT sobj = sclass->newInstance((SJVM*) env->functions->reserved0);
    ((Thread*) env->functions->reserved1)->nativeLocal->addLocal(sobj);
    int len = 0;
    int clen = strlen(data);
    unsigned char* bytes = (unsigned char*) data;
    for (int i = 0; i < clen;) {
        if ((bytes[i] & 0x80) == 0) {
            len++;
            i++;
        } else if ((bytes[i] & 0xE0) == 0xC0) {
            len += 2;
            i += 2;
        } else if ((bytes[i] & 0xF0) == 0xE0) {
            len += 3;
            i += 3;
        }
    }
    JCHARARRAY chars = ClassFile::newArray<JCHAR>((SJVM*)env->functions->reserved0,(char*)"[C",len);
    ((SJVM*) env->functions->reserved0)->gc->incrRef(chars);
    sobj->fields[sclass->fieldMap["value"]->index].l = chars;
    JCHAR* array = chars->array;
    for (int i = 0, c = 0; i < len; i++) {
        if ((bytes[c] & 0x80) == 0x00) {
            array[i] = bytes[c];
            c++;
        } else if ((bytes[c] & 0xE0) == 0xC0) {
            array[i] = ((bytes[c] & 0x1F) << 6) + (bytes[c + 1] & 0x3F);
            c += 2;
        } else if ((bytes[c] & 0xF0) == 0xE0) {
            array[i] = ((bytes[c] & 0xF) << 12) + ((bytes[c + 1] & 0x3F) << 6) + (bytes[c + 2] & 0x3F);
            c += 3;
        }
    }
    sobj->fields[sclass->fieldMap["offset"]->index].i = 0;
    sobj->fields[sclass->fieldMap["count"]->index].i = len;
    return (jstring) sobj;
}

extern JNICALL jsize SJVM_GetStringUTFLength(JNIEnv* env, jstring data) {
    JOBJECT sobj = ((JOBJECT) data);
    JCLASS sclass = sobj->type;
    JCHARARRAY chars = (JCHARARRAY) sobj->fields[sclass->fieldMap["value"]->index].l;
    int offset = sobj->fields[sclass->fieldMap["offset"]->index].i;
    int clen = sobj->fields[sclass->fieldMap["count"]->index].i;
    JCHAR* ptr = chars->array;
    int len = 0;
    for (int i = offset; i < clen; i++) {
        if (ptr[i] >= 0x0001 && ptr[i] <= 0x007F) {
            len += 1;
        } else if (ptr[i] >= 0x0080 && ptr[i] <= 0x07FF) {
            len += 2;
        } else {
            len += 3;
        }
    }
    return len;
}

extern JNICALL const char* SJVM_GetStringUTFChars(JNIEnv* env, jstring data, jboolean* copy) {
    JOBJECT sobj = ((JOBJECT) data);
    JCLASS sclass = sobj->type;
    JCHARARRAY chars = (JCHARARRAY) sobj->fields[sclass->fieldMap["value"]->index].l;
    int offset = sobj->fields[sclass->fieldMap["offset"]->index].i;
    int clen = sobj->fields[sclass->fieldMap["count"]->index].i;
    JCHAR* ptr = chars->array;
    int len = 0;
    for (int i = offset; i < clen; i++) {
        if (ptr[i] >= 0x0001 && ptr[i] <= 0x007F) {
            len += 1;
        } else if (ptr[i] >= 0x0080 && ptr[i] <= 0x07FF) {
            len += 2;
        } else {
            len += 3;
        }
    }
    char* buf = new char[len + 1];
    buf[len] = 0;
    if (copy != NULL) {
        *copy = JNI_TRUE;
    }
    for (int j = 0, i = offset; i < clen; i++) {
        if (ptr[i] >= 0x0001 && ptr[i] <= 0x007F) {
            buf[j++] = ptr[i] & 0x7F;
        } else if (ptr[i] >= 0x0080 && ptr[i] <= 0x07FF) {
            buf[j++] = 0xC0 | ((ptr[i] >> 6) & 0x1F);
            buf[j++] = 0x80 | (ptr[i] & 0x3F);
        } else {
            buf[j++] = 0xE0 | ((ptr[i] >> 12) & 0x0F);
            buf[j++] = 0x80 | ((ptr[i] >> 6) & 0x3F);
            buf[j++] = 0x80 | (ptr[i] & 0x3F);
        }
    }
    return buf;
}

extern JNICALL void SJVM_ReleaseStringUTFChars(JNIEnv* env, jstring data, const char* chars) {
    delete [] chars;
}

extern JNICALL jsize SJVM_GetArrayLength(JNIEnv* env, jarray arr) {
    return ((JOBJECTARRAY) arr)->size;
}

extern JNICALL jobjectArray SJVM_NewObjectArray(JNIEnv* env, jsize len, jclass cls, jobject init) {
    std::string str("[");
    str.append(((JCLASS)cls)->name);
    JOBJECTARRAY array = ClassFile::newArray<JOBJECT>((SJVM*)env->functions->reserved0,(char*)str.c_str(),len);
    for (int i = 0; i < len; i++) {
        array->array[i] = (JOBJECT) init;
    }
    return (jobjectArray) array;
    ((Thread*) env->functions->reserved1)->nativeLocal->addLocal(array);
}

extern JNICALL jobject SJVM_GetObjectArrayElement(JNIEnv* env, jobjectArray arr, jsize elem) {
    JOBJECT obj = ((JOBJECTARRAY) arr)->array[elem];
    ((Thread*) env->functions->reserved1)->nativeLocal->addLocal(obj);
    return (jobject) obj;
}

extern JNICALL void SJVM_SetObjectArrayElement(JNIEnv* env, jobjectArray arr, jsize elem, jobject val) {
    JOBJECT old = ((JOBJECTARRAY) arr)->array[elem];
    if (old) ((SJVM*) env->functions->reserved0)->gc->decrRef(old);
    ((JOBJECTARRAY) arr)->array[elem] = (JOBJECT) val;
    ((SJVM*) env->functions->reserved0)->gc->incrRef((JOBJECT) val);
}

extern JNICALL jbooleanArray SJVM_NewBooleanArray(JNIEnv* env, jsize len) {
    return (jbooleanArray) ClassFile::newArray<JBOOLEAN>((SJVM*)env->functions->reserved0,(char*)"[Z",len);
}

extern JNICALL jbyteArray SJVM_NewByteArray(JNIEnv* env, jsize len) {
    return (jbyteArray) ClassFile::newArray<JBYTE>((SJVM*)env->functions->reserved0,(char*)"[B",len);
}

extern JNICALL jcharArray SJVM_NewCharArray(JNIEnv* env, jsize len) {
    return (jcharArray) ClassFile::newArray<JCHAR>((SJVM*)env->functions->reserved0,(char*)"[C",len);
}

extern JNICALL jshortArray SJVM_NewShortArray(JNIEnv* env, jsize len) {
    return (jshortArray) ClassFile::newArray<JSHORT>((SJVM*)env->functions->reserved0,(char*)"[S",len);
}

extern JNICALL jintArray SJVM_NewIntArray(JNIEnv* env, jsize len) {
    return (jintArray) ClassFile::newArray<JINT>((SJVM*)env->functions->reserved0,(char*)"[I",len);
}

extern JNICALL jlongArray SJVM_NewLongArray(JNIEnv* env, jsize len) {
    return (jlongArray) ClassFile::newArray<JLONG>((SJVM*)env->functions->reserved0,(char*)"[J",len);
}

extern JNICALL jfloatArray SJVM_NewFloatArray(JNIEnv* env, jsize len) {
    return (jfloatArray) ClassFile::newArray<JFLOAT>((SJVM*)env->functions->reserved0,(char*)"[F",len);
}

extern JNICALL jdoubleArray SJVM_NewDoubleArray(JNIEnv* env, jsize len) {
    return (jdoubleArray) ClassFile::newArray<JDOUBLE>((SJVM*)env->functions->reserved0,(char*)"[D",len);
}

extern JNICALL jboolean* SJVM_GetBooleanArrayElements(JNIEnv* env, jbooleanArray arr, jboolean* iscopy) {
    if (iscopy != NULL) {
        *iscopy = JNI_FALSE;
    }
    return (jboolean*) ((JBOOLEANARRAY) arr)->array;
}

extern JNICALL jbyte* SJVM_GetByteArrayElements(JNIEnv* env, jbyteArray arr, jboolean* iscopy) {
    if (iscopy != NULL) {
        *iscopy = JNI_FALSE;
    }
    return (jbyte*) ((JBYTEARRAY) arr)->array;
}

extern JNICALL jchar* SJVM_GetCharArrayElements(JNIEnv* env, jcharArray arr, jboolean* iscopy) {
    if (iscopy != NULL) {
        *iscopy = JNI_FALSE;
    }
    return (jchar*) ((JCHARARRAY) arr)->array;
}

extern JNICALL jshort* SJVM_GetShortArrayElements(JNIEnv* env, jshortArray arr, jboolean* iscopy) {
    if (iscopy != NULL) {
        *iscopy = JNI_FALSE;
    }
    return (jshort*) ((JSHORTARRAY) arr)->array;
}

extern JNICALL jint* SJVM_GetIntArrayElements(JNIEnv* env, jintArray arr, jboolean* iscopy) {
    if (iscopy != NULL) {
        *iscopy = JNI_FALSE;
    }
    return (jint*) ((JINTARRAY) arr)->array;
}

extern JNICALL jlong* SJVM_GetLongArrayElements(JNIEnv* env, jlongArray arr, jboolean* iscopy) {
    if (iscopy != NULL) {
        *iscopy = JNI_FALSE;
    }
    return (jlong*) ((JLONGARRAY) arr)->array;
}

extern JNICALL jfloat* SJVM_GetFloatArrayElements(JNIEnv* env, jfloatArray arr, jboolean* iscopy) {
    if (iscopy != NULL) {
        *iscopy = JNI_FALSE;
    }
    return (jfloat*) ((JFLOATARRAY) arr)->array;
}

extern JNICALL jdouble* SJVM_GetDoubleArrayElements(JNIEnv* env, jdoubleArray arr, jboolean* iscopy) {
    if (iscopy != NULL) {
        *iscopy = JNI_FALSE;
    }
    return (jdouble*) ((JDOUBLEARRAY) arr)->array;
}

extern JNICALL void SJVM_ReleaseBooleanArrayElements(JNIEnv* env, jbooleanArray arr, jboolean* elems, jint mode) {
    //nothing
}

extern JNICALL void SJVM_ReleaseByteArrayElements(JNIEnv* env, jbyteArray arr, jbyte* elems, jint mode) {
    //nothing
}

extern JNICALL void SJVM_ReleaseCharArrayElements(JNIEnv* env, jcharArray arr, jchar* elems, jint mode) {
    //nothing
}

extern JNICALL void SJVM_ReleaseShortArrayElements(JNIEnv* env, jshortArray arr, jshort* elems, jint mode) {
    //nothing
}

extern JNICALL void SJVM_ReleaseIntArrayElements(JNIEnv* env, jintArray arr, jint* elems, jint mode) {
    //nothing
}

extern JNICALL void SJVM_ReleaseLongArrayElements(JNIEnv* env, jlongArray arr, jlong* elems, jint mode) {
    //nothing
}

extern JNICALL void SJVM_ReleaseFloatArrayElements(JNIEnv* env, jfloatArray arr, jfloat* elems, jint mode) {
    //nothing
}

extern JNICALL void SJVM_ReleaseDoubleArrayElements(JNIEnv* env, jdoubleArray arr, jdouble* elems, jint mode) {
    //nothing
}

extern JNICALL void SJVM_GetBooleanArrayRegion(JNIEnv* env, jbooleanArray arr, jsize start, jsize len, jboolean* data) {
    memcpy(data, &((JBOOLEANARRAY) arr)->array[start], len * sizeof (jboolean));
}

extern JNICALL void SJVM_GetByteArrayRegion(JNIEnv* env, jbyteArray arr, jsize start, jsize len, jbyte* data) {
    memcpy(data, &((JBYTEARRAY) arr)->array[start], len * sizeof (jbyte));
}

extern JNICALL void SJVM_GetCharArrayRegion(JNIEnv* env, jcharArray arr, jsize start, jsize len, jchar* data) {
    memcpy(data, &((JCHARARRAY) arr)->array[start], len * sizeof (jchar));
}

extern JNICALL void SJVM_GetShortArrayRegion(JNIEnv* env, jshortArray arr, jsize start, jsize len, jshort* data) {
    memcpy(data, &((JSHORTARRAY) arr)->array[start], len * sizeof (jshort));
}

extern JNICALL void SJVM_GetIntArrayRegion(JNIEnv* env, jintArray arr, jsize start, jsize len, jint* data) {
    memcpy(data, &((JINTARRAY) arr)->array[start], len * sizeof (jint));
}

extern JNICALL void SJVM_GetLongArrayRegion(JNIEnv* env, jlongArray arr, jsize start, jsize len, jlong* data) {
    memcpy(data, &((JLONGARRAY) arr)->array[start], len * sizeof (jlong));
}

extern JNICALL void SJVM_GetFloatArrayRegion(JNIEnv* env, jfloatArray arr, jsize start, jsize len, jfloat* data) {
    memcpy(data, &((JFLOATARRAY) arr)->array[start], len * sizeof (jfloat));
}

extern JNICALL void SJVM_GetDoubleArrayRegion(JNIEnv* env, jdoubleArray arr, jsize start, jsize len, jdouble* data) {
    memcpy(data, &((JDOUBLEARRAY) arr)->array[start], len * sizeof (jdouble));
}

extern JNICALL void SJVM_SetBooleanArrayRegion(JNIEnv* env, jbooleanArray arr, jsize start, jsize len, const jboolean* data) {
    Array<char> *array = (Array<char>*)arr;
    memcpy(array->array + sizeof (jboolean) * start, (char*) data, sizeof (jboolean) * len);
}

extern JNICALL void SJVM_SetByteArrayRegion(JNIEnv* env, jbyteArray arr, jsize start, jsize len, const jbyte* data) {
    Array<char> *array = (Array<char>*)arr;
    memcpy(array->array + sizeof (jbyte) * start, (char*) data, sizeof (jbyte) * len);
}

extern JNICALL void SJVM_SetCharArrayRegion(JNIEnv* env, jcharArray arr, jsize start, jsize len, const jchar* data) {
    Array<char> *array = (Array<char>*)arr;
    memcpy(array->array + sizeof (jchar) * start, (char*) data, sizeof (jchar) * len);
}

extern JNICALL void SJVM_SetShortArrayRegion(JNIEnv* env, jshortArray arr, jsize start, jsize len, const jshort* data) {
    Array<char> *array = (Array<char>*)arr;
    memcpy(array->array + sizeof (jshort) * start, (char*) data, sizeof (jshort) * len);
}

extern JNICALL void SJVM_SetIntArrayRegion(JNIEnv* env, jintArray arr, jsize start, jsize len, const jint* data) {
    Array<char> *array = (Array<char>*)arr;
    memcpy(array->array + sizeof (jint) * start, (char*) data, sizeof (jint) * len);
}

extern JNICALL void SJVM_SetLongArrayRegion(JNIEnv* env, jlongArray arr, jsize start, jsize len, const jlong* data) {
    Array<char> *array = (Array<char>*)arr;
    memcpy(array->array + sizeof (jlong) * start, (char*) data, sizeof (jlong) * len);
}

extern JNICALL void SJVM_SetFloatArrayRegion(JNIEnv* env, jfloatArray arr, jsize start, jsize len, const jfloat* data) {
    Array<char> *array = (Array<char>*)arr;
    memcpy(array->array + sizeof (jfloat) * start, (char*) data, sizeof (jfloat) * len);
}

extern JNICALL void SJVM_SetDoubleArrayRegion(JNIEnv* env, jdoubleArray arr, jsize start, jsize len, const jdouble* data) {
    Array<char> *array = (Array<char>*)arr;
    memcpy(array->array + sizeof (jdouble) * start, (char*) data, sizeof (jdouble) * len);
}

extern JNICALL jint SJVM_RegisterNatives(JNIEnv* env, jclass cls, const JNINativeMethod* methods, jint nmethods) {
    return JNI_ERR;
}

extern JNICALL jint SJVM_UnregisterNatives(JNIEnv* env, jclass cls) {
    return JNI_ERR;
}

extern JNICALL jint SJVM_MonitorEnter(JNIEnv* env, jobject obj) {
    ((JOBJECT) obj)->mutex.lock();
    return JNI_OK;
}

extern JNICALL jint SJVM_MonitorExit(JNIEnv* env, jobject obj) {
    ((JOBJECT) obj)->mutex.unlock();
    return JNI_OK;
}

void SJVM_JNIExceptionHandler(void) {

}

extern JNICALL jint SJVM_GetJavaVM(JNIEnv* env, JavaVM** vm) {
    *vm = ((SJVM*) env)->getJavaVM();
    return JNI_OK;
}

extern JNICALL jint SJVM_DestroyJavaVM(JavaVM* vm) {
    SJVM *sjvm = ((SJVM*) vm->functions->reserved0);
    delete sjvm;
    return JNI_OK;
}

extern JNICALL jint SJVM_AttachCurrentThread(JavaVM* vm, void** env, void* args) {
    Thread* thread = Thread::getThread((SJVM*) vm->functions->reserved0);
    if (env) *((JNIEnv**) env) = thread->getEnv();
    return JNI_OK;
}

extern JNICALL jint SJVM_DetachCurrentThread(JavaVM* vm) {
    //FIXME detach thread
    return JNI_ERR;
}

extern JNICALL jint SJVM_GetEnv(JavaVM* vm, void** penv, jint interface_id) {
    //FIXME attaches thread...
    Thread* thread = Thread::getThread((SJVM*) vm->functions->reserved0);
    if (penv) *((JNIEnv**) penv) = thread->getEnv();
    return JNI_OK;
}

struct JNINativeInterface_ SJVM_JNINativeInterface = {
    NULL,
    NULL,
    NULL,
    NULL,
    SJVM_GetVersion,
    SJVM_DefineClass,
    SJVM_FindClass,
    NULL,
    NULL,
    NULL,
    SJVM_GetSuperClass,
    SJVM_IsAssignableFrom,
    NULL,
    SJVM_Throw,
    SJVM_ThrowNew,
    SJVM_ExceptionOccurred,
    SJVM_ExceptionDescribe,
    SJVM_ExceptionClear,
    SJVM_FatalError,
    NULL,
    NULL,
    SJVM_NewGlobalRef,
    SJVM_DeleteGlobalRef,
    SJVM_DeleteLocalRef,
    SJVM_IsSameObject,
    NULL,
    NULL,
    SJVM_AllocObject,
    SJVM_NewObject,
    SJVM_NewObjectV,
    SJVM_NewObjectA,
    SJVM_GetObjectClass,
    SJVM_IsInstanceOf,
    SJVM_GetMethodID,
    SJVM_CallObjectMethod,
    SJVM_CallObjectMethodV,
    SJVM_CallObjectMethodA,
    SJVM_CallBooleanMethod,
    SJVM_CallBooleanMethodV,
    SJVM_CallBooleanMethodA,
    SJVM_CallByteMethod,
    SJVM_CallByteMethodV,
    SJVM_CallByteMethodA,
    SJVM_CallCharMethod,
    SJVM_CallCharMethodV,
    SJVM_CallCharMethodA,
    SJVM_CallShortMethod,
    SJVM_CallShortMethodV,
    SJVM_CallShortMethodA,
    SJVM_CallIntMethod,
    SJVM_CallIntMethodV,
    SJVM_CallIntMethodA,
    SJVM_CallLongMethod,
    SJVM_CallLongMethodV,
    SJVM_CallLongMethodA,
    SJVM_CallFloatMethod,
    SJVM_CallFloatMethodV,
    SJVM_CallFloatMethodA,
    SJVM_CallDoubleMethod,
    SJVM_CallDoubleMethodV,
    SJVM_CallDoubleMethodA,
    SJVM_CallVoidMethod,
    SJVM_CallVoidMethodV,
    SJVM_CallVoidMethodA,
    SJVM_CallNonvirtualObjectMethod,
    SJVM_CallNonvirtualObjectMethodV,
    SJVM_CallNonvirtualObjectMethodA,
    SJVM_CallNonvirtualBooleanMethod,
    SJVM_CallNonvirtualBooleanMethodV,
    SJVM_CallNonvirtualBooleanMethodA,
    SJVM_CallNonvirtualByteMethod,
    SJVM_CallNonvirtualByteMethodV,
    SJVM_CallNonvirtualByteMethodA,
    SJVM_CallNonvirtualCharMethod,
    SJVM_CallNonvirtualCharMethodV,
    SJVM_CallNonvirtualCharMethodA,
    SJVM_CallNonvirtualShortMethod,
    SJVM_CallNonvirtualShortMethodV,
    SJVM_CallNonvirtualShortMethodA,
    SJVM_CallNonvirtualIntMethod,
    SJVM_CallNonvirtualIntMethodV,
    SJVM_CallNonvirtualIntMethodA,
    SJVM_CallNonvirtualLongMethod,
    SJVM_CallNonvirtualLongMethodV,
    SJVM_CallNonvirtualLongMethodA,
    SJVM_CallNonvirtualFloatMethod,
    SJVM_CallNonvirtualFloatMethodV,
    SJVM_CallNonvirtualFloatMethodA,
    SJVM_CallNonvirtualDoubleMethod,
    SJVM_CallNonvirtualDoubleMethodV,
    SJVM_CallNonvirtualDoubleMethodA,
    SJVM_CallNonvirtualVoidMethod,
    SJVM_CallNonvirtualVoidMethodV,
    SJVM_CallNonvirtualVoidMethodA,
    SJVM_GetFieldID,
    SJVM_GetObjectField,
    SJVM_GetBooleanField,
    SJVM_GetByteField,
    SJVM_GetCharField,
    SJVM_GetShortField,
    SJVM_GetIntField,
    SJVM_GetLongField,
    SJVM_GetFloatField,
    SJVM_GetDoubleField,
    SJVM_SetObjectField,
    SJVM_SetBooleanField,
    SJVM_SetByteField,
    SJVM_SetCharField,
    SJVM_SetShortField,
    SJVM_SetIntField,
    SJVM_SetLongField,
    SJVM_SetFloatField,
    SJVM_SetDoubleField,
    SJVM_GetStaticMethodID,
    SJVM_CallStaticObjectMethod,
    SJVM_CallStaticObjectMethodV,
    SJVM_CallStaticObjectMethodA,
    SJVM_CallStaticBooleanMethod,
    SJVM_CallStaticBooleanMethodV,
    SJVM_CallStaticBooleanMethodA,
    SJVM_CallStaticByteMethod,
    SJVM_CallStaticByteMethodV,
    SJVM_CallStaticByteMethodA,
    SJVM_CallStaticCharMethod,
    SJVM_CallStaticCharMethodV,
    SJVM_CallStaticCharMethodA,
    SJVM_CallStaticShortMethod,
    SJVM_CallStaticShortMethodV,
    SJVM_CallStaticShortMethodA,
    SJVM_CallStaticIntMethod,
    SJVM_CallStaticIntMethodV,
    SJVM_CallStaticIntMethodA,
    SJVM_CallStaticLongMethod,
    SJVM_CallStaticLongMethodV,
    SJVM_CallStaticLongMethodA,
    SJVM_CallStaticFloatMethod,
    SJVM_CallStaticFloatMethodV,
    SJVM_CallStaticFloatMethodA,
    SJVM_CallStaticDoubleMethod,
    SJVM_CallStaticDoubleMethodV,
    SJVM_CallStaticDoubleMethodA,
    SJVM_CallStaticVoidMethod,
    SJVM_CallStaticVoidMethodV,
    SJVM_CallStaticVoidMethodA,
    SJVM_GetStaticFieldID,
    SJVM_GetStaticObjectField,
    SJVM_GetStaticBooleanField,
    SJVM_GetStaticByteField,
    SJVM_GetStaticCharField,
    SJVM_GetStaticShortField,
    SJVM_GetStaticIntField,
    SJVM_GetStaticLongField,
    SJVM_GetStaticFloatField,
    SJVM_GetStaticDoubleField,
    SJVM_SetStaticObjectField,
    SJVM_SetStaticBooleanField,
    SJVM_SetStaticByteField,
    SJVM_SetStaticCharField,
    SJVM_SetStaticShortField,
    SJVM_SetStaticIntField,
    SJVM_SetStaticLongField,
    SJVM_SetStaticFloatField,
    SJVM_SetStaticDoubleField,
    SJVM_NewString,
    SJVM_GetStringLength,
    SJVM_GetStringChars,
    SJVM_ReleaseStringChars,
    SJVM_NewStringUTF,
    SJVM_GetStringUTFLength,
    SJVM_GetStringUTFChars,
    SJVM_ReleaseStringUTFChars,
    SJVM_GetArrayLength,
    SJVM_NewObjectArray,
    SJVM_GetObjectArrayElement,
    SJVM_SetObjectArrayElement,
    SJVM_NewBooleanArray,
    SJVM_NewByteArray,
    SJVM_NewCharArray,
    SJVM_NewShortArray,
    SJVM_NewIntArray,
    SJVM_NewLongArray,
    SJVM_NewFloatArray,
    SJVM_NewDoubleArray,
    SJVM_GetBooleanArrayElements,
    SJVM_GetByteArrayElements,
    SJVM_GetCharArrayElements,
    SJVM_GetShortArrayElements,
    SJVM_GetIntArrayElements,
    SJVM_GetLongArrayElements,
    SJVM_GetFloatArrayElements,
    SJVM_GetDoubleArrayElements,
    SJVM_ReleaseBooleanArrayElements,
    SJVM_ReleaseByteArrayElements,
    SJVM_ReleaseCharArrayElements,
    SJVM_ReleaseShortArrayElements,
    SJVM_ReleaseIntArrayElements,
    SJVM_ReleaseLongArrayElements,
    SJVM_ReleaseFloatArrayElements,
    SJVM_ReleaseDoubleArrayElements,
    SJVM_GetBooleanArrayRegion,
    SJVM_GetByteArrayRegion,
    SJVM_GetCharArrayRegion,
    SJVM_GetShortArrayRegion,
    SJVM_GetIntArrayRegion,
    SJVM_GetLongArrayRegion,
    SJVM_GetFloatArrayRegion,
    SJVM_GetDoubleArrayRegion,
    SJVM_SetBooleanArrayRegion,
    SJVM_SetByteArrayRegion,
    SJVM_SetCharArrayRegion,
    SJVM_SetShortArrayRegion,
    SJVM_SetIntArrayRegion,
    SJVM_SetLongArrayRegion,
    SJVM_SetFloatArrayRegion,
    SJVM_SetDoubleArrayRegion,
    SJVM_RegisterNatives,
    SJVM_UnregisterNatives,
    SJVM_MonitorEnter,
    SJVM_MonitorExit,
    SJVM_GetJavaVM,
};

struct JNIInvokeInterface_ SJVM_JNIInvokeInterface = {
    NULL,
    NULL,
    NULL,
    SJVM_DestroyJavaVM,
    SJVM_AttachCurrentThread,
    SJVM_DetachCurrentThread,
    SJVM_GetEnv,
};
