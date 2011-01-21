#include "ClassFile.h"
#include "ClassLoader.h"
#include "FieldInfo.h"
#include "MethodInfo.h"
#include "CPInfo.h"
#include "AttributeInfo.h"
#include "SJVM.h"
#include "Thread.h"

#include <cstring>
#include <map>
#include <vector>
#include <iostream>

//#define DEBUG_OUTPUT

#ifdef DEBUG_OUTPUT
#define debug(v) std::cout << v;
#else
#define debug(v)
#endif

#define U2(var,data,index) var = data[index++] << 8; var |= data[index++];

ClassFile::ClassFile(char *_name, SJVM *jvm) : name(_name) {
    thisClassIndex = 0;
    superClassIndex = 0;
    interfaceCount = 0;
    interfaceIndexes = 0;
    fieldCount = 0;
    fields = 0;
    methodCount = 0;
    methods = 0;
    attributeCount = 0;
    attributes = 0;
    instanceFields = 0;
    accessFlags = 0;
    cpCount = 0;
    cp = 0;
    
    interfaceMap["java/lang/Cloneable"] = jvm->loadClass("java/lang/Cloneable");
    interfaceMap["java/io/Serializable"] = jvm->loadClass("java/io/Serializable");
    
    arrayType = 1;
    superClass = jvm->loadClass("java/lang/Object");
    if (_name[1] == 'L' || _name[1] == '[') {
        componentType = jvm->loadClass(_name + 1);
    } else {
        componentType = 0;
    }
    type = jvm->loadClass("java/lang/Class");
    this->Object::fields = new Variable[type->instanceFields];
    type->prepFields(this);
}

ClassFile::ClassFile(unsigned char *data) : name("null") {
    arrayType = 0;
    componentType = 0;
    int index = 8;
    U2(cpCount, data, index);
    debug("Constant Pool Count " << cpCount << '\n');
    cp = new CPInfo*[cpCount];
    cp[0] = 0;
    for (int i = 1; i < cpCount; i++) {
        cp[i] = CPInfo::read(data, index);
        if (cp[i]->type == CPInfo::CONSTANT_Double || cp[i]->type == CPInfo::CONSTANT_Long) {
            cp[++i] = 0;
        }
        /*switch (cp[i]->type) {
            case CPInfo::CONSTANT_Long :
            {
                LongDoubleInfo *info = (LongDoubleInfo*) cp[i];
                debug(i << ": " << *(long long*)info->data << '\n');
                break;
            }
            case CPInfo::CONSTANT_Utf8 :
            {
                UTF8Info *info = (UTF8Info*) cp[i];
                debug(i << ": " << info->bytes << '\n');
                break;
            }
            case CPInfo::CONSTANT_String :
            {
                StringInfo *info = (StringInfo*) cp[i];
                debug(i << ": StringInfo: " << info->stringIndex << '\n');
                break;
            }
            case CPInfo::CONSTANT_NameAndType :
            {
                NameAndTypeInfo *info = (NameAndTypeInfo*) cp[i];
                debug(i << ": Name: " << info->nameIndex << " Type: " << info->descriptorIndex << '\n');
                break;
            }
            case CPInfo::CONSTANT_Fieldref :
            case CPInfo::CONSTANT_Methodref :
            case CPInfo::CONSTANT_InterfaceMethodref :
            {
                FieldMethodInfo *info = (FieldMethodInfo*) cp[i];
                debug(i << ": Class: " << info->classIndex << " NameAndType: " << info->nameAndTypeIndex << '\n');
                break;
            }
            case CPInfo::CONSTANT_Class :
            {
                ClassInfo *info = (ClassInfo*) cp[i];
                debug(i << ": ClassName: " << info->nameIndex << '\n');
                break;
            }
        }*/
    }
    U2(accessFlags, data, index);
    U2(thisClassIndex, data, index);
    debug("This Class " << thisClassIndex << '\n');
    U2(superClassIndex, data, index);
    debug("Super Class " << superClassIndex << '\n');
    U2(interfaceCount, data, index);
    debug("Interfaces Count " << interfaceCount << '\n');
    interfaceIndexes = new unsigned short int[interfaceCount];
    for (int i = 0; i < interfaceCount; i++) {
        U2(interfaceIndexes[i], data, index);
    }
    U2(fieldCount, data, index);
    debug("Field Count " << fieldCount << '\n');
    fields = new FieldInfo*[fieldCount];
    for (int i = 0; i < fieldCount; i++)
        fields[i] = new FieldInfo(data, index);
    U2(methodCount, data, index);
    debug("Method Count " << methodCount << '\n');
    methods = new MethodInfo*[methodCount];
    for (int i = 0; i < methodCount; i++) {
        methods[i] = new MethodInfo(data, index);
        UTF8Info *info = (UTF8Info*) cp[methods[i]->nameIndex];
        debug(i << ": " << info->bytes << '\n');
    }
    U2(attributeCount, data, index);
    debug("Attribute Count " << attributeCount << '\n');
    attributes = new AttributeInfo*[attributeCount];
    for (int i = 0; i < attributeCount; i++)
        attributes[i] = new AttributeInfo(data, index);
}

