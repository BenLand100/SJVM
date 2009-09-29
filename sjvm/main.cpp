
#include "jni.h"

#include "ClassFile.h"
#include "ClassLoader.h"
#include "SJVM.h"
#include "Thread.h"
#include "Object.h"

#include <iostream>
#include <fstream>
#include <cstring>

using namespace std;

void printall(); //object.cpp

/*
 * 
 */
int main(int argc, char** argv) {
    JavaVMInitArgs args;
    args.version = JNI_VERSION_1_1;
    args.nOptions = 1;
    args.options = new JavaVMOption[1];
    args.options->optionString = (char*)"/home/benland100/Desktop/sjvm/javatest/";
    JavaVM *vm;
    JNIEnv *env;
    JNI_CreateJavaVM(&vm,(void**)&env,(void*)&args);
    delete [] args.options;
    //jclass system = env->FindClass("java/lang/System");
    //jclass printstream = env->FindClass("java/io/PrintStream");
    //jfieldID outID = env->GetStaticFieldID(system,"out","Ljava/io/PrintStream;");
    //jobject out = env->GetStaticObjectField(system,outID);
    //cout << (int)out << '\n';
    //jclass mainclass = env->FindClass("smart/Client");
    //jmethodID main = env->GetStaticMethodID(mainclass,"main","([Ljava/lang/String;)V");
    //env->CallStaticVoidMethod(mainclass,main,env->NewObjectArray(0,env->FindClass("java/lang/String"),0));
    //jobject obj = env->AllocObject(exception);//env->NewObject(exception,einit);
    //env->CallVoidMethod(obj,einit);
    //jclass test = env->FindClass("fields/subclass");

    jclass mainclass = env->FindClass("javatest");
    jmethodID main = env->GetStaticMethodID(mainclass,"main","([Ljava/lang/String;)V");

    try {
        env->CallStaticVoidMethod(mainclass,main);
        if (env->ExceptionOccurred() == 0) {
            cout << "Executing Success\n";
        } else {
            cout << "Damn, exceptions...\n";
        }
    } catch ( ... ) {
    }
    env->DeleteLocalRef(mainclass);
    vm->DestroyJavaVM();
    return 0;
}

