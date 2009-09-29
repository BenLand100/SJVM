/* 
 * File:   MethodInfo.h
 * Author: Benjamin J. Land
 *
 * Created on November 25, 2008, 7:36 PM
 */

#ifndef _METHODINFO_H
#define	_METHODINFO_H

#include <string>
#include <vector>
#include "ClassFile.h"

class AttributeInfo;

typedef struct {
    unsigned short int startPC;
    unsigned short int endPC;
    unsigned short int handlerPC;
    unsigned short int catchType;
} ExceptionEntry;

class Code {
public:
    unsigned short int maxStack;
    unsigned short int maxLocals;
    unsigned long int codeLength;
    unsigned char *code;
    unsigned short int exceptionCount;
    ExceptionEntry **exceptions;
    unsigned short int attributeCount;
    AttributeInfo **attributes;
    
    Code(AttributeInfo *code);
    ~Code();
};

class MethodInfo {
    friend class ClassFile;
private:
    unsigned short int nameIndex;
    unsigned short int descriptorIndex;
    unsigned short int attributeCount;
    AttributeInfo **attributes;

    MethodInfo(unsigned char *data, int &index);

public:
    unsigned short int accessFlags;
    std::string name;
    std::string descriptor;
    std::string full;
    std::vector<char> typeSz;
    ClassFile* clazz;
    int nargs; //size in bytes
    int argsc; //number of args
    int result;
    int resobject;
    Code *code;
    void *native;
    
    ~MethodInfo();
};

#endif	/* _METHODINFO_H */

