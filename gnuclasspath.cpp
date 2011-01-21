#include <sstream>

#include "jni.h"
#include "Arrays.h"
#include "ClassFile.h"
#include "SJVM.h"
#include <iostream>
#include <string>

//#define DEBUG_OUTPUT

#ifdef DEBUG_OUTPUT
#define debug(v) std::cout << v;
#else
#define debug(v)
#endif

extern "C" void JNICALL Java_gnu_classpath_VMSystemProperties_preInit(JNIEnv* env, jclass type, jobject properties) {
    jclass props = env->FindClass("java/util/Properties");
    jmethodID set = env->GetMethodID(props, "setProperty", "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/Object;");
    env->CallObjectMethod(properties, set, env->NewStringUTF("java.version"), env->NewStringUTF(""));
    env->CallObjectMethod(properties, set, env->NewStringUTF("java.vendor"), env->NewStringUTF(""));
    env->CallObjectMethod(properties, set, env->NewStringUTF("java.vendor.url"), env->NewStringUTF(""));
    env->CallObjectMethod(properties, set, env->NewStringUTF("java.home"), env->NewStringUTF(""));
    env->CallObjectMethod(properties, set, env->NewStringUTF("java.vm.specification.version"), env->NewStringUTF(""));
    env->CallObjectMethod(properties, set, env->NewStringUTF("java.vm.specification.vendor"), env->NewStringUTF(""));
    env->CallObjectMethod(properties, set, env->NewStringUTF("java.vm.specification.name"), env->NewStringUTF(""));
    env->CallObjectMethod(properties, set, env->NewStringUTF("java.vm.version"), env->NewStringUTF(""));
    env->CallObjectMethod(properties, set, env->NewStringUTF("java.vm.vendor"), env->NewStringUTF(""));
    env->CallObjectMethod(properties, set, env->NewStringUTF("java.vm.name"), env->NewStringUTF(""));
    env->CallObjectMethod(properties, set, env->NewStringUTF("java.specification.version"), env->NewStringUTF(""));
    env->CallObjectMethod(properties, set, env->NewStringUTF("java.specification.vendor"), env->NewStringUTF(""));
    env->CallObjectMethod(properties, set, env->NewStringUTF("java.specification.name"), env->NewStringUTF(""));
    env->CallObjectMethod(properties, set, env->NewStringUTF("java.class.version"), env->NewStringUTF(""));
    env->CallObjectMethod(properties, set, env->NewStringUTF("java.class.path"), env->NewStringUTF(""));
    env->CallObjectMethod(properties, set, env->NewStringUTF("java.library.path"), env->NewStringUTF(""));
    env->CallObjectMethod(properties, set, env->NewStringUTF("java.io.tmpdir"), env->NewStringUTF(""));
    env->CallObjectMethod(properties, set, env->NewStringUTF("java.compiler"), env->NewStringUTF(""));
    env->CallObjectMethod(properties, set, env->NewStringUTF("java.ext.dirs"), env->NewStringUTF(""));
    env->CallObjectMethod(properties, set, env->NewStringUTF("os.name"), env->NewStringUTF(""));
    env->CallObjectMethod(properties, set, env->NewStringUTF("os.arch"), env->NewStringUTF(""));
    env->CallObjectMethod(properties, set, env->NewStringUTF("os.version"), env->NewStringUTF(""));
    env->CallObjectMethod(properties, set, env->NewStringUTF("file.separator"), env->NewStringUTF(""));
    env->CallObjectMethod(properties, set, env->NewStringUTF("path.separator"), env->NewStringUTF(""));
    env->CallObjectMethod(properties, set, env->NewStringUTF("line.separator"), env->NewStringUTF(""));
    env->CallObjectMethod(properties, set, env->NewStringUTF("user.name"), env->NewStringUTF(""));
    env->CallObjectMethod(properties, set, env->NewStringUTF("user.home"), env->NewStringUTF(""));
    env->CallObjectMethod(properties, set, env->NewStringUTF("user.dir"), env->NewStringUTF(""));
    env->CallObjectMethod(properties, set, env->NewStringUTF("gnu.cpu.endian"), env->NewStringUTF(""));
}

extern "C" void JNICALL Java_java_lang_VMSystem_arraycopy(JNIEnv* env, jclass type, jobject src, jint sstart, jobject dest, jint dstart, jint length) {
    JBYTEARRAY s = (JBYTEARRAY) src;
    JBYTEARRAY d = (JBYTEARRAY) dest;
    debug(d->elemsz << ' ' << s->elemsz << ' ' << sstart << ' ' << dstart << ' ' << length << '\n');
    memcpy(d->array + d->elemsz*dstart, s->array + s->elemsz*sstart, s->elemsz * length);
}

extern "C" jobject JNICALL Java_java_lang_VMObject_clone(JNIEnv* env, jclass type, jobject cloner) {
    JOBJECT obj = (JOBJECT)cloner;
    JCLASS cls = obj->type;
    if (cls->arrayType) {
        return (jobject) ((JBYTEARRAY)obj)->clone();
    } else {
        JOBJECT clone = cls->newInstance((SJVM*) env->functions->reserved0); //No need to add it to the gc, done on return
        memcpy(clone->fields,obj->fields,cls->instanceFields*sizeof(Variable));
        for (int i = 0; i < cls->objectIndexes.size(); i++) 
            ((SJVM*)env->functions->reserved0)->gc->incrRef(clone->fields[cls->objectIndexes[i]].l);
        return (jobject) clone;
    }
}

extern "C" jstring JNICALL Java_java_lang_VMDouble_toString(JNIEnv* env, jclass type, jdouble val, jboolean isfloat) {
    std::stringstream stream;
    stream << val;
    return env->NewStringUTF(stream.str().c_str());
}

extern "C" jclass JNICALL Java_java_lang_VMObject_getClass(JNIEnv* env, jclass type, jobject obj) {
    return (jclass) ((JOBJECT)obj)->type;
}

extern "C" jstring JNICALL Java_java_lang_VMClass_getName(JNIEnv* env, jclass type, jclass cls) {
    return env->NewStringUTF(((JCLASS)cls)->name.c_str());
}

