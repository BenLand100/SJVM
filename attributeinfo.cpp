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