ClassFile::~ClassFile() {
    for (int i = 0; i < cpCount; i++)
        if (cp[i] != 0) delete cp[i];
    delete [] cp;
    delete [] interfaceIndexes;
    for (int i = 0; i < fieldCount; i++) {
        //if (fields[i]->staticObject)
        //    fields[i]->staticValue.l->staticCount--;
        delete fields[i];
    }
    delete [] fields;
    for (int i = 0; i < methodCount; i++)
        delete methods[i];
    delete [] methods;
    for (int i = 0; i < attributeCount; i++)
        delete attributes[i];
    delete [] attributes;
}

int ClassFile::validate(ClassLoader *loader, SJVM *jvm) {
    ClassInfo *cinfo = (ClassInfo*) cp[thisClassIndex];
    UTF8Info *uinfo = (UTF8Info*) cp[cinfo->nameIndex];
    name.clear();
    name.append(uinfo->bytes);
    debug("Validating " << name << '\n');
    if (superClassIndex != 0) {
        ClassInfo *cinfo = (ClassInfo*) cp[superClassIndex];
        UTF8Info *uinfo = (UTF8Info*) cp[cinfo->nameIndex];
        superClass = loader->load(uinfo->bytes, jvm);
        instanceFields = superClass->instanceFields;
    } else {
        superClass = 0;
        instanceFields = 0;
    }
    for (int i = 0; i < interfaceCount; i++) {
        ClassInfo *cinfo = (ClassInfo*) cp[interfaceIndexes[i]];
        UTF8Info *uinfo = (UTF8Info*) cp[cinfo->nameIndex];
        ClassFile *interface = loader->load(uinfo->bytes, jvm);
        interfaceMap[interface->name] = interface;
    }
    for (int i = 0; i < methodCount; i++) {
        UTF8Info *uinfo;
        uinfo = (UTF8Info*) cp[methods[i]->nameIndex];
        methods[i]->clazz = this;
        methods[i]->name.clear();
        methods[i]->name.append(uinfo->bytes);
        debug("\t-" << methods[i]->name << " ");
        uinfo = (UTF8Info*) cp[methods[i]->descriptorIndex];
        methods[i]->descriptor.clear();
        methods[i]->descriptor.append(uinfo->bytes);
        methods[i]->nargs = 0;
        methods[i]->argsc = 0;
        for (char *c = ((char*) methods[i]->descriptor.c_str()) + 1;; c++) {
            switch (*c) {
                case '[': 
                    continue;
                case ')':
                    switch(*(++c)) {
                        case 'V':
                            methods[i]->result = 0;
                            break;
                        case 'D':
                        case 'J':
                            methods[i]->result = 2;
                            break;
                        case 'L':
                        case '[':
                            methods[i]->resobject = 1;
                        default:
                            methods[i]->result = 1;
                    }
                    break;
                case 'L':
                    for (;*c != ';';c++);
                case 'Z':
                case 'C':
                case 'B':
                case 'S':
                case 'I':
                case 'F':
                    methods[i]->nargs++;
                    methods[i]->argsc++;
                    methods[i]->typeSz.push_back(1);
                    continue;
                case 'J':
                case 'D':
                    methods[i]->nargs+=2;
                    methods[i]->argsc++;
                    methods[i]->typeSz.push_back(2);
                    continue;
            }
            break;
        }
        debug(methods[i]->descriptor << '\n');
        methods[i]->code = 0;
        for (int c = 0; c < methods[i]->attributeCount; c++) {
            AttributeInfo *ainfo = methods[i]->attributes[c];
            char* name = ((UTF8Info*) cp[ainfo->nameIndex])->bytes;
            if (!strcmp(name, "Code")) {
                methods[i]->code = new Code(ainfo);
                break;
            }
        }
        methods[i]->full.clear();
        methods[i]->full.append(methods[i]->name);
        methods[i]->full.append(methods[i]->descriptor);
        methodMap[methods[i]->full] = methods[i];
    }
    if (superClass)
        objectIndexes = superClass->objectIndexes;
    debug("Starting with " << objectIndexes.size() << " Object Fields\n");
    for (int i = 0; i < fieldCount; i++) {
        UTF8Info *uinfo;
        uinfo = (UTF8Info*) cp[fields[i]->nameIndex];
        fields[i]->name.clear();
        fields[i]->name.append(uinfo->bytes);
        debug("\t-" << fields[i]->name << ' ');
        uinfo = (UTF8Info*) cp[fields[i]->descriptorIndex];
        fields[i]->descriptor.clear();
        fields[i]->descriptor.append(uinfo->bytes);
        debug(fields[i]->descriptor << '\n');
        if (fields[i]->accessFlags & 0x0008) { //static
            fields[i]->index = -1;
            switch (fields[i]->descriptor.c_str()[0]) {
                case 'Z':
                    fields[i]->staticValue.z = 0;
                    break;
                case 'B':
                    fields[i]->staticValue.b = 0;
                    break;
                case 'C':
                    fields[i]->staticValue.c = 0;
                    break;
                case 'S':
                    fields[i]->staticValue.s = 0;
                    break;
                case 'F':
                    fields[i]->staticValue.f = 0.0;
                    break;
                case 'D':
                    fields[i]->staticValue.d = 0.0;
                    break;
                case 'I':
                    fields[i]->staticValue.i = 0;
                    break;
                case 'J':
                    fields[i]->staticValue.j = 0;
                    break;
                case 'L':
                case '[':
                    fields[i]->staticValue.l = 0;
                    fields[i]->staticObject = 1;
                    break;
                default:
                    debug("Invalid descriptor\n");
            }
        } else {
            instanceFields++;
            fields[i]->index = instanceFields - 1;
            if (fields[i]->descriptor.c_str()[0] == 'L' || fields[i]->descriptor.c_str()[0] == '[')
                objectIndexes.push_back(instanceFields - 1);
        }
        fieldMap[fields[i]->name] = fields[i];
    }
    type = loader->load("java/lang/Class",jvm);
    Object::fields = new Variable[type->instanceFields];
    type->prepFields(this);
    if (methodMap.find("<clinit>()V") != methodMap.end()) {
        debug("Running static initilizer\n");
        MethodInfo *info = methodMap["<clinit>()V"];
        Thread *thread = Thread::getThread(jvm);
        jvm->execute(info,this, thread,0,0,&thread->nativeException);
    }
    return 0;
}

