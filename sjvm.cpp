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

#include "jni.h"
#include "GarbageCollector.h"
#include "Arrays.h"
#include "ClassFile.h"
#include "ClassLoader.h"
#include "CPInfo.h"
#include "SJVM.h"
#include "MethodInfo.h"
#include "FieldInfo.h"
#include "Exception.h"
#include "Native.h"
#include "jni.h"

#include <iostream>
#include <map>
#include <string>
#include <sstream>
#include <fstream>
#include <cstring>

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

SJVM::SJVM(ClassPath *path) {
    gc = new GlobalGC;
    loader = new ClassLoader(path);
    vm = new JavaVM;
    JNIInvokeInterface_ *functions = new JNIInvokeInterface_;
    memcpy(functions, &SJVM_JNIInvokeInterface, sizeof (JNIInvokeInterface_));
    functions->reserved0 = this;
    vm->functions = functions;
}

SJVM::~SJVM() {
    delete gc;
    delete loader;
    delete vm->functions;
    delete vm;
    debug("SJVM Destroyed");
}

extern "C" void JNICALL Java_sjvm_SJVM_nativePrint(JNIEnv* env, jclass type, jstring string) {
    const char* bytes = env->GetStringUTFChars(string, 0);
    std::cout << bytes << '\n';
    env->ReleaseStringUTFChars(string, bytes);
}

ClassFile* SJVM::loadClass(const char* name) {
    return loader->load(name, this);
}

JavaVM* SJVM::getJavaVM() {
    return vm;
}

#include <math.h>
#include "OPCodes.h"
#include "Thread.h"
#include "Native.h"
#include "GarbageCollector.h"
#define U2(var,data,index) var = data[index++] << 8; var |= data[index++];
#define U4(var,data,index) var = data[index++] << 24; var |= data[index++] << 16; var |= data[index++] << 8; var |= data[index++];

/**
 * args are treated as a 32bit array, longs happen to be 32 bits
 * exception sets the value to zero if no exception, otherwise a pointer to
 * a valid exception object (must be freed by caller) 
 */
