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
