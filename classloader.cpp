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

#include "Arrays.h"
#include "ClassFile.h"
#include "SJVM.h"
#include "ClassLoader.h"
#include "Thread.h"
#include "Exception.h"

#include <iostream>
#include <fstream>
#include <string>

//#define DEBUG_OUTPUT

#ifdef DEBUG_OUTPUT
#define debug(v) std::cout << v;
#else
#define debug(v)
#endif

FolderClassPath::FolderClassPath(const char* cpath) : path(cpath) {

}

int FolderClassPath::request(const char *name, unsigned char *&data) {
    std::string fpath(path);
    fpath.append(name);
    fpath.append(".class");
    std::ifstream file(fpath.c_str(), std::ios::in | std::ios::binary | std::ios::ate);
    if (file.is_open()) {
        int size = file.tellg();
        data = new unsigned char[size];
        file.seekg(0, std::ios::beg);
        file.read((char*) data, size);
        file.close();
        return 0;
    }
    return -1;
}

void FolderClassPath::release(unsigned char *data) {
    delete [] data;
}

void BootstrapClassPath::add(ClassPath* path) {
    paths.push_back(path);
}

BootstrapClassPath::~BootstrapClassPath() {
    for (int i = 0; i < paths.size(); i++) {
        delete paths[i];
    }
}

int BootstrapClassPath::request(const char* name, unsigned char *&data) {
    for (int i = 0; i < paths.size(); i++) {
        if (paths[i]->request(name, data) == 0) {
            mapped[data] = paths[i];
            return 0;
        }
    }
    return 1;
}

void BootstrapClassPath::release(unsigned char* data) {
    mapped[data]->release(data);
    mapped.erase(data);
}

ClassLoader::ClassLoader(ClassPath *classpath) {
    bootstrap = classpath;
}

ClassFile* ClassLoader::load(const char* name, SJVM *jvm) {
    mutex.lock();
    if (classes.find(name) == classes.end()) {
        unsigned char *data;
        debug("Loading class " << name << '\n');
        if (name[0] == '[') {
            debug("Array Type detected\n");
            ClassFile *atype = new ClassFile((char*)name, jvm);
            classes[name] = atype;
            return atype;
        }
        if (bootstrap->request(name, data)) {
            Thread *thread = Thread::getThread(jvm);
            JNIEnv* env = thread->getEnv();
            jclass clazz = env->FindClass("java/lang/ClassNotFoundException");
            jmethodID id = env->GetMethodID(clazz, "<init>", "(Ljava/lang/String;)V");
            JOBJECT obj = (JOBJECT) env->NewObject(clazz, id, env->NewStringUTF(name));
            thread->nativeException = new Exception(0, 0, obj);
            return 0;
        }
        ClassFile *classfile = new ClassFile(data);
        bootstrap->release(data);
        classes[name] = classfile;
        classfile->validate(this, jvm);
        return classfile;
    }
    debug("Class " << name << " Cached\n");
    ClassFile *result = classes[name];
    mutex.unlock();
    return result;
}

ClassLoader::~ClassLoader() {
    mutex.lock();
    for (std::map<std::string, ClassFile*>::iterator iter = classes.begin(); iter != classes.end(); iter++) {
        debug("Freeing class " << iter->second->name << '\n');
        delete iter->second;
    }
    classes.clear();
    delete bootstrap;
    mutex.unlock();
}
