#ifndef _CLASS_H
#define	_CLASS_H

#include "Types.h"
#include "Object.h"
#include "SJVM.h"

#include <map>
#include <string>
#include <vector>

class SJVM;
class MethodInfo;
class FieldInfo;
class AttributeInfo;
class CPInfo;
class Thread;

class ClassFile : public Object {
    
private:
    friend class ClassLoader;
    
    unsigned short int thisClassIndex;
    unsigned short int superClassIndex;
    unsigned short int interfaceCount;
    unsigned short int *interfaceIndexes;
    unsigned short int fieldCount;
    FieldInfo **fields;
    unsigned short int methodCount;
    MethodInfo **methods;
    unsigned short int attributeCount;
    AttributeInfo **attributes;
    
    
    ClassFile(unsigned char *data);
    ClassFile(char *name, SJVM *jvm);
    int validate(ClassLoader *loader, SJVM *jvm);
    void prepFields(Object *ref);
    
public:
    
    template <class T> static Array<T>* newArray(SJVM* sjvm, char *type, int size) {
        if (*type != '[') return 0;
        Array<T> *ref = new Array<T>(size);
        sjvm->gc->reg(ref);
        ref->type = sjvm->loadClass(type);
        return ref;
    };
    
    unsigned short int instanceFields;
    unsigned short int accessFlags;
    unsigned short int cpCount;
    bool arrayType;
    ClassFile *componentType;
    CPInfo **cp;
    std::string name;
    ClassFile *superClass;
    std::map<std::string,MethodInfo*> methodMap;
    std::map<std::string,FieldInfo*> fieldMap;
    std::map<std::string,ClassFile*> interfaceMap;
  
    std::vector<unsigned int> objectIndexes;
    
    JOBJECT newInstance(SJVM* sjvm);
    JINT instanceOf(Object *ref);
    
    virtual ~ClassFile();
    
};


#endif	/* _CLASS_H */