Variable SJVM::execute(MethodInfo *meth, ClassFile *cur, Thread* thread, long* args, long argsc, Exception **thrown) {
    debug("Invoked " << meth->full << " from " << cur->name << '\n');
    *thrown = 0;
    LocalGC localgc(gc, thread);
    if (meth->native || meth->accessFlags & 0x0100) {
        if (meth->accessFlags & 0x0008) {
            argsc += 2;
            long *newargs = new long[argsc];
            newargs[0] = (long) thread->getEnv();
            newargs[1] = (long) cur;
            for (int i = 2; i < argsc; i++)
                newargs[i] = args[i - 2];
            args = newargs;
        } else {
            argsc++;
            long *newargs = new long[argsc];
            newargs[0] = (long) thread->getEnv();
            for (int i = 1; i < argsc; i++)
                newargs[i] = args[i - 1];
            args = newargs;
        }
        if (meth->native == 0) {
            debug("Locating Native Method\n");
            std::stringstream fullname;
            fullname << "Java_";
            std::string classname(cur->name);
            for (int i = 0; (i = classname.find('_', i)) != std::string::npos; i += 2)
                classname.replace(i, 1, "_1", 0, 2);
            for (int i = 0; (i = classname.find('/', i)) != std::string::npos; i += 1)
                classname.replace(i, 1, "_", 0, 1);
            fullname << classname;
            std::string methodname(meth->name);
            for (int i = 0; (i = methodname.find('_', i)) != std::string::npos; i += 2)
                methodname.replace(i, 1, "_1", 0, 2);
            for (int i = 0; (i = methodname.find('/', i)) != std::string::npos; i += 1)
                methodname.replace(i, 1, "_", 0, 1);
            fullname << "_";
            fullname << methodname;
            std::stringstream search;
            search << fullname.str();
            //search << '@' << argsc * 4;
            debug("Searching for native... " << search.str() << '\n');
            if (!(meth->native = libpool.address(search.str().c_str()))) {
                std::string descriptor;
                descriptor.append(meth->descriptor, 1, meth->descriptor.find(')') - 1);
                for (int i = 0; (i = descriptor.find('_', i)) != std::string::npos; i += 2)
                    descriptor.replace(i, 1, "_1", 0, 2);
                for (int i = 0; (i = descriptor.find(';', i)) != std::string::npos; i += 2)
                    descriptor.replace(i, 1, "_2", 0, 2);
                for (int i = 0; (i = descriptor.find('[', i)) != std::string::npos; i += 2)
                    descriptor.replace(i, 1, "_3", 0, 2);
                for (int i = 0; (i = descriptor.find('/', i)) != std::string::npos; i += 1)
                    descriptor.replace(i, 1, "_", 0, 1);
                search.seekp(0);
                fullname << "__";
                fullname << descriptor;
                search << fullname.str();
                //search << '@' << argsc * 4;
                debug("Searching for native... " << search.str() << '\n');
                if (!(meth->native = libpool.address(search.str().c_str()))) {
                    warn("Failed to locate native method: " << search.str() << '\n');
                    Variable result;
                    result.j = 0;
                    /*JNIEnv* env = thread->getEnv();
                    jclass clazz = env->FindClass("java/lang/UnsatisfiedLinkError");
                    if (thread->nativeException) return result;
                    jmethodID id = env->GetMethodID(clazz, "<init>", "()V");
                    JOBJECT obj = (JOBJECT) env->NewObject(clazz, id);
                     *thrown = new Exception(cur, meth, obj);*/
                    delete [] args;
                    return result;
                }
            }
        }
        debug("Native address: " << (unsigned int) meth->native << '\n');
        thread->nativeException = 0;
        long resl, resh;
        asm ("1:\n"
                    "dec %%ecx\n"
                    "push (%%edx,%%ecx,4)\n"
                    "jnz 1b\n"
                    "call *%%eax"
                    : "=a" (resl), "=d" (resh)
                    : "c" (argsc), "d" (args), "a" (meth->native)
                    : "memory");
        delete [] args;
        *thrown = thread->nativeException;
        if (meth->resobject) thread->nativeLocal->removeLocal((JOBJECT) resl);
        Variable result;
        if (meth->result == 2) {
            result.j = ((long long) resh << 32) | (resl);
        } else if (meth->result == 1) {
            result.i = resl;
        }
        return result;
    } else {
        CPInfo **cp = cur->cp;
        long locals[meth->code->maxLocals];
        for (int i = 0; i < argsc; i++)
            locals[i] = args[i];
        long stackStart[meth->code->maxStack];
        long *stack = stackStart - 1;
        unsigned char *bc = meth->code->code;
        unsigned int pc = 0;
        Exception *exception = 0;
        for (;;) {
            debug("OP: " << (unsigned int) bc[pc] << '\n');
            switch (bc[pc++]) {
                case aaload:
                {
                    int pos = *(stack--);
                    JOBJECTARRAY array = (JOBJECTARRAY) * stack;
                    if (!array) {
                        JNIEnv* env = thread->getEnv();
                        debug("aaload null\n");
                        jclass clazz = env->FindClass("java/lang/NullPointerException");
                        if (thread->nativeException) goto EXCEPTION_HANDLER;
                        jmethodID id = env->GetMethodID(clazz, "<init>", "()V");
                        JOBJECT obj = (JOBJECT) env->NewObject(clazz, id);
                        exception = new Exception(cur, meth, obj);
                        gc->incrPersist(exception->cause);
                        goto EXCEPTION_HANDLER;
                    }
                    if (pos < 0 || pos >= array->size) {
                        JNIEnv* env = thread->getEnv();
                        jclass clazz = env->FindClass("java/lang/ArrayIndexOutOfBoundsException");
                        if (thread->nativeException) goto EXCEPTION_HANDLER;
                        jmethodID id = env->GetMethodID(clazz, "<init>", "(I)V");
                        JOBJECT obj = (JOBJECT) env->NewObject(clazz, id, pos);
                        exception = new Exception(cur, meth, obj);
                        gc->incrPersist(exception->cause);
                        goto EXCEPTION_HANDLER;
                    }
                    *(JOBJECT*) stack = array->array[pos];
                    localgc.addLocal(*(JOBJECT*) stack);
                    break;
                }
                case aastore:
                {
                    JOBJECT obj = *(JOBJECT*) (stack--);
                    int pos = *(stack--);
                    JOBJECTARRAY array = (JOBJECTARRAY) *(stack--);
                    if (!array) {
                        JNIEnv* env = thread->getEnv();
                        debug("aastore null\n");
                        jclass clazz = env->FindClass("java/lang/NullPointerException");
                        if (thread->nativeException) goto EXCEPTION_HANDLER;
                        jmethodID id = env->GetMethodID(clazz, "<init>", "()V");
                        JOBJECT obj = (JOBJECT) env->NewObject(clazz, id);
                        exception = new Exception(cur, meth, obj);
                        gc->incrPersist(exception->cause);
                        goto EXCEPTION_HANDLER;
                    }
                    if (pos < 0 || pos >= array->size) {
                        JNIEnv* env = thread->getEnv();
                        jclass clazz = env->FindClass("java/lang/ArrayIndexOutOfBoundsException");
                        if (thread->nativeException) goto EXCEPTION_HANDLER;
                        jmethodID id = env->GetMethodID(clazz, "<init>", "(I)V");
                        JOBJECT obj = (JOBJECT) env->NewObject(clazz, id, pos);
                        exception = new Exception(cur, meth, obj);
                        gc->incrPersist(exception->cause);
                        goto EXCEPTION_HANDLER;
                    }
                    JOBJECT old = array->array[pos];
                    if (old) gc->decrRef(old);
                    array->array[pos] = obj;
                    gc->incrRef(obj);
                    break;
                }
                case aconst_null:
                    stack++;
                    *stack = 0;
                    break;
                case aload:
                    stack++;
                    *stack = locals[bc[pc++]];
                    break;
                case aload_0:
                    stack++;
                    *stack = locals[0];
                    break;
                case aload_1:
                    stack++;
                    *stack = locals[1];
                    break;
                case aload_2:
                    stack++;
                    *stack = locals[2];
                    break;
                case aload_3:
                    stack++;
                    *stack = locals[3];
                    break;
                case anewarray:
                {
                    int size = *(stack--);
                    unsigned short index;
                    U2(index, bc, pc);
                    ClassInfo *cinfo = (ClassInfo*) cp[index];
                    ClassFile* arrayType;
                    if (cinfo->classRef == 0) {
                        std::string name(((UTF8Info*) cp[cinfo->nameIndex])->bytes);
                        arrayType = loader->load((char*) name.c_str(), this);
                        if (thread->nativeException) goto EXCEPTION_HANDLER;
                        cinfo->classRef = arrayType;
                        cinfo->name = name;
                    } else {
                        arrayType = cinfo->classRef;
                    }
                    std::string name("[");
                    name.append(cinfo->name);
                    JOBJECTARRAY array = ClassFile::newArray<JOBJECT>(this,(char*)name.c_str(),size);
                    *(JOBJECT*) (++stack) = array;
                    localgc.addLocal(*(JOBJECT*) stack);
                    debug("anewarray\n");
                    break;
                }
                case areturn:
                {
                    Variable result;
                    result.l = *(JOBJECT*) stack;
                    localgc.removeLocal(result.l);
                    return result;
                }
                case arraylength:
                    if (!(*(JOBJECTARRAY*) stack)) {
                        JNIEnv* env = thread->getEnv();
                        debug("arraylength null\n");
                        jclass clazz = env->FindClass("java/lang/NullPointerException");
                        if (thread->nativeException) goto EXCEPTION_HANDLER;
                        jmethodID id = env->GetMethodID(clazz, "<init>", "()V");
                        JOBJECT obj = (JOBJECT) env->NewObject(clazz, id);
                        exception = new Exception(cur, meth, obj);
                        gc->incrPersist(exception->cause);
                        goto EXCEPTION_HANDLER;
                    }
                    *stack = ((JOBJECTARRAY) * stack)->size; //type irrelevant
                    break;
                case astore:
                    locals[bc[pc++]] = *stack;
                    stack--;
                    break;
                case astore_0:
                    locals[0] = *stack;
                    stack--;
                    break;
                case astore_1:
                    locals[1] = *stack;
                    stack--;
                    break;
                case astore_2:
                    locals[2] = *stack;
                    stack--;
                    break;
                case astore_3:
                    locals[3] = *stack;
                    stack--;
                    break;
                case baload:
                {
                    int pos = *(stack--);
                    JBYTEARRAY array = (JBYTEARRAY) * stack;
                    if (!array) {
                        JNIEnv* env = thread->getEnv();
                        debug("baload null\n");
                        jclass clazz = env->FindClass("java/lang/NullPointerException");
                        if (thread->nativeException) goto EXCEPTION_HANDLER;
                        jmethodID id = env->GetMethodID(clazz, "<init>", "()V");
                        JOBJECT obj = (JOBJECT) env->NewObject(clazz, id);
                        exception = new Exception(cur, meth, obj);
                        gc->incrPersist(exception->cause);
                        goto EXCEPTION_HANDLER;
                    }
                    if (pos < 0 || pos >= array->size) {
                        JNIEnv* env = thread->getEnv();
                        jclass clazz = env->FindClass("java/lang/ArrayIndexOutOfBoundsException");
                        if (thread->nativeException) goto EXCEPTION_HANDLER;
                        jmethodID id = env->GetMethodID(clazz, "<init>", "(I)V");
                        JOBJECT obj = (JOBJECT) env->NewObject(clazz, id, pos);
                        exception = new Exception(cur, meth, obj);
                        gc->incrPersist(exception->cause);
                        goto EXCEPTION_HANDLER;
                    }
                    *stack = array->array[pos];
                    break;
                }
                case bastore:
                {
                    JBYTE b = *(stack--);
                    int pos = *(stack--);
                    JBYTEARRAY array = (JBYTEARRAY) *(stack--);
                    if (!array) {
                        JNIEnv* env = thread->getEnv();
                        debug("bastore null\n");
                        jclass clazz = env->FindClass("java/lang/NullPointerException");
                        if (thread->nativeException) goto EXCEPTION_HANDLER;
                        jmethodID id = env->GetMethodID(clazz, "<init>", "()V");
                        JOBJECT obj = (JOBJECT) env->NewObject(clazz, id);
                        exception = new Exception(cur, meth, obj);
                        gc->incrPersist(exception->cause);
                        goto EXCEPTION_HANDLER;
                    }
                    if (pos < 0 || pos >= array->size) {
                        JNIEnv* env = thread->getEnv();
                        jclass clazz = env->FindClass("java/lang/ArrayIndexOutOfBoundsException");
                        if (thread->nativeException) goto EXCEPTION_HANDLER;
                        jmethodID id = env->GetMethodID(clazz, "<init>", "(I)V");
                        JOBJECT obj = (JOBJECT) env->NewObject(clazz, id, pos);
                        exception = new Exception(cur, meth, obj);
                        gc->incrPersist(exception->cause);
                        goto EXCEPTION_HANDLER;
                    }
                    array->array[pos] = b;
                    break;
                }
                case bipush:
                    *(++stack) = bc[pc++];
                    break;
                case caload:
                {
                    int pos = *(unsigned int*) (stack--);
                    JCHARARRAY array = (JCHARARRAY) * stack;
                    if (!array) {
                        JNIEnv* env = thread->getEnv();
                        debug("caload null\n");
                        jclass clazz = env->FindClass("java/lang/NullPointerException");
                        if (thread->nativeException) goto EXCEPTION_HANDLER;
                        jmethodID id = env->GetMethodID(clazz, "<init>", "()V");
                        JOBJECT obj = (JOBJECT) env->NewObject(clazz, id);
                        exception = new Exception(cur, meth, obj);
                        gc->incrPersist(exception->cause);
                        goto EXCEPTION_HANDLER;
                    }
                    if (pos < 0 || pos >= array->size) {
                        JNIEnv* env = thread->getEnv();
                        jclass clazz = env->FindClass("java/lang/ArrayIndexOutOfBoundsException");
                        if (thread->nativeException) goto EXCEPTION_HANDLER;
                        jmethodID id = env->GetMethodID(clazz, "<init>", "(I)V");
                        JOBJECT obj = (JOBJECT) env->NewObject(clazz, id, pos);
                        exception = new Exception(cur, meth, obj);
                        gc->incrPersist(exception->cause);
                        goto EXCEPTION_HANDLER;
                    }
                    *stack = array->array[pos];
                    break;
                }
                case castore:
                {
                    JCHAR c = *(unsigned int*) (stack--);
                    int pos = *(stack--);
                    JCHARARRAY array = (JCHARARRAY) *(stack--);
                    if (!array) {
                        JNIEnv* env = thread->getEnv();
                        debug("castore null\n");
                        jclass clazz = env->FindClass("java/lang/NullPointerException");
                        if (thread->nativeException) goto EXCEPTION_HANDLER;
                        jmethodID id = env->GetMethodID(clazz, "<init>", "()V");
                        JOBJECT obj = (JOBJECT) env->NewObject(clazz, id);
                        exception = new Exception(cur, meth, obj);
                        gc->incrPersist(exception->cause);
                        goto EXCEPTION_HANDLER;
                    }
                    if (pos < 0 || pos >= array->size) {
                        JNIEnv* env = thread->getEnv();
                        jclass clazz = env->FindClass("java/lang/ArrayIndexOutOfBoundsException");
                        if (thread->nativeException) goto EXCEPTION_HANDLER;
                        jmethodID id = env->GetMethodID(clazz, "<init>", "(I)V");
                        JOBJECT obj = (JOBJECT) env->NewObject(clazz, id, pos);
                        exception = new Exception(cur, meth, obj);
                        gc->incrPersist(exception->cause);
                        goto EXCEPTION_HANDLER;
                    }
                    array->array[pos] = c;
                    break;
                }
                case checkcast:
                {
                    unsigned short index;
                    U2(index, bc, pc);
                    ClassInfo *cinfo = (ClassInfo*) cp[index];
                    ClassFile *type;
                    if (cinfo->classRef == 0) {
                        std::string name(((UTF8Info*) cp[cinfo->nameIndex])->bytes);
                        type = loader->load((char*) name.c_str(), this);
                        if (thread->nativeException) goto EXCEPTION_HANDLER;
                        cinfo->classRef = type;
                        cinfo->name = name;
                    } else {
                        type = cinfo->classRef;
                    }
                    if (!(*(JOBJECT*) stack)) break;
                    if (!type->instanceOf(*(JOBJECT*) stack)) {
                        JNIEnv* env = thread->getEnv();
                        jclass clazz = env->FindClass("java/lang/ClassCastException");
                        if (thread->nativeException) goto EXCEPTION_HANDLER;
                        jmethodID id = env->GetMethodID(clazz, "<init>", "()V");
                        JOBJECT obj = (JOBJECT) env->NewObject(clazz, id);
                        exception = new Exception(cur, meth, obj);
                        gc->incrPersist(exception->cause);
                        goto EXCEPTION_HANDLER;
                    }
                    break;
                }
                case d2f:
                    *(JFLOAT*) (--stack) = (JFLOAT) *((JDOUBLE*) (stack - 1));
                    break;
                case d2i:
                    *(JINT*) (--stack) = (JINT) *((JDOUBLE*) (stack - 1));
                    break;
                case d2l:
                    *(JLONG*) (stack - 1) = (JLONG) *((JDOUBLE*) (stack - 1));
                    break;
                case dadd:
                {
                    double a = *((JDOUBLE*) (stack - 1));
                    stack -= 2;
                    *(JDOUBLE*) (stack - 1) = *(JDOUBLE*) (stack - 1) + a;
                    debug("dadd: " << *(JDOUBLE*) (stack - 1) << '\n');
                    break;
                }
                case daload:
                {
                    int pos = *(stack--);
                    JDOUBLEARRAY array = (JDOUBLEARRAY) * stack;
                    if (!array) {
                        JNIEnv* env = thread->getEnv();
                        debug("daload null\n");
                        jclass clazz = env->FindClass("java/lang/NullPointerException");
                        if (thread->nativeException) goto EXCEPTION_HANDLER;
                        jmethodID id = env->GetMethodID(clazz, "<init>", "()V");
                        JOBJECT obj = (JOBJECT) env->NewObject(clazz, id);
                        exception = new Exception(cur, meth, obj);
                        gc->incrPersist(exception->cause);
                        goto EXCEPTION_HANDLER;
                    }
                    if (pos < 0 || pos >= array->size) {
                        JNIEnv* env = thread->getEnv();
                        jclass clazz = env->FindClass("java/lang/ArrayIndexOutOfBoundsException");
                        if (thread->nativeException) goto EXCEPTION_HANDLER;
                        jmethodID id = env->GetMethodID(clazz, "<init>", "(I)V");
                        JOBJECT obj = (JOBJECT) env->NewObject(clazz, id, pos);
                        exception = new Exception(cur, meth, obj);
                        gc->incrPersist(exception->cause);
                        goto EXCEPTION_HANDLER;
                    }
                    *(JDOUBLE*) (stack++) = array->array[pos];
                    break;
                }
                case dastore:
                {
                    JDOUBLE d = *((JDOUBLE*) (stack - 1));
                    stack -= 2;
                    int pos = *(stack--);
                    JDOUBLEARRAY array = (JDOUBLEARRAY) *(stack--);
                    if (!array) {
                        JNIEnv* env = thread->getEnv();
                        debug("dastore null\n");
                        jclass clazz = env->FindClass("java/lang/NullPointerException");
                        if (thread->nativeException) goto EXCEPTION_HANDLER;
                        jmethodID id = env->GetMethodID(clazz, "<init>", "()V");
                        JOBJECT obj = (JOBJECT) env->NewObject(clazz, id);
                        exception = new Exception(cur, meth, obj);
                        gc->incrPersist(exception->cause);
                        goto EXCEPTION_HANDLER;
                    }
                    if (pos < 0 || pos >= array->size) {
                        JNIEnv* env = thread->getEnv();
                        jclass clazz = env->FindClass("java/lang/ArrayIndexOutOfBoundsException");
                        if (thread->nativeException) goto EXCEPTION_HANDLER;
                        jmethodID id = env->GetMethodID(clazz, "<init>", "(I)V");
                        JOBJECT obj = (JOBJECT) env->NewObject(clazz, id, pos);
                        exception = new Exception(cur, meth, obj);
                        gc->incrPersist(exception->cause);
                        goto EXCEPTION_HANDLER;
                    }
                    array->array[pos] = d;
                    break;
                }
                case dcmpl:
                case dcmpg:
                {
                    JDOUBLE a = *((JDOUBLE*) (stack - 1));
                    stack -= 2;
                    JDOUBLE b = *((JDOUBLE*) (stack - 1));
                    stack -= 2;
                    if (b > a) *(++stack) = 1;
                    if (b < a) *(++stack) = -1;
                    if (b == a) *(++stack) = 0;
                    debug("dcmp[l/g]: " << a << " " << b << " " << *stack << '\n');
                    break;
                }
                case dconst_0:
                    *(JDOUBLE*) (++stack) = (JDOUBLE) 0;
                    stack++;
                    break;
                case dconst_1:
                    *(JDOUBLE*) (++stack) = (JDOUBLE) 1;
                    stack++;
                    break;
                case ddiv:
                {
                    double a = *((JDOUBLE*) (stack - 1));
                    stack -= 2;
                    *(JDOUBLE*) (stack - 1) = *(JDOUBLE*) (stack - 1) / a;
                    break;
                }
                case dload:
                    *(++stack) = locals[bc[pc]];
                    *(++stack) = locals[bc[pc++]];
                    break;
                case dload_0:
                    *(++stack) = locals[0];
                    *(++stack) = locals[1];
                    break;
                case dload_1:
                    *(++stack) = locals[1];
                    *(++stack) = locals[2];
                    break;
                case dload_2:
                    *(++stack) = locals[2];
                    *(++stack) = locals[3];
                    break;
                case dload_3:
                    *(++stack) = locals[3];
                    *(++stack) = locals[4];
                    break;
                case dmul:
                {
                    double a = *((JDOUBLE*) (stack - 1));
                    stack -= 2;
                    *(JDOUBLE*) (stack - 1) = *(JDOUBLE*) (stack - 1) * a;
                    break;
                }
                case dneg:
                    *(JDOUBLE*) (stack - 1) = -(*(JDOUBLE*) (stack - 1));
                    break;
                case drem:
                {
                    double a = *((JDOUBLE*) (stack - 1));
                    stack -= 2;
                    *(JDOUBLE*) (stack - 1) = fmod(*(JDOUBLE*) (stack - 1), a);
                    break;
                }
                case dreturn:
                {
                    Variable result;
                    result.d = *(JDOUBLE*) (stack - 1);
                    return result;
                }
                case dstore:
                    locals[bc[pc]] = *(stack - 1);
                    locals[bc[pc++]] = *(stack);
                    stack -= 2;
                    break;
                case dstore_0:
                    locals[0] = *(stack - 1);
                    locals[1] = *(stack);
                    stack -= 2;
                    break;
                case dstore_1:
                    locals[1] = *(stack - 1);
                    locals[2] = *(stack);
                    stack -= 2;
                    break;
                    ;
                case dstore_2:
                    locals[2] = *(stack - 1);
                    locals[3] = *(stack);
                    stack -= 2;
                    break;
                case dstore_3:
                    locals[3] = *(stack - 1);
                    locals[4] = *(stack);
                    stack -= 2;
                    break;
                case dsub:
                {
                    double a = *((JDOUBLE*) (stack - 1));
                    stack -= 2;
                    *(JDOUBLE*) (stack - 1) = *(JDOUBLE*) (stack - 1) - a;
                    break;
                }
                case dup:
                {
                    int temp = *stack;
                    *(++stack) = temp;
                    break;
                }
                case dup_x1:
                {
                    int temp = *stack;
                    int last = *(--stack);
                    *stack = temp;
                    *(++stack) = last;
                    *(++stack) = temp;
                    break;
                }
                case dup_x2:
                {
                    int temp = *stack;
                    int last1 = *(--stack);
                    int last2 = *(--stack);
                    *stack = temp;
                    *(++stack) = last2;
                    *(++stack) = last1;
                    *(++stack) = temp;
                    break;
                }
                case dup2: //assumes being used on cat2
                {
                    int h = *(stack - 1);
                    int l = *(stack);
                    *(++stack) = h;
                    *(++stack) = l;
                    break;
                }
                case dup2_x1: //assumes being used on cat2
                {
                    int h = *(stack - 1);
                    int l = *(stack);
                    int last = *(stack - 2);
                    stack -= 3;
                    *(++stack) = h;
                    *(++stack) = l;
                    *(++stack) = last;
                    *(++stack) = h;
                    *(++stack) = l;
                    break;
                    break;
                }
                case dup2_x2: //assumes being used on cat2
                {
                    int h = *(stack - 1);
                    int l = *(stack);
                    int lasth = *(stack - 2);
                    int lastl = *(stack - 3);
                    stack -= 4;
                    *(++stack) = h;
                    *(++stack) = l;
                    *(++stack) = lastl;
                    *(++stack) = lasth;
                    *(++stack) = h;
                    *(++stack) = l;
                    break;
                }
                case f2d:
                    *(JDOUBLE*) stack = (JDOUBLE) *(JFLOAT*) stack;
                    stack++;
                    break;
                case f2i:
                    *(JINT*) stack = (JINT) *(JFLOAT*) stack;
                    break;
                case f2l:
                    *(JLONG*) stack = (JLONG) *(JFLOAT*) stack;
                    stack++;
                    break;
                case fadd:
                {
                    JFLOAT a = *(JFLOAT*) (stack--);
                    debug("fadd: " << a << '+');
                    debug(*(JFLOAT*) stack << '=');
                    *(JFLOAT*) stack = *(JFLOAT*) stack + a;
                    debug(*(JFLOAT*) stack << '\n');
                    break;
                }
                case faload:
                {
                    int pos = *(stack--);
                    JFLOATARRAY array = (JFLOATARRAY) * stack;
                    if (!array) {
                        JNIEnv* env = thread->getEnv();
                        debug("faload null\n");
                        jclass clazz = env->FindClass("java/lang/NullPointerException");
                        if (thread->nativeException) goto EXCEPTION_HANDLER;
                        jmethodID id = env->GetMethodID(clazz, "<init>", "()V");
                        JOBJECT obj = (JOBJECT) env->NewObject(clazz, id);
                        exception = new Exception(cur, meth, obj);
                        gc->incrPersist(exception->cause);
                        goto EXCEPTION_HANDLER;
                    }
                    if (pos < 0 || pos >= array->size) {
                        JNIEnv* env = thread->getEnv();
                        jclass clazz = env->FindClass("java/lang/ArrayIndexOutOfBoundsException");
                        if (thread->nativeException) goto EXCEPTION_HANDLER;
                        jmethodID id = env->GetMethodID(clazz, "<init>", "(I)V");
                        JOBJECT obj = (JOBJECT) env->NewObject(clazz, id, pos);
                        exception = new Exception(cur, meth, obj);
                        gc->incrPersist(exception->cause);
                        goto EXCEPTION_HANDLER;
                    }
                    *(JFLOAT*) stack = array->array[pos];
                    break;
                }
                case fastore:
                {
                    JFLOAT f = *(stack--);
                    int pos = *(stack--);
                    JFLOATARRAY array = (JFLOATARRAY) *(stack--);
                    if (!array) {
                        JNIEnv* env = thread->getEnv();
                        debug("fastore null\n");
                        jclass clazz = env->FindClass("java/lang/NullPointerException");
                        if (thread->nativeException) goto EXCEPTION_HANDLER;
                        jmethodID id = env->GetMethodID(clazz, "<init>", "()V");
                        JOBJECT obj = (JOBJECT) env->NewObject(clazz, id);
                        exception = new Exception(cur, meth, obj);
                        gc->incrPersist(exception->cause);
                        goto EXCEPTION_HANDLER;
                    }
                    if (pos < 0 || pos >= array->size) {
                        JNIEnv* env = thread->getEnv();
                        jclass clazz = env->FindClass("java/lang/ArrayIndexOutOfBoundsException");
                        if (thread->nativeException) goto EXCEPTION_HANDLER;
                        jmethodID id = env->GetMethodID(clazz, "<init>", "(I)V");
                        JOBJECT obj = (JOBJECT) env->NewObject(clazz, id, pos);
                        exception = new Exception(cur, meth, obj);
                        gc->incrPersist(exception->cause);
                        goto EXCEPTION_HANDLER;
                    }
                    array->array[pos] = f;
                    break;
                }
                case fcmpl:
                case fcmpg:
                {
                    JFLOAT a = *(JFLOAT*) (stack--);
                    JFLOAT b = *(JFLOAT*) (stack);
                    if (b > a) *(stack) = 1;
                    if (b < a) *(stack) = -1;
                    if (b == a) *(stack) = 0;
                    debug("fcmp[l/g]: " << a << " " << b << " " << *stack << '\n');
                    break;
                }
                case fconst_0:
                    *(++stack) = 0;
                    break;
                case fconst_1:
                    *(++stack) = 1;
                    break;
                case fconst_2:
                    *(++stack) = 2;
                    break;
                case fdiv:
                {
                    JFLOAT a = *(JFLOAT*) (stack--);
                    *(JFLOAT*) stack = *(JFLOAT*) stack / a;
                    break;
                }
                case fload:
                    *(++stack) = locals[bc[pc++]];
                    debug("fload: " << *(JFLOAT*) stack << '\n');
                    break;
                case fload_0:
                    *(++stack) = locals[0];
                    break;
                case fload_1:
                    *(++stack) = locals[1];
                    break;
                case fload_2:
                    *(++stack) = locals[2];
                    break;
                case fload_3:
                    *(++stack) = locals[3];
                    break;
                case fmul:
                {
                    float a = *(JFLOAT*) (stack--);
                    *(JFLOAT*) stack = *(JFLOAT*) stack * a;
                    break;
                }
                case fneg:
                    *stack = -(*stack);
                    break;
                case frem:
                {
                    JFLOAT a = *(stack--);
                    *(JFLOAT*) stack = fmod(*(JFLOAT*) stack, a);
                    break;
                }
                case freturn:
                {
                    Variable result;
                    result.f = *(JFLOAT*) stack;
                    return result;
                }
                case fstore:
                    locals[bc[pc++]] = *stack;
                    debug("fstore: " << *(JFLOAT*) stack << '\n');
                    stack--;
                    break;
                case fstore_0:
                    locals[0] = *stack;
                    stack--;
                    break;
                case fstore_1:
                    locals[1] = *stack;
                    stack--;
                    break;
                case fstore_2:
                    locals[2] = *stack;
                    stack--;
                    break;
                case fstore_3:
                    locals[3] = *stack;
                    stack--;
                    break;
                case fsub:
                {
                    JFLOAT a = *(JFLOAT*) (stack--);
                    *(JFLOAT*) stack = *(JFLOAT*) stack - a;
                    break;
                }
                case getfield:
                {
                    unsigned short int index;
                    U2(index, bc, pc);
                    FieldMethodInfo *info = (FieldMethodInfo*) cp[index];
                    ClassInfo *cinfo = (ClassInfo*) cp[info->classIndex];
                    JCLASS type;
                    if (cinfo->classRef == 0) {
                        std::string name(((UTF8Info*) cp[cinfo->nameIndex])->bytes);
                        type = loader->load((char*) name.c_str(), this);
                        if (thread->nativeException) goto EXCEPTION_HANDLER;
                        cinfo->classRef = type;
                        cinfo->name = name;
                    } else {
                        type = cinfo->classRef;
                    }
                    FieldInfo *field;
                    if (info->fieldRef == 0) {
                        if (info->name.size() < 1)
                            info->name.append(((UTF8Info*) cp[((NameAndTypeInfo*) cp[info->nameAndTypeIndex])->nameIndex])->bytes);
                        field = 0;
                        for (JCLASS find = type; find != 0 && field == 0; find = find->superClass)
                            if (find->fieldMap.find(info->name) != find->fieldMap.end())
                                field = find->fieldMap[info->name];
                        info->fieldRef = field;
                    } else {
                        field = info->fieldRef;
                    }
                    debug("getfield: " << field->descriptor << " " << field->name << '\n');
                    if (!(*(JOBJECT*) stack)) {
                        JNIEnv* env = thread->getEnv();
                        debug("getfield null\n");
                        jclass clazz = env->FindClass("java/lang/NullPointerException");
                        if (thread->nativeException) goto EXCEPTION_HANDLER;
                        jmethodID id = env->GetMethodID(clazz, "<init>", "()V");
                        JOBJECT obj = (JOBJECT) env->NewObject(clazz, id);
                        exception = new Exception(cur, meth, obj);
                        gc->incrPersist(exception->cause);
                        goto EXCEPTION_HANDLER;
                    }
                    switch (field->descriptor.c_str()[0]) {
                        case 'D':
                        case 'J':
                            *(JLONG*) stack = (*(JOBJECT*) stack)->fields[field->index].j;
                            stack++;
                            break;
                        case 'L':
                        case '[':
                            *stack = (*(JOBJECT*) stack)->fields[field->index].i;
                            localgc.addLocal(*(JOBJECT*) stack);
                            break;
                        default:
                            *stack = (*(JOBJECT*) stack)->fields[field->index].i;
                    }
                    break;
                }
                case getstatic:
                {
                    unsigned short int index;
                    U2(index, bc, pc);
                    FieldMethodInfo *info = (FieldMethodInfo*) cp[index];
                    FieldInfo *field;
                    if (info->fieldRef == 0) {
                        ClassInfo *cinfo = (ClassInfo*) cp[info->classIndex];
                        JCLASS type;
                        if (cinfo->classRef == 0) {
                            std::string name(((UTF8Info*) cp[cinfo->nameIndex])->bytes);
                            type = loader->load((char*) name.c_str(), this);
                            if (thread->nativeException) goto EXCEPTION_HANDLER;
                            cinfo->classRef = type;
                            cinfo->name = name;
                        } else {
                            type = cinfo->classRef;
                        }
                        if (info->name.size() < 1)
                            info->name.append(((UTF8Info*) cp[((NameAndTypeInfo*) cp[info->nameAndTypeIndex])->nameIndex])->bytes);
                        field = 0;
                        for (JCLASS find = type; find != 0 && field == 0; find = find->superClass)
                            if (find->fieldMap.find(info->name) != find->fieldMap.end())
                                field = find->fieldMap[info->name];
                        info->fieldRef = field;
                    } else {
                        field = info->fieldRef;
                    }
                    debug("getstatic: " << field->descriptor << " " << field->name << '\n');
                    switch (field->descriptor.c_str()[0]) {
                        case 'D':
                        case 'J':
                            *(JLONG*) (stack + 1) = field->staticValue.j;
                            stack += 2;
                            break;
                        case 'L':
                        case '[':
                            *(++stack) = field->staticValue.i;
                            localgc.addLocal(*(JOBJECT*) stack);
                            break;
                        default:
                            *(++stack) = field->staticValue.i;
                    }
                    break;
                }
                case jump:
                {
                    int orig = pc - 1;
                    short int offset;
                    U2(offset, bc, pc);
                    pc = orig + offset; //fight the autoinc
                    break;
                }
                case jump_w:
                {
                    int orig = pc - 1;
                    long int offset;
                    U4(offset, bc, pc);
                    pc = orig + offset; //fight the autoinc
                    break;
                }
                case i2b:
                    *(JBYTE*) stack = (JBYTE) *(JINT*) stack;
                    break;
                case i2c:
                    *(JCHAR*) stack = (JCHAR) *(JINT*) stack;
                    break;
                case i2d:
                    *(JDOUBLE*) stack = (JDOUBLE) *(JINT*) stack;
                    stack++;
                    break;
                case i2f:
                    *(JFLOAT*) stack = (JFLOAT) *(JINT*) stack;
                    break;
                case i2l:
                    *(JLONG*) stack = (JLONG) *(JINT*) stack;
                    stack++;
                    break;
                case i2s:
                    *(JSHORT*) stack = (JSHORT) *(JINT*) stack;
                    break;
                case iadd:
                {
                    JINT a = *(stack--);
                    *stack = *stack + a;
                    debug("iadd: " << *stack << '\n');
                    break;
                }
                case iaload:
                {
                    int pos = *(stack--);
                    JINTARRAY array = (JINTARRAY) * stack;
                    if (!array) {
                        JNIEnv* env = thread->getEnv();
                        debug("iaload null\n");
                        jclass clazz = env->FindClass("java/lang/NullPointerException");
                        if (thread->nativeException) goto EXCEPTION_HANDLER;
                        jmethodID id = env->GetMethodID(clazz, "<init>", "()V");
                        JOBJECT obj = (JOBJECT) env->NewObject(clazz, id);
                        exception = new Exception(cur, meth, obj);
                        gc->incrPersist(exception->cause);
                        goto EXCEPTION_HANDLER;
                    }
                    if (pos < 0 || pos >= array->size) {
                        JNIEnv* env = thread->getEnv();
                        jclass clazz = env->FindClass("java/lang/ArrayIndexOutOfBoundsException");
                        if (thread->nativeException) goto EXCEPTION_HANDLER;
                        jmethodID id = env->GetMethodID(clazz, "<init>", "(I)V");
                        JOBJECT obj = (JOBJECT) env->NewObject(clazz, id, pos);
                        exception = new Exception(cur, meth, obj);
                        gc->incrPersist(exception->cause);
                        goto EXCEPTION_HANDLER;
                    }
                    *stack = array->array[pos];
                    break;
                }
                case iand:
                {
                    JINT a = *(stack--);
                    *stack = *stack & a;
                    break;
                }
                case iastore:
                {
                    JINT i = *(stack--);
                    int pos = *(stack--);
                    JINTARRAY array = (JINTARRAY) *(stack--);
                    if (!array) {
                        JNIEnv* env = thread->getEnv();
                        debug("iastore null\n");
                        jclass clazz = env->FindClass("java/lang/NullPointerException");
                        if (thread->nativeException) goto EXCEPTION_HANDLER;
                        jmethodID id = env->GetMethodID(clazz, "<init>", "()V");
                        JOBJECT obj = (JOBJECT) env->NewObject(clazz, id);
                        exception = new Exception(cur, meth, obj);
                        gc->incrPersist(exception->cause);
                        goto EXCEPTION_HANDLER;
                    }
                    if (pos < 0 || pos >= array->size) {
                        JNIEnv* env = thread->getEnv();
                        jclass clazz = env->FindClass("java/lang/ArrayIndexOutOfBoundsException");
                        if (thread->nativeException) goto EXCEPTION_HANDLER;
                        jmethodID id = env->GetMethodID(clazz, "<init>", "(I)V");
                        JOBJECT obj = (JOBJECT) env->NewObject(clazz, id, pos);
                        exception = new Exception(cur, meth, obj);
                        gc->incrPersist(exception->cause);
                        goto EXCEPTION_HANDLER;
                    }
                    array->array[pos] = i;
                    break;
                }
                case iconst_m1:
                    *(++stack) = -1;
                    break;
                case iconst_0:
                    *(++stack) = 0;
                    break;
                case iconst_1:
                    *(++stack) = 1;
                    break;
                case iconst_2:
                    *(++stack) = 2;
                    break;
                case iconst_3:
                    *(++stack) = 3;
                    break;
                case iconst_4:
                    *(++stack) = 4;
                    break;
                case iconst_5:
                    *(++stack) = 5;
                    break;
                case idiv:
                {
                    JINT a = *(stack--);
                    if (a == 0) {
                        JNIEnv* env = thread->getEnv();
                        jclass clazz = env->FindClass("java/lang/ArithmeticException");
                        if (thread->nativeException) goto EXCEPTION_HANDLER;
                        jmethodID id = env->GetMethodID(clazz, "<init>", "()V");
                        JOBJECT obj = (JOBJECT) env->NewObject(clazz, id);
                        exception = new Exception(cur, meth, obj);
                        gc->incrPersist(exception->cause);
                        goto EXCEPTION_HANDLER;
                    }
                    *stack = *stack / a;
                    break;
                }
                case if_acmpeq:
                {
                    int orig = pc - 1;
                    JOBJECT a = *(JOBJECT*) (stack--);
                    JOBJECT b = *(JOBJECT*) (stack--);
                    short int offset;
                    U2(offset, bc, pc);
                    if (a == b) pc = orig + offset;
                    break;
                }
                case if_acmpne:
                {
                    int orig = pc - 1;
                    JOBJECT a = *(JOBJECT*) (stack--);
                    JOBJECT b = *(JOBJECT*) (stack--);
                    short int offset;
                    U2(offset, bc, pc);
                    if (a != b) pc = orig + offset;
                    break;
                }
                case if_icmpeq:
                {
                    int orig = pc - 1;
                    JINT b = *(stack--);
                    JINT a = *(stack--);
                    short int offset;
                    U2(offset, bc, pc);
                    if (a == b) pc = orig + offset;
                    break;
                }
                case if_icmpne:
                {
                    int orig = pc - 1;
                    JINT b = *(stack--);
                    JINT a = *(stack--);
                    short int offset;
                    U2(offset, bc, pc);
                    if (a != b) pc = orig + offset;
                    break;
                }
                case if_icmplt:
                {
                    int orig = pc - 1;
                    JINT b = *(stack--);
                    JINT a = *(stack--);
                    short int offset;
                    U2(offset, bc, pc);
                    if (a < b) pc = orig + offset;
                    break;
                }
                case if_icmple:
                {
                    int orig = pc - 1;
                    JINT b = *(stack--);
                    JINT a = *(stack--);
                    short int offset;
                    U2(offset, bc, pc);
                    if (a <= b) pc = orig + offset;
                    break;
                }
                case if_icmpgt:
                {
                    int orig = pc - 1;
                    JINT b = *(stack--);
                    JINT a = *(stack--);
                    short int offset;
                    U2(offset, bc, pc);
                    if (a > b) pc = orig + offset;
                    break;
                }
                case if_icmpge:
                {
                    int orig = pc - 1;
                    JINT b = *(stack--);
                    JINT a = *(stack--);
                    short int offset;
                    U2(offset, bc, pc);
                    if (a >= b) pc = orig + offset;
                    break;
                }
                case ifeq:
                {
                    int orig = pc - 1;
                    JINT a = *(stack--);
                    short int offset;
                    U2(offset, bc, pc);
                    if (a == 0) pc = orig + offset;
                    break;
                }
                case ifne:
                {
                    int orig = pc - 1;
                    JINT a = *(stack--);
                    short int offset;
                    U2(offset, bc, pc);
                    if (a != 0) pc = orig + offset;
                    break;
                }
                case iflt:
                {
                    int orig = pc - 1;
                    JINT a = *(stack--);
                    short int offset;
                    U2(offset, bc, pc);
                    if (a < 0) pc = orig + offset;
                    break;
                }
                case ifle:
                {
                    int orig = pc - 1;
                    JINT a = *(stack--);
                    short int offset;
                    U2(offset, bc, pc);
                    if (a <= 0) pc = orig + offset;
                    break;
                }
                case ifgt:
                {
                    int orig = pc - 1;
                    JINT a = *(stack--);
                    short int offset;
                    U2(offset, bc, pc);
                    if (a > 0) pc = orig + offset;
                    break;
                }
                case ifge:
                {
                    int orig = pc - 1;
                    JINT a = *(stack--);
                    short int offset;
                    U2(offset, bc, pc);
                    if (a >= 0) pc = orig + offset;
                    break;
                }
                case ifnonnull:
                {
                    int orig = pc - 1;
                    JOBJECT a = *(JOBJECT*) (stack--);
                    short int offset;
                    U2(offset, bc, pc);
                    if (a != 0) pc = orig + offset;
                    break;
                }
                case ifnull:
                {
                    int orig = pc - 1;
                    JOBJECT a = *(JOBJECT*) (stack--);
                    short int offset;
                    U2(offset, bc, pc);
                    if (a == 0) pc = orig + offset;
                    break;
                }
                case iinc:
                {
                    int i = bc[pc++];
                    JBYTE b = (JBYTE) bc[pc++];
                    locals[i] += b;
                    break;
                }
                case iload:
                    *(++stack) = locals[bc[pc++]];
                    break;
                case iload_0:
                    *(++stack) = locals[0];
                    break;
                case iload_1:
                    *(++stack) = locals[1];
                    break;
                case iload_2:
                    *(++stack) = locals[2];
                    break;
                case iload_3:
                    *(++stack) = locals[3];
                    break;
                case imul:
                {
                    JINT a = *(stack--);
                    *stack = *stack * a;
                    break;
                }
                case ineg:
                    *stack = -(*stack);
                    break;
                case instanceof:
                {
                    unsigned short index;
                    U2(index, bc, pc);
                    ClassInfo *cinfo = (ClassInfo*) cp[index];
                    ClassFile *type;
                    if (cinfo->classRef == 0) {
                        std::string name(((UTF8Info*) cp[cinfo->nameIndex])->bytes);
                        type = loader->load((char*) name.c_str(), this);
                        if (thread->nativeException) goto EXCEPTION_HANDLER;
                        cinfo->classRef = type;
                        cinfo->name = name;
                    } else {
                        type = cinfo->classRef;
                    }
                    if (*stack) {
                        *stack = (int) type->instanceOf(*(JOBJECT*) stack);
                    } else {
                        *stack = 0;
                    }
                    break;
                }
                case invokeinterface:
                {
                    unsigned short int index;
                    U2(index, bc, pc);
                    pc++; //skip the count
                    pc++; //skip the 0
                    FieldMethodInfo *info = (FieldMethodInfo*) cp[index];
                    ClassInfo *cinfo = (ClassInfo*) cp[info->classIndex];
                    JCLASS type;
                    if (cinfo->classRef == 0) {
                        std::string name(((UTF8Info*) cp[cinfo->nameIndex])->bytes);
                        type = loader->load((char*) name.c_str(), this);
                        if (thread->nativeException) goto EXCEPTION_HANDLER;
                        cinfo->classRef = type;
                        cinfo->name = name;
                    } else {
                        type = cinfo->classRef;
                    }
                    MethodInfo *method;
                    if (info->methodRef == 0) {
                        if (info->name.size() < 1) {
                            info->name.append(((UTF8Info*) cp[((NameAndTypeInfo*) cp[info->nameAndTypeIndex])->nameIndex])->bytes);
                            info->name.append(((UTF8Info*) cp[((NameAndTypeInfo*) cp[info->nameAndTypeIndex])->descriptorIndex])->bytes);
                        }
                        method = type->methodMap[info->name]; //uses name+descriptor
                        info->methodRef = method;
                    } else {
                        method = info->methodRef;
                    }
                    long args[method->nargs + 1]; //contains object ref
                    for (int i = 0; i < method->nargs + 1; i++)
                        args[i] = (stack - method->nargs)[i];
                    stack -= method->nargs;
                    if (!(*(JOBJECT*) stack)) {
                        JNIEnv* env = thread->getEnv();
                        debug("invokeinterface null\n");
                        jclass clazz = env->FindClass("java/lang/NullPointerException");
                        if (thread->nativeException) goto EXCEPTION_HANDLER;
                        jmethodID id = env->GetMethodID(clazz, "<init>", "()V");
                        JOBJECT obj = (JOBJECT) env->NewObject(clazz, id);
                        exception = new Exception(cur, meth, obj);
                        gc->incrPersist(exception->cause);
                        goto EXCEPTION_HANDLER;
                    }
                    JOBJECT ref = *(JOBJECT*) (stack--);
                    MethodInfo *invoked;
                    for (type = ref->type; type != 0; type = type->superClass) {
                        if (type->methodMap.find(method->full) != type->methodMap.end()) {
                            invoked = type->methodMap[method->full];
                            break;
                        }
                    }
                    debug("invokeinterface: " << method->full << " in " << type->name << '\n');
                    Variable result = execute(invoked, type, thread, args, method->nargs + 1, &exception);
                    if (exception) goto EXCEPTION_HANDLER;
                    if (method->resobject) localgc.addLocal(result.l);
                    if (method->result == 2) {
                        *(JLONG*) (++stack) = result.j;
                        stack++;
                    } else if (method->result == 1) {
                        *(JINT*) (++stack) = result.i;
                    }
                    break;
                }
                case invokespecial:
                {
                    unsigned short int index;
                    U2(index, bc, pc);
                    FieldMethodInfo *info = (FieldMethodInfo*) cp[index];
                    JCLASS type;
                    ClassInfo *cinfo = (ClassInfo*) cp[info->classIndex];
                    if (cinfo->classRef == 0) {
                        std::string name(((UTF8Info*) cp[cinfo->nameIndex])->bytes);
                        type = loader->load((char*) name.c_str(), this);
                        if (thread->nativeException) goto EXCEPTION_HANDLER;
                        cinfo->classRef = type;
                        cinfo->name = name;
                    } else {
                        type = cinfo->classRef;
                    }
                    MethodInfo *method;
                    if (info->methodRef == 0) {
                        if (info->name.size() < 1) {
                            info->name.append(((UTF8Info*) cp[((NameAndTypeInfo*) cp[info->nameAndTypeIndex])->nameIndex])->bytes);
                            info->name.append(((UTF8Info*) cp[((NameAndTypeInfo*) cp[info->nameAndTypeIndex])->descriptorIndex])->bytes);
                        }
                        method = type->methodMap[info->name]; //uses name+descriptor
                        info->methodRef = method;
                    } else {
                        method = info->methodRef;
                    }
                    long args[method->nargs + 1]; //contains object ref
                    for (int i = 0; i < method->nargs + 1; i++)
                        args[i] = (stack - method->nargs)[i];
                    stack -= method->nargs;
                    if (!(*(JOBJECT*) stack)) {
                        JNIEnv* env = thread->getEnv();
                        debug("invokespecial null\n");
                        jclass clazz = env->FindClass("java/lang/NullPointerException");
                        if (thread->nativeException) goto EXCEPTION_HANDLER;
                        jmethodID id = env->GetMethodID(clazz, "<init>", "()V");
                        JOBJECT obj = (JOBJECT) env->NewObject(clazz, id);
                        exception = new Exception(cur, meth, obj);
                        gc->incrPersist(exception->cause);
                        goto EXCEPTION_HANDLER;
                    }
                    JOBJECT ref = *(JOBJECT*) (stack--);
                    Variable result;
                    if ((type->accessFlags & 0x0020) && (method->name.c_str()[0] != '<')) {
                        int found = 0;
                        for (ClassFile *c = cur->superClass; c != 0; c = c->superClass) {
                            debug("searching in " << c->name << '\n');
                            if (c->methodMap.find(method->full) != c->methodMap.end()) {
                                MethodInfo *invoked = c->methodMap[method->full];
                                debug("invokespecial: " << invoked->full << " in " << c->name << '\n');
                                result = execute(invoked, c, thread, args, method->nargs + 1, &exception);
                                found = 1;
                                break;
                            }
                        }
                        if (!found) {
                            debug("invokespecial: " << method->full << " in " << type->name << '\n');
                            result = execute(method, type, thread, args, method->nargs + 1, &exception);
                        }
                    } else {
                        debug("invokespecial: " << method->full << " in " << type->name << '\n');
                        result = execute(method, type, thread, args, method->nargs + 1, &exception);
                    }
                    if (exception) goto EXCEPTION_HANDLER;
                    if (method->resobject) localgc.addLocal(result.l);
                    if (method->result == 2) {
                        *(JLONG*) (++stack) = result.j;
                        stack++;
                    } else if (method->result == 1) {
                        *(JINT*) (++stack) = result.i;
                    }
                    break;
                }
                case invokestatic:
                {
                    unsigned short int index;
                    U2(index, bc, pc);
                    FieldMethodInfo *info = (FieldMethodInfo*) cp[index];
                    JCLASS type;
                    ClassInfo *cinfo = (ClassInfo*) cp[info->classIndex];
                    if (cinfo->classRef == 0) {
                        std::string name(((UTF8Info*) cp[cinfo->nameIndex])->bytes);
                        type = loader->load((char*) name.c_str(), this);
                        if (thread->nativeException) goto EXCEPTION_HANDLER;
                        cinfo->classRef = type;
                        cinfo->name = name;
                    } else {
                        type = cinfo->classRef;
                    }
                    MethodInfo *method;
                    if (info->methodRef == 0) {
                        if (info->name.size() < 1) {
                            info->name.append(((UTF8Info*) cp[((NameAndTypeInfo*) cp[info->nameAndTypeIndex])->nameIndex])->bytes);
                            info->name.append(((UTF8Info*) cp[((NameAndTypeInfo*) cp[info->nameAndTypeIndex])->descriptorIndex])->bytes);
                        }
                        for (; type != 0; type = type->superClass) {
                            if (type->methodMap.find(info->name) != type->methodMap.end()) {
                                method = type->methodMap[info->name]; //uses name+descriptor
                                break;
                            }
                        }
                        info->methodRef = method;
                    } else {
                        method = info->methodRef;
                    }
                    long args[method->nargs];
                    for (int i = 0; i < method->nargs; i++)
                        args[i] = (stack - method->nargs + 1)[i];
                    stack -= method->nargs;
                    debug("invokestatic: " << method->full << " in " << type->name << '\n');
                    Variable result = execute(method, type, thread, args, method->nargs, &exception);
                    if (exception) goto EXCEPTION_HANDLER;
                    if (method->resobject) localgc.addLocal(result.l);
                    if (method->result == 2) {
                        *(JLONG*) (++stack) = result.j;
                        stack++;
                    } else if (method->result == 1) {
                        *(JINT*) (++stack) = result.i;
                    }
                    break;
                }
                case invokevirtual:
                {
                    unsigned short int index;
                    U2(index, bc, pc);
                    FieldMethodInfo *info = (FieldMethodInfo*) cp[index];
                    MethodInfo *method;
                    ClassInfo *cinfo = (ClassInfo*) cp[info->classIndex];
                    JCLASS type;
                    if (cinfo->classRef == 0) {
                        std::string name(((UTF8Info*) cp[cinfo->nameIndex])->bytes);
                        type = loader->load((char*) name.c_str(), this);
                        if (thread->nativeException) goto EXCEPTION_HANDLER;
                        cinfo->classRef = type;
                        cinfo->name = name;
                    } else {
                        type = cinfo->classRef;
                    }
                    if (info->methodRef == 0) {
                        if (info->name.size() < 1) {
                            info->name.append(((UTF8Info*) cp[((NameAndTypeInfo*) cp[info->nameAndTypeIndex])->nameIndex])->bytes);
                            info->name.append(((UTF8Info*) cp[((NameAndTypeInfo*) cp[info->nameAndTypeIndex])->descriptorIndex])->bytes);
                        }
                        for (; type != 0; type = type->superClass) {
                            if (type->methodMap.find(info->name) != type->methodMap.end()) {
                                method = type->methodMap[info->name]; //uses name+descriptor
                                break;
                            }
                        }
                        info->methodRef = method;
                    } else {
                        method = info->methodRef;
                    }
                    long args[method->nargs + 1]; //contains object ref
                    for (int i = 0; i < method->nargs + 1; i++)
                        args[i] = (stack - method->nargs)[i];
                    stack -= method->nargs;
                    if (!(*(JOBJECT*) stack)) {
                        JNIEnv* env = thread->getEnv();
                        debug("invokevirtual null\n");
                        debug(method->name << '\n');
                        jclass clazz = env->FindClass("java/lang/NullPointerException");
                        if (thread->nativeException) goto EXCEPTION_HANDLER;
                        jmethodID id = env->GetMethodID(clazz, "<init>", "()V");
                        JOBJECT obj = (JOBJECT) env->NewObject(clazz, id);
                        exception = new Exception(cur, meth, obj);
                        gc->incrPersist(exception->cause);
                        goto EXCEPTION_HANDLER;
                    }
                    JOBJECT ref = *(JOBJECT*) (stack--);
                    for (type = ref->type; type != 0; type = type->superClass) {
                        if (type->methodMap.find(method->full) != type->methodMap.end()) {
                            debug("invokevirtual: " << method->full << " in " << type->name << '\n');
                            MethodInfo *invoked = type->methodMap[method->full];
                            Variable result = execute(invoked, type, thread, args, method->nargs + 1, &exception);
                            if (exception) goto EXCEPTION_HANDLER;
                            if (method->resobject) localgc.addLocal(result.l);
                            if (method->result == 2) {
                                *(JLONG*) (++stack) = result.j;
                                stack++;
                            } else if (method->result == 1) {
                                *(JINT*) (++stack) = result.i;
                            }
                            break;
                        }
                    }
                    break;
                }
                case ior:
                {
                    JINT a = *(JINT*) (stack--);
                    *(JINT*) stack = *(JINT*) stack | a;
                    break;
                }
                case irem:
                {
                    JINT a = *(JINT*) (stack--);
                    if (a == 0) {
                        JNIEnv* env = thread->getEnv();
                        jclass clazz = env->FindClass("java/lang/ArithmeticException");
                        if (thread->nativeException) goto EXCEPTION_HANDLER;
                        jmethodID id = env->GetMethodID(clazz, "<init>", "()V");
                        JOBJECT obj = (JOBJECT) env->NewObject(clazz, id);
                        exception = new Exception(cur, meth, obj);
                        gc->incrPersist(exception->cause);
                        goto EXCEPTION_HANDLER;
                    }
                    *(JINT*) stack = (*(JINT*) stack) % a;
                    debug("irem: " << *stack << '\n');
                    break;
                }
                case ireturn:
                {
                    Variable result;
                    result.i = *(JINT*) stack;
                    return result;
                }
                case ishl:
                {
                    int sh = *(stack--);
                    *stack = *stack << sh;
                    break;
                }
                case ishr:
                {
                    int sh = *(stack--);
                    *stack = *stack >> sh;
                    break;
                }
                case istore:
                    locals[bc[pc++]] = *stack;
                    stack--;
                    break;
                case istore_0:
                    locals[0] = *stack;
                    stack--;
                    break;
                case istore_1:
                    locals[1] = *stack;
                    stack--;
                    break;
                case istore_2:
                    locals[2] = *stack;
                    stack--;
                    break;
                case istore_3:
                    locals[3] = *stack;
                    stack--;
                    break;
                case isub:
                {
                    JINT a = *(stack--);
                    *stack = *stack - a;
                    break;
                }
                case iushr:
                {
                    int sh = *(stack--);
                    *(unsigned int*) stack = *(unsigned int*) stack >> sh;
                    break;
                }
                case ixor:
                {
                    JINT a = *(stack--);
                    *stack = *stack ^ a;
                    break;
                }
                case jsr:
                {
                    int orig = pc - 1;
                    short int offset;
                    U2(offset, bc, pc);
                    *(++stack) = pc;
                    pc = orig + offset;
                    break;
                }
                case jsr_w:
                {
                    int orig = pc - 1;
                    long int offset;
                    U4(offset, bc, pc);
                    *(++stack) = pc;
                    pc = orig + offset;
                    break;
                }
                case l2d:
                    *(JDOUBLE*) (stack - 1) = (JDOUBLE) *(JLONG*) (stack - 1);
                    break;
                case l2f:
                    *(JFLOAT*) (stack - 1) = (JFLOAT) *(JLONG*) (stack - 1);
                    stack--;
                    break;
                case l2i:
                    *(JINT*) (stack - 1) = (JINT) *(JLONG*) (stack - 1);
                    stack--;
                    break;
                case ladd:
                {
                    JLONG a = *(JLONG*) (stack - 1);
                    stack -= 2;
                    *(JLONG*) (stack - 1) = *(JLONG*) (stack - 1) + a;
                    debug("ladd: " << *(JLONG*) (stack - 1) << '\n');
                    break;
                }
                case laload:
                {
                    int pos = *(stack--);
                    JLONGARRAY array = (JLONGARRAY) * stack;
                    if (!array) {
                        JNIEnv* env = thread->getEnv();
                        debug("laload null\n");
                        jclass clazz = env->FindClass("java/lang/NullPointerException");
                        if (thread->nativeException) goto EXCEPTION_HANDLER;
                        jmethodID id = env->GetMethodID(clazz, "<init>", "()V");
                        JOBJECT obj = (JOBJECT) env->NewObject(clazz, id);
                        exception = new Exception(cur, meth, obj);
                        gc->incrPersist(exception->cause);
                        goto EXCEPTION_HANDLER;
                    }
                    if (pos < 0 || pos >= array->size) {
                        JNIEnv* env = thread->getEnv();
                        jclass clazz = env->FindClass("java/lang/ArrayIndexOutOfBoundsException");
                        if (thread->nativeException) goto EXCEPTION_HANDLER;
                        jmethodID id = env->GetMethodID(clazz, "<init>", "(I)V");
                        JOBJECT obj = (JOBJECT) env->NewObject(clazz, id, pos);
                        exception = new Exception(cur, meth, obj);
                        gc->incrPersist(exception->cause);
                        goto EXCEPTION_HANDLER;
                    }
                    *(JLONG*) stack = array->array[pos];
                    stack++;
                    break;
                }
                case land:
                {
                    JLONG a = *(JLONG*) (stack - 1);
                    stack -= 2;
                    *(JLONG*) (stack - 1) = *(JLONG*) (stack - 1) & a;
                    break;
                }
                case lastore:
                {
                    JLONG l = *(JLONG*) (stack - 1);
                    stack -= 2;
                    int pos = *(stack--);
                    JLONGARRAY array = (JLONGARRAY) *(stack--);
                    if (!array) {
                        JNIEnv* env = thread->getEnv();
                        debug("lastore null\n");
                        jclass clazz = env->FindClass("java/lang/NullPointerException");
                        if (thread->nativeException) goto EXCEPTION_HANDLER;
                        jmethodID id = env->GetMethodID(clazz, "<init>", "()V");
                        JOBJECT obj = (JOBJECT) env->NewObject(clazz, id);
                        exception = new Exception(cur, meth, obj);
                        gc->incrPersist(exception->cause);
                        goto EXCEPTION_HANDLER;
                    }
                    if (pos < 0 || pos >= array->size) {
                        JNIEnv* env = thread->getEnv();
                        jclass clazz = env->FindClass("java/lang/ArrayIndexOutOfBoundsException");
                        if (thread->nativeException) goto EXCEPTION_HANDLER;
                        jmethodID id = env->GetMethodID(clazz, "<init>", "(I)V");
                        JOBJECT obj = (JOBJECT) env->NewObject(clazz, id, pos);
                        exception = new Exception(cur, meth, obj);
                        gc->incrPersist(exception->cause);
                        goto EXCEPTION_HANDLER;
                    }
                    array->array[pos] = l;
                    break;
                }
                case lcmp:
                {
                    JLONG b = *(JLONG*) (stack - 1);
                    stack -= 2;
                    JLONG a = *(JLONG*) (stack - 1);
                    stack -= 2;
                    if (a < b) *(++stack) = -1;
                    if (a = b) *(++stack) = 0;
                    if (a > b) *(++stack) = 1;
                    break;
                }
                case lconst_0:
                    *(JLONG*) (++stack) = (JLONG) 0;
                    stack++;
                    break;
                case lconst_1:
                    *(JLONG*) (++stack) = (JLONG) 1;
                    stack++;
                    break;
                case ldc:
                {
                    CPInfo *info = cp[bc[pc++]];
                    debug("ldc: ");
                    switch (info->type) {
                        case CPInfo::CONSTANT_Float :
                        {
                            unsigned int bits = *(unsigned int*) ((IntegerFloatInfo*) info)->data;
                            int s = ((bits >> 31) == 0) ? 1 : -1;
                            int e = ((bits >> 23) & 0xff);
                            int m = (e == 0) ?
                                    (bits & 0x7fffff) << 1 :
                                    (bits & 0x7fffff) | 0x800000;
                            *(JFLOAT*) (++stack) = (float) s * (float) m * pow((float) 2, e - 150);
                            debug(*(JFLOAT*) stack << '\n');
                            break;
                        }
                        case CPInfo::CONSTANT_Integer :
                        {
                            *(JINT*) (++stack) = *(JINT*) ((IntegerFloatInfo*) info)->data;
                            debug(*(JINT*) stack << '\n');
                            break;
                        }
                        case CPInfo::CONSTANT_String :
                        {
                            unsigned char* bytes = (unsigned char*) ((UTF8Info*) cp[((StringInfo*) info)->stringIndex])->bytes;
                            std::string cstr((const char*) bytes);
                            if (strings.find(cstr) != strings.end()) {
                                *(JOBJECT*) (++stack) = strings[cstr];
                            } else {
                                JCLASS jstring = loader->load("java/lang/String", this);
                                if (thread->nativeException) goto EXCEPTION_HANDLER;
                                JOBJECT sobj = jstring->newInstance(this);
                                int len = 0;
                                for (int i = 0; i < cstr.length();) {
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
                                debug(len << '\n');
                                JCHARARRAY chars = ClassFile::newArray<JCHAR>(this,(char*)"[C",len);
                                JCHAR* array = chars->array;
                                gc->incrRef(chars);
                                sobj->fields[jstring->fieldMap["value"]->index].l = chars;
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
                                sobj->fields[jstring->fieldMap["offset"]->index].i = 0;
                                sobj->fields[jstring->fieldMap["count"]->index].i = len;
                                strings[cstr] = sobj;
                                gc->incrPersist(sobj);
                                *(JOBJECT*) (++stack) = sobj;
                                localgc.addLocal(*(JOBJECT*) stack);
                            }
                        }
                    }
                    break;
                }
                case ldc_w:
                {
                    unsigned short int index;
                    U2(index, bc, pc);
                    CPInfo *info = cp[index];
                    debug("ldc_w: ");
                    switch (info->type) {
                        case CPInfo::CONSTANT_Float :
                        case CPInfo::CONSTANT_Integer :
                                    *(JINT*) (++stack) = *(JINT*) ((IntegerFloatInfo*) info)->data;
                            debug(*(JFLOAT*) stack << '\n');
                            break;
                        case CPInfo::CONSTANT_String :
                                    unsigned char* bytes = (unsigned char*) ((UTF8Info*) cp[((StringInfo*) info)->stringIndex])->bytes;
                            std::string cstr((const char*) bytes);
                            if (strings.find(cstr) != strings.end()) {
                                *(JOBJECT*) (++stack) = strings[cstr];
                            } else {
                                JCLASS jstring = loader->load("java/lang/String", this);
                                if (thread->nativeException) goto EXCEPTION_HANDLER;
                                JOBJECT sobj = jstring->newInstance(this);
                                int len = 0;
                                for (int i = 0; i < cstr.length();) {
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
                                JCHARARRAY chars = ClassFile::newArray<JCHAR>(this,(char*)"[C",len);
                                JCHAR* array = chars->array;
                                gc->incrRef(chars);
                                sobj->fields[jstring->fieldMap["value"]->index].l = chars;
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
                                sobj->fields[jstring->fieldMap["offset"]->index].i = 0;
                                sobj->fields[jstring->fieldMap["count"]->index].i = len;
                                strings[cstr] = sobj;
                                gc->incrPersist(sobj);
                                *(JOBJECT*) (++stack) = sobj;
                                localgc.addLocal(*(JOBJECT*) stack);
                            }
                    }
                    break;
                }
                case ldc2_w:
                {
                    unsigned short int index;
                    U2(index, bc, pc);
                    CPInfo *info = cp[index];
                    debug("ldc2_w: ");
                    switch (info->type) {
                        case CPInfo::CONSTANT_Long :
                                    *(JLONG*) (++stack) = *(JLONG*) ((LongDoubleInfo*) info)->data;
                            stack++;
                            debug(*(JLONG*) (stack - 1) << '\n');
                            break;
                        case CPInfo::CONSTANT_Double :
                                    unsigned long long bits = *(unsigned long long*) ((LongDoubleInfo*) info)->data;
                            int s = ((bits >> 63) == 0) ? 1 : -1;
                            int e = (int) ((bits >> 52) & 0x7ffL);
                            long long m = (e == 0) ?
                                    (bits & 0xfffffffffffffLLU) << 1 :
                                    (bits & 0xfffffffffffffLLU) | 0x10000000000000LLU;
                            *(JDOUBLE*) (++stack) = s * m * pow(2, e - 1075);
                            stack++;
                            debug(*(JDOUBLE*) (stack - 1) << '\n');
                            break;
                    }
                    break;
                }
                case ldiv:
                {
                    JLONG a = *(JLONG*) (stack - 1);
                    if (a == 0) {
                        JNIEnv* env = thread->getEnv();
                        jclass clazz = env->FindClass("java/lang/ArithmeticException");
                        if (thread->nativeException) goto EXCEPTION_HANDLER;
                        jmethodID id = env->GetMethodID(clazz, "<init>", "()V");
                        JOBJECT obj = (JOBJECT) env->NewObject(clazz, id);
                        exception = new Exception(cur, meth, obj);
                        gc->incrPersist(exception->cause);
                        goto EXCEPTION_HANDLER;
                    }
                    stack -= 2;
                    *(JLONG*) (stack - 1) = *(JLONG*) (stack - 1) / a;
                    break;
                }
                case lload:
                    *(JLONG*) (++stack) = *(JLONG*) & locals[bc[pc++]];
                    stack++;
                    break;
                case lload_0:
                    *(JLONG*) (++stack) = *(JLONG*) & locals[0];
                    stack++;
                    break;
                case lload_1:
                    *(JLONG*) (++stack) = *(JLONG*) & locals[1];
                    stack++;
                    break;
                case lload_2:
                    *(JLONG*) (++stack) = *(JLONG*) & locals[2];
                    stack++;
                    break;
                case lload_3:
                    *(JLONG*) (++stack) = *(JLONG*) & locals[3];
                    stack++;
                    break;
                case lmul:
                {
                    JLONG a = *(JLONG*) (stack - 1);
                    stack -= 2;
                    *(JLONG*) (stack - 1) = *(JLONG*) (stack - 1) * a;
                    break;
                }
                case lneg:
                    *(JLONG*) (stack - 1) = -(*(JLONG*) (stack - 1));
                    break;
                case lookupswitch:
                {
                    int orig = pc - 1;
                    pc += (4 - pc % 4) & 3; //4byte allign
                    int defa;
                    U4(defa, bc, pc);
                    int npairs;
                    U4(npairs, bc, pc);
                    npairs *= 8;
                    int k = *(stack--);
                    int v, offset;
                    for (int i = pc; i < npairs + pc; i += 4) {
                        U4(v, bc, i);
                        if (k == v) {
                            U4(offset, bc, i);
                            pc = orig + offset;
                            goto done;
                        }
                    }
                    pc = orig + defa;
done:
                    debug("lookupswitch: " << k << '\n');
                    break;
                }
                case lor:
                {
                    JLONG a = *(JLONG*) (stack - 1);
                    stack -= 2;
                    *(JLONG*) (stack - 1) = *(JLONG*) (stack - 1) | a;
                    break;
                }
                case lrem:
                {
                    JLONG a = *(JLONG*) (stack - 1);
                    if (a == 0) {
                        JNIEnv* env = thread->getEnv();
                        jclass clazz = env->FindClass("java/lang/ArithmeticException");
                        if (thread->nativeException) goto EXCEPTION_HANDLER;
                        jmethodID id = env->GetMethodID(clazz, "<init>", "()V");
                        JOBJECT obj = (JOBJECT) env->NewObject(clazz, id);
                        exception = new Exception(cur, meth, obj);
                        gc->incrPersist(exception->cause);
                        goto EXCEPTION_HANDLER;
                    }
                    stack -= 2;
                    *(JLONG*) (stack - 1) = *(JLONG*) (stack - 1) % a;
                    debug("lrem: " << *(JLONG*) (stack - 1) << '\n');
                    break;
                }
                case lreturn:
                {
                    Variable result;
                    result.j = *(JLONG*) (stack - 1);
                    return result;
                }
                case lshl:
                {
                    int sh = *(stack--);
                    *(JLONG*) (stack - 1) = *(JLONG*) (stack - 1) << sh;
                    break;
                }
                case lshr:
                {
                    int sh = *(stack--);
                    *(JLONG*) (stack - 1) = *(JLONG*) (stack - 1) >> sh;
                    break;
                }
                case lstore:
                    *(JLONG*) & locals[bc[pc++]] = *(JLONG*) (stack - 1);
                    stack -= 2;
                    break;
                case lstore_0:
                    *(JLONG*) & locals[0] = *(JLONG*) (stack - 1);
                    stack -= 2;
                    break;
                case lstore_1:
                    *(JLONG*) & locals[1] = *(JLONG*) (stack - 1);
                    stack -= 2;
                    break;
                case lstore_2:
                    debug("lstore_2: " << *(JLONG*) (stack - 1) << '\n');
                    *(JLONG*) &locals[2] = *(JLONG*) (stack - 1);
                    stack -= 2;
                    break;
                case lstore_3:
                    *(JLONG*) & locals[3] = *(JLONG*) (stack - 1);
                    stack -= 2;
                    break;
                case lushr:
                {
                    int sh = *(stack--);
                    *(unsigned JLONG*) (stack - 1) = *(unsigned JLONG*) (stack - 1) >> sh;
                    break;
                }
                case lsub:
                {
                    JLONG a = *(JLONG*) (stack - 1);
                    stack -= 2;
                    *(JLONG*) (stack - 1) = *(JLONG*) (stack - 1) - a;
                    break;
                }
                case lxor:
                {
                    JLONG a = *(JLONG*) (stack - 1);
                    stack -= 2;
                    *(JLONG*) (stack - 1) = *(JLONG*) (stack - 1) ^ a;
                    break;
                }
                case monitorenter:
                    if (!(*(JOBJECT*) stack)) {
                        JNIEnv* env = thread->getEnv();
                        debug("monitorenter null\n");
                        jclass clazz = env->FindClass("java/lang/NullPointerException");
                        if (thread->nativeException) goto EXCEPTION_HANDLER;
                        jmethodID id = env->GetMethodID(clazz, "<init>", "()V");
                        JOBJECT obj = (JOBJECT) env->NewObject(clazz, id);
                        exception = new Exception(cur, meth, obj);
                        gc->incrPersist(exception->cause);
                        goto EXCEPTION_HANDLER;
                    }
                    (*(JOBJECT*) stack--)->mutex.lock();
                    debug("Monitor Enter\n");
                    break;
                case monitorexit:
                    if (!(*(JOBJECT*) stack)) {
                        JNIEnv* env = thread->getEnv();
                        debug("monitorexit null\n");
                        jclass clazz = env->FindClass("java/lang/NullPointerException");
                        if (thread->nativeException) goto EXCEPTION_HANDLER;
                        jmethodID id = env->GetMethodID(clazz, "<init>", "()V");
                        JOBJECT obj = (JOBJECT) env->NewObject(clazz, id);
                        exception = new Exception(cur, meth, obj);
                        gc->incrPersist(exception->cause);
                        goto EXCEPTION_HANDLER;
                    }
                    (*(JOBJECT*) stack--)->mutex.unlock();
                    debug("Monitor Exit\n");
                    break;
                case multianewarray:
                {
                    debug("Multi A New Array\n");
                    JNIEnv* env = thread->getEnv();
                    env->FatalError("MultiANewArray not implemented");
                    break;
                }
                case newobj:
                {
                    unsigned short index;
                    U2(index, bc, pc);
                    ClassInfo *cinfo = (ClassInfo*) cp[index];
                    ClassFile* type;
                    if (cinfo->classRef == 0) {
                        type = loader->load(((UTF8Info*) cp[cinfo->nameIndex])->bytes, this);
                        if (thread->nativeException) goto EXCEPTION_HANDLER;
                        cinfo->classRef = type;
                        std::string name(((UTF8Info*) cp[cinfo->nameIndex])->bytes);
                        cinfo->name = name;
                    } else {
                        type = cinfo->classRef;
                    }
                    debug("new: " << cinfo->name << '\n');
                    *(JOBJECT*) (++stack) = type->newInstance(this);
                    localgc.addLocal(*(JOBJECT*) stack);
                    break;
                }
                case newarray:
                {
                    int size = *(stack--);
                    if (size < 0) {
                        JNIEnv* env = thread->getEnv();
                        jclass clazz = env->FindClass("java/lang/NegativeArraySizeException");
                        if (thread->nativeException) goto EXCEPTION_HANDLER;
                        jmethodID id = env->GetMethodID(clazz, "<init>", "()V");
                        JOBJECT obj = (JOBJECT) env->NewObject(clazz, id);
                        exception = new Exception(cur, meth, obj);
                        gc->incrPersist(exception->cause);
                        goto EXCEPTION_HANDLER;
                    }
                    JOBJECT array;
                    switch (bc[pc++]) {
                        case 4:
                            array = ClassFile::newArray<JBOOLEAN>(this,(char*)"[Z",size);
                            break;
                        case 5:
                            array = ClassFile::newArray<JCHAR>(this,(char*)"[C",size);
                            break;
                        case 6:
                            array = ClassFile::newArray<JFLOAT>(this,(char*)"[F",size);
                            break;
                        case 7:
                            array = ClassFile::newArray<JDOUBLE>(this,(char*)"[D",size);
                            break;
                        case 8:
                            array = ClassFile::newArray<JBYTE>(this,(char*)"[B",size);
                            break;
                        case 9:
                            array = ClassFile::newArray<JSHORT>(this,(char*)"[S",size);
                            break;
                        case 10:
                            array = ClassFile::newArray<JINT>(this,(char*)"[I",size);
                            break;
                        case 11:
                            array = ClassFile::newArray<JLONG>(this,(char*)"[J",size);
                            break;
                    }
                    if (thread->nativeException) goto EXCEPTION_HANDLER;
                    *(JOBJECT*) (++stack) = array;
                    localgc.addLocal(*(JOBJECT*) stack);
                    break;
                }
                case nop:
                    break;
                case pop:
                    stack--;
                    break;
                case pop2:
                    stack -= 2;
                    break;
                case putfield:
                {
                    unsigned short int index;
                    U2(index, bc, pc);
                    FieldMethodInfo *info = (FieldMethodInfo*) cp[index];
                    FieldInfo *field;
                    JCLASS type;
                    ClassInfo *cinfo = (ClassInfo*) cp[info->classIndex];
                    if (cinfo->classRef == 0) {
                        std::string name(((UTF8Info*) cp[cinfo->nameIndex])->bytes);
                        type = loader->load((char*) name.c_str(), this);
                        if (thread->nativeException) goto EXCEPTION_HANDLER;
                        cinfo->classRef = type;
                        cinfo->name = name;
                    } else {
                        type = cinfo->classRef;
                    }
                    if (info->fieldRef == 0) {
                        if (info->name.size() < 1)
                            info->name.append(((UTF8Info*) cp[((NameAndTypeInfo*) cp[info->nameAndTypeIndex])->nameIndex])->bytes);
                        field = 0;
                        for (JCLASS find = type; find != 0 && field == 0; find = find->superClass)
                            if (find->fieldMap.find(info->name) != find->fieldMap.end())
                                field = find->fieldMap[info->name];
                        info->fieldRef = field;
                    } else {
                        field = info->fieldRef;
                    }
                    debug("putfield: " << field->descriptor << " " << field->name << '\n');
                    if (!(*(JOBJECT*) (stack - 1))) {
                        JNIEnv* env = thread->getEnv();
                        debug("putfield null\n");
                        jclass clazz = env->FindClass("java/lang/NullPointerException");
                        if (thread->nativeException) goto EXCEPTION_HANDLER;
                        jmethodID id = env->GetMethodID(clazz, "<init>", "()V");
                        JOBJECT obj = (JOBJECT) env->NewObject(clazz, id);
                        exception = new Exception(cur, meth, obj);
                        gc->incrPersist(exception->cause);
                        goto EXCEPTION_HANDLER;
                    }
                    switch (field->descriptor.c_str()[0]) {
                        case 'D':
                        case 'J':
                            (*(JOBJECT*) (stack - 2))->fields[field->index].j = *(JLONG*) (stack - 1);
                            stack -= 3;
                            break;
                        case 'L':
                        case '[':
                        {
                            JOBJECT old = (*(JOBJECT*) (stack - 1))->fields[field->index].l;
                            if (old) gc->decrRef(old);
                            (*(JOBJECT*) (stack - 1))->fields[field->index].i = *stack;
                            gc->incrRef(*(JOBJECT*) stack);
                            stack -= 2;
                            break;
                        }
                        default:
                            (*(JOBJECT*) (stack - 1))->fields[field->index].i = *stack;
                            stack -= 2;
                    }
                    break;
                }
                case putstatic:
                {
                    unsigned short int index;
                    U2(index, bc, pc);
                    FieldMethodInfo *info = (FieldMethodInfo*) cp[index];
                    FieldInfo *field;
                    if (info->fieldRef == 0) {
                        ClassInfo *cinfo = (ClassInfo*) cp[info->classIndex];
                        JCLASS type;
                        if (cinfo->classRef == 0) {
                            std::string name(((UTF8Info*) cp[cinfo->nameIndex])->bytes);
                            type = loader->load((char*) name.c_str(), this);
                            if (thread->nativeException) goto EXCEPTION_HANDLER;
                            cinfo->classRef = type;
                            cinfo->name = name;
                        } else {
                            type = cinfo->classRef;
                        }
                        if (info->name.size() < 1)
                            info->name.append(((UTF8Info*) cp[((NameAndTypeInfo*) cp[info->nameAndTypeIndex])->nameIndex])->bytes);
                        field = 0;
                        for (JCLASS find = type; find != 0 && field == 0; find = find->superClass)
                            if (find->fieldMap.find(info->name) != find->fieldMap.end())
                                field = find->fieldMap[info->name];
                        info->fieldRef = field;
                    } else {
                        field = info->fieldRef;
                    }
                    debug("putfield: " << field->descriptor << " " << field->name << '\n');
                    switch (field->descriptor.c_str()[0]) {
                        case 'D':
                        case 'J':
                            field->staticValue.j = *(JLONG*) (stack - 1);
                            stack -= 2;
                            break;
                        case 'L':
                        case '[':
                            if (field->staticValue.l)
                                gc->decrPersist(field->staticValue.l);
                            field->staticValue.i = *(stack--);
                            gc->incrPersist(field->staticValue.l);
                            break;
                        default:
                            field->staticValue.i = *(stack--);
                    }
                    break;
                }
                case ret:
                    pc = locals[bc[pc++]];
                    break;
                case vreturn:
                    Variable result;
                    return result;
                case saload:
                {
                    int pos = *(stack--);
                    JSHORTARRAY array = (JSHORTARRAY) * stack;
                    if (!array) {
                        JNIEnv* env = thread->getEnv();
                        debug("saload null\n");
                        jclass clazz = env->FindClass("java/lang/NullPointerException");
                        if (thread->nativeException) goto EXCEPTION_HANDLER;
                        jmethodID id = env->GetMethodID(clazz, "<init>", "()V");
                        JOBJECT obj = (JOBJECT) env->NewObject(clazz, id);
                        exception = new Exception(cur, meth, obj);
                        gc->incrPersist(exception->cause);
                        goto EXCEPTION_HANDLER;
                    }
                    if (pos < 0 || pos >= array->size) {
                        JNIEnv* env = thread->getEnv();
                        jclass clazz = env->FindClass("java/lang/ArrayIndexOutOfBoundsException");
                        if (thread->nativeException) goto EXCEPTION_HANDLER;
                        jmethodID id = env->GetMethodID(clazz, "<init>", "(I)V");
                        JOBJECT obj = (JOBJECT) env->NewObject(clazz, id, pos);
                        exception = new Exception(cur, meth, obj);
                        gc->incrPersist(exception->cause);
                        goto EXCEPTION_HANDLER;
                    }
                    *stack = array->array[pos];
                    break;
                }
                case sastore:
                {
                    JSHORT s = *(stack--);
                    int pos = *(stack--);
                    JSHORTARRAY array = (JSHORTARRAY) *(stack--);
                    if (!array) {
                        JNIEnv* env = thread->getEnv();
                        debug("sastore null\n");
                        jclass clazz = env->FindClass("java/lang/NullPointerException");
                        if (thread->nativeException) goto EXCEPTION_HANDLER;
                        jmethodID id = env->GetMethodID(clazz, "<init>", "()V");
                        JOBJECT obj = (JOBJECT) env->NewObject(clazz, id);
                        exception = new Exception(cur, meth, obj);
                        gc->incrPersist(exception->cause);
                        goto EXCEPTION_HANDLER;
                    }
                    if (pos < 0 || pos >= array->size) {
                        JNIEnv* env = thread->getEnv();
                        jclass clazz = env->FindClass("java/lang/ArrayIndexOutOfBoundsException");
                        if (thread->nativeException) goto EXCEPTION_HANDLER;
                        jmethodID id = env->GetMethodID(clazz, "<init>", "(I)V");
                        JOBJECT obj = (JOBJECT) env->NewObject(clazz, id, pos);
                        exception = new Exception(cur, meth, obj);
                        gc->incrPersist(exception->cause);
                        goto EXCEPTION_HANDLER;
                    }
                    array->array[pos] = s;
                    break;
                }
                case sipush:
                {
                    JSHORT s;
                    U2(s, bc, pc);
                    *(++stack) = s;
                    break;
                }
                case swap:
                {
                    int temp = *stack;
                    *stack = *(stack - 1);
                    *(stack - 1) = temp;
                }
                case tableswitch:
                {
                    int orig = pc - 1;
                    pc += (4 - pc % 4) & 3; //4byte allign
                    int defoff;
                    U4(defoff, bc, pc);
                    int low;
                    U4(low, bc, pc);
                    int high;
                    U4(high, bc, pc);
                    int count = high - low + 1;
                    int index = *(stack--);
                    if (index <= high && index >= low) {
                        int offset;
                        int pos = pc + (index - low)*4;
                        U4(offset, bc, pos);
                        pc = orig + offset;
                    } else {
                        pc = orig + defoff;
                    }
                    debug("tableswitch: " << index << '\n');
                    break;
                }
                case wide:
                    switch (bc[pc++]) {
                        case aload:
                        {
                            unsigned int index;
                            U2(index, bc, pc);
                            *(++stack) = locals[index];
                            break;
                        }
                        case iload:
                        case fload:
                        {
                            unsigned int index;
                            U2(index, bc, pc);
                            *(++stack) = locals[index];
                            break;
                        }
                        case dload:
                        case lload:
                        {
                            unsigned int index;
                            U2(index, bc, pc);
                            *(JLONG*) (++stack) = *(JLONG*) & locals[index];
                            stack++;
                            break;
                        }
                        case astore:
                        case istore:
                        case fstore:
                        {
                            unsigned int index;
                            U2(index, bc, pc);
                            locals[index] = *(stack--);
                            break;
                        }
                        case dstore:
                        case lstore:
                        {
                            unsigned int index;
                            U2(index, bc, pc);
                            *(JLONG*) & locals[index] = *(JLONG*) (stack - 1);
                            stack -= 2;
                            break;
                        }
                        case ret:
                        {
                            unsigned int index;
                            U2(index, bc, pc);
                            pc = locals[index];
                            break;
                        }
                        case iinc:
                        {
                            unsigned short int index;
                            U2(index, bc, pc);
                            short int value;
                            U2(value, bc, pc);
                            locals[index] += value;
                            break;
                        }
                    }
                    break;
                case athrow:
                {
                    if (!(*(JOBJECT*) stack)) {
                        JNIEnv* env = thread->getEnv();
                        debug("athrow null\n");
                        jclass clazz = env->FindClass("java/lang/NullPointerException");
                        if (thread->nativeException) goto EXCEPTION_HANDLER;
                        jmethodID id = env->GetMethodID(clazz, "<init>", "()V");
                        JOBJECT obj = (JOBJECT) env->NewObject(clazz, id);
                        exception = new Exception(cur, meth, obj);
                        gc->incrPersist(exception->cause);
                        goto EXCEPTION_HANDLER;
                    }
                    exception = new Exception(cur, meth, *(JOBJECT*) (stack--));
                    gc->incrPersist(exception->cause);
                    goto EXCEPTION_HANDLER;
                }
                default:
                    warn("Unknown Code --- Ignoring\n");
            }
            continue;
EXCEPTION_HANDLER:
            if (thread->nativeException) {
                exception = thread->nativeException;
                thread->nativeException = 0;
            }
            debug("Exception Detected " << (exception->cause ? exception->cause->type->name : "Native") << '\n');
            unsigned short count = meth->code->exceptionCount;
            ExceptionEntry **entries = meth->code->exceptions;
            for (int i = 0; i < count; i++) {
                debug(entries[i]->startPC << '<' << pc << '<' << entries[i]->endPC << '\n');
                if (pc >= entries[i]->startPC && pc <= entries[i]->endPC) {
                    if (entries[i]->catchType == 0) {
                        pc = entries[i]->handlerPC;
                        goto EXCEPTION_CONTINUE;
                    }
                    ClassInfo *cinfo = (ClassInfo*) cp[entries[i]->catchType];
                    ClassFile *type;
                    if (cinfo->classRef == 0) {
                        std::string name(((UTF8Info*) cp[cinfo->nameIndex])->bytes);
                        type = loader->load((char*) name.c_str(), this);
                        if (thread->nativeException) goto EXCEPTION_HANDLER;
                        cinfo->classRef = type;
                        cinfo->name = name;
                    } else {
                        type = cinfo->classRef;
                    }
                    JOBJECT cause = exception->cause;
                    if (type->instanceOf(cause)) {
                        pc = entries[i]->handlerPC;
                        goto EXCEPTION_CONTINUE;
                    }
                }
            }
            *thrown = new Exception(cur, meth, exception);
            debug("Throwing Exeption: " << (exception->cause ? exception->cause->type->name : "Native") << '\n');
            Variable result;
            return result; //meaningless result
EXCEPTION_CONTINUE:
            localgc.addLocal(exception->cause);
            gc->decrPersist(exception->cause);
            *(JOBJECT*) (++stack) = exception->cause;
            delete exception;
            debug("Exception Caught Entering Handler");
            continue;
        }
    }
}
