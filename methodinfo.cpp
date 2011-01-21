#include "MethodInfo.h"
#include "AttributeInfo.h"

#define U2(var,data,index) var = data[index++] << 8; var |= data[index++];
#define U4(var,data,index) var = data[index++] << 24; var |= data[index++] << 16; var |= data[index++] << 8; var |= data[index++];

MethodInfo::MethodInfo(unsigned char *data, int &index) {
    U2(accessFlags,data,index);
    U2(nameIndex,data,index);
    U2(descriptorIndex,data,index);
    U2(attributeCount,data,index);
    attributes = new AttributeInfo*[attributeCount];
    for (int i = 0; i < attributeCount; i++) 
        attributes[i] = new AttributeInfo(data, index);
    nargs = -1;
    argsc = -1;
    result = -1;
    native = 0;
    clazz = 0;
    resobject = 0;
}

MethodInfo::~MethodInfo() {
    if (code) delete code;
    for (int i = 0; i < attributeCount; i++) 
        delete attributes[i];
    delete [] attributes;
}

Code::Code(AttributeInfo *code) {
    unsigned char *data = code->data;
    int index = 0;
    U2(maxStack,data,index);
    U2(maxLocals,data,index);
    U4(codeLength,data,index);
    this->code = new unsigned char[codeLength];
    for (int i = 0; i < codeLength; i++) {
        this->code[i] = data[index+i];
    }
    index += codeLength;
    U2(exceptionCount,data,index);
    exceptions = new ExceptionEntry*[exceptionCount];
    for (int i = 0; i < exceptionCount; i++) {
        exceptions[i] = new ExceptionEntry;
        U2(exceptions[i]->startPC,data,index);
        U2(exceptions[i]->endPC,data,index);
        U2(exceptions[i]->handlerPC,data,index);
        U2(exceptions[i]->catchType,data,index);
    }
    U2(attributeCount,data,index);
    attributes = new AttributeInfo*[attributeCount];
    for (int i = 0; i < attributeCount; i++) 
        attributes[i] = new AttributeInfo(data, index);
}

Code::~Code() {
    delete [] code;
    for (int i = 0; i < exceptionCount; i++) {
        delete exceptions[i];
    }
    delete [] exceptions;
    for (int i = 0; i < attributeCount; i++) {
        delete attributes[i];
    }
    delete [] attributes;
}
