#include "FieldInfo.h"
#include "AttributeInfo.h"

#define U2(var,data,index) var = data[index++] << 8; var |= data[index++];

FieldInfo::FieldInfo(unsigned char *data, int &index) {
    staticObject = 0;
    U2(accessFlags,data,index);
    U2(nameIndex,data,index);
    U2(descriptorIndex,data,index);
    U2(attributeCount,data,index);
    attributes = new AttributeInfo*[attributeCount];
    for (int i = 0; i < attributeCount; i++) 
        attributes[i] = new AttributeInfo(data, index);
}

FieldInfo::~FieldInfo() {
    for (int i = 0; i < attributeCount; i++) 
        delete attributes[i];
    delete [] attributes;
}