//ONLY PLACE NEW OBJECTS ARE CREATED... Minus native ones
Object* ClassFile::newInstance(SJVM *sjvm) {
    if (arrayType) return 0; //use newArray
    Object* ref = new Object();
    sjvm->gc->reg(ref);
    debug("New " << this->name << " allocated\n");
    ref->type = this;
    ref->fields = new Variable[instanceFields];
    prepFields(ref);
    return ref;
}

void ClassFile::prepFields(Object *ref) {
    debug("Prepping " << this->name << " fields\n");
    if (superClass != 0) superClass->prepFields(ref);
    for (std::map<std::string, FieldInfo*>::iterator iter = fieldMap.begin(); iter != fieldMap.end(); iter++) {
        FieldInfo* info = iter->second;
        if (info->index >= 0) {
            debug(info->index << ": " << iter->first << '\n');
            switch (info->descriptor.c_str()[0]) {
                case 'Z':
                    ref->fields[info->index].z = 0;
                    break;
                case 'B':
                    ref->fields[info->index].b = 0;
                    break;
                case 'C':
                    ref->fields[info->index].c = 0;
                    break;
                case 'S':
                    ref->fields[info->index].s = 0;
                    break;
                case 'F':
                    ref->fields[info->index].f = 0.0;
                    break;
                case 'D':
                    ref->fields[info->index].d = 0.0;
                    break;
                case 'I':
                    ref->fields[info->index].i = 0;
                    break;
                case 'J':
                    ref->fields[info->index].j = 0;
                    break;
                case 'L':
                case '[':
                    ref->fields[info->index].l = 0;
                    break;
                default:
                    debug("Invalid descriptor\n");
            }
        }
    }
}

JINT ClassFile::instanceOf(Object *ref) {
    debug((int)ref->type << ref->type->name << " instanceof " << name << '\n');
    if (ref->type->arrayType) {
        if (superClass == 0) return 1;
        for (JCLASS type = ref->type; type != 0; type = type->superClass) {
            debug("subclass...? " << type->name << '\n');
            if (type == this) return 1;
            for (std::map<std::string,ClassFile*>::iterator iter = type->interfaceMap.begin(); iter != type->interfaceMap.end(); iter++) {
                debug("implements...? " << iter->second->name << '\n');
                if (iter->second == this) return 1;
            }
        }
        /*for (JCLASS type = ref->type->componentType; type != 0; type = type->superClass) {debug("subclass...? " << type->name << '\n');
            if (type == componentType) return 1;
            for (std::map<std::string,ClassFile*>::iterator iter = type->interfaceMap.begin(); iter != type->interfaceMap.end(); iter++) {
                debug("implements...? " << iter->second->name << '\n');
                if (iter->second == componentType) return 1;
            }
        }*/
        return 0;
    } else {
        if (superClass == 0) return 1;
        for (JCLASS type = ref->type; type != 0; type = type->superClass) {
            debug("subclass...? " << type->name << '\n');
            if (type == this) return 1;
            for (std::map<std::string,ClassFile*>::iterator iter = type->interfaceMap.begin(); iter != type->interfaceMap.end(); iter++) {
                debug("implements...? " << iter->second->name << '\n');
                if (iter->second == this) return 1;
            }
        }
        return 0;
    }
}
