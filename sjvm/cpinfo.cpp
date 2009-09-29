#include "CPInfo.h"
#include "FieldInfo.h"
#include "MethodInfo.h"

#include <iostream>

#define U2(var,data,index) var = data[index++] << 8; var |= data[index++];

CPInfo::CPInfo(char type) {
    this->type = type;
}

CPInfo* CPInfo::read(unsigned char *data, int &index) {
    switch (data[index]) {
        case CONSTANT_Class:
            return new ClassInfo(data,index);
        case CONSTANT_Fieldref:
        case CONSTANT_Methodref:
        case CONSTANT_InterfaceMethodref:
            return new FieldMethodInfo(data,index);
        case CONSTANT_String:
            return new StringInfo(data,index);
        case CONSTANT_Integer:
        case CONSTANT_Float:
            return new IntegerFloatInfo(data,index);
        case CONSTANT_Long:
        case CONSTANT_Double:
            return new LongDoubleInfo(data,index);
        case CONSTANT_NameAndType:
            return new NameAndTypeInfo(data,index);
        case CONSTANT_Utf8:
            return new UTF8Info(data,index);
    }
    return 0;
}

UTF8Info::UTF8Info(unsigned char *data, int &index) : CPInfo(data[index++]) {
    U2(size,data,index);
    bytes = new char[size+1];
    for (int i = 0; i < size; i++) 
        bytes[i] = data[i+index];
    bytes[size] = 0;
    index += size;
}

UTF8Info::~UTF8Info() {
    delete [] bytes;
}

StringInfo::StringInfo(unsigned char *data, int &index) : CPInfo(data[index++]) {
    U2(stringIndex,data,index);
}

NameAndTypeInfo::NameAndTypeInfo(unsigned char *data, int &index) : CPInfo(data[index++]) {
    U2(nameIndex,data,index);
    U2(descriptorIndex,data,index);
}

LongDoubleInfo::LongDoubleInfo(unsigned char *data, int &index) : CPInfo(data[index++]) {
    for (int i = 0; i < 8; i++) 
        this->data[i] = data[7-i+index];
    index += 8;
}

IntegerFloatInfo::IntegerFloatInfo(unsigned char *data, int &index) : CPInfo(data[index++]) {
    for (int i = 0; i < 4; i++) 
        this->data[i] = data[3-i+index];
    index += 4;
}

FieldMethodInfo::FieldMethodInfo(unsigned char *data, int &index) : CPInfo(data[index++]) {
    U2(classIndex,data,index);
    U2(nameAndTypeIndex,data,index);
    fieldRef = 0;
    methodRef = 0;
}

ClassInfo::ClassInfo(unsigned char *data, int &index) : CPInfo(data[index++]) {
    U2(nameIndex,data,index);
    classRef = 0;
}
