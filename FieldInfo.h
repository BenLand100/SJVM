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

