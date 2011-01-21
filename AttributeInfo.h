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
 * File:   AttributeInfo.h
 * Author: Benjamin J. Land
 *
 * Created on November 25, 2008, 7:37 PM
 */

#ifndef _ATTRIBUTEINFO_H
#define	_ATTRIBUTEINFO_H

class AttributeInfo {
public:
    short int nameIndex;
    long int length;
    unsigned char *data;
    
    ~AttributeInfo();
    
    AttributeInfo(unsigned char *data, int &index);
};

#endif	/* _ATTRIBUTEINFO_H */

