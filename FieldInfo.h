/* 
 * File:   FieldInfo.h
 * Author: Benjamin J. Land
 *
 * Created on November 25, 2008, 7:36 PM
 */

#ifndef _FIELDINFO_H
#define	_FIELDINFO_H

class AttributeInfo;

#include "Types.h"

#include <string>

class FieldInfo {
    friend class ClassFile;
private:
    short int accessFlags;
    short int nameIndex;
    short int descriptorIndex;
    short int attributeCount;
    AttributeInfo **attributes;
    char staticObject;   
    
    FieldInfo(unsigned char *data, int &index);
    
public:
    std::string name;
    std::string descriptor;
    int index; //0 to n stored in object, -1 class field
    Variable staticValue;
    
    ~FieldInfo();
};

#endif	/* _FIELDINFO_H */

