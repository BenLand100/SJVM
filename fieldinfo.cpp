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
