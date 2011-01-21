/* 
 * File:   CPInfo.h
 * Author: Benjamin J. Land
 *
 * Created on November 25, 2008, 7:37 PM
 */

#ifndef _CPINFO_H
#define	_CPINFO_H

#include <string>

class FieldInfo;
class MethodInfo;
class ClassFile;

class CPInfo {
public:
    const static char CONSTANT_Class = 7;
    const static char CONSTANT_Fieldref = 9;
    const static char CONSTANT_Methodref = 10;
    const static char CONSTANT_InterfaceMethodref = 11;
    const static char CONSTANT_String = 8;
    const static char CONSTANT_Integer = 3;
    const static char CONSTANT_Float = 4;
    const static char CONSTANT_Long = 5;
    const static char CONSTANT_Double = 6;
    const static char CONSTANT_NameAndType = 12;
    const static char CONSTANT_Utf8 = 1;
    
    char type;
    
    static CPInfo* read(unsigned char *data, int &index);
    
    virtual ~CPInfo() { };
    
protected:
    CPInfo(char type);
    
};

class UTF8Info : public CPInfo {
public:
    short int size;
    char *bytes;
    UTF8Info(unsigned char *data, int &index);
    virtual ~UTF8Info();
};

class StringInfo : public CPInfo {
public:
    short int stringIndex;
    StringInfo(unsigned char *data, int &index);
};

class NameAndTypeInfo : public CPInfo {
public:
    short int nameIndex;
    short int descriptorIndex;
    NameAndTypeInfo(unsigned char *data, int &index);
};

class LongDoubleInfo : public CPInfo {
public:
    char data[8];
    LongDoubleInfo(unsigned char *data, int &index);
};

class IntegerFloatInfo : public CPInfo {
public:
    char data[4];
    IntegerFloatInfo(unsigned char *data, int &index);
};

class FieldMethodInfo : public CPInfo {
public:
    short int classIndex;
    short int nameAndTypeIndex;
    FieldInfo* fieldRef;
    MethodInfo* methodRef;
    std::string name;
    FieldMethodInfo(unsigned char *data, int &index);
};

class ClassInfo : public CPInfo {
public:
    short int nameIndex;
    ClassFile *classRef;
    std::string name;
    ClassInfo(unsigned char *data, int &index);
};

#endif	/* _CPINFO_H */

