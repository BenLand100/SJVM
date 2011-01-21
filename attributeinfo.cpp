#include "AttributeInfo.h"

#define U2(var,data,index) var = data[index++] << 8; var |= data[index++];
#define U4(var,data,index) var = data[index++] << 24; var |= data[index++] << 16; var |= data[index++] << 8; var |= data[index++];

#include <iostream>

AttributeInfo::AttributeInfo(unsigned char *data, int &index) {
    U2(nameIndex,data,index);
    U4(length,data,index);
    this->data = new unsigned char[length];
    for (int i = 0; i < length; i++)
        this->data[i] = data[i+index];
    index += length;
}

AttributeInfo::~AttributeInfo() {
    delete [] data;
}
