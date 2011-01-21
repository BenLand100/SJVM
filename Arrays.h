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
 * File:   Arrays.h
 * Author: Benjamin J. Land
 *
 * Created on November 27, 2008, 7:22 PM
 */

#ifndef _ARRAYS_H
#define	_ARRAYS_H

#include <cstring>
#include "Object.h"

template <class T>
class Array : public Object {
    friend class ClassFile;
    
private:
    Array() {
    };
    
protected:
    Array(int size) {
        this->elemsz = sizeof(T);
        this->size = size;
        array = new T[size];
        for (int i = 0; i < size; i++) 
            array[i] = 0;
    };
    
public:
    virtual ~Array() {
        delete [] array;
    };
    Array<T>* clone() {
        Array<T>* res = new Array<T>();
        res->type = type;
        res->elemsz = elemsz;
        res->size = size;
        res->array = (T*) new char[size*elemsz];
        memcpy(res->array,array,size*elemsz);
        return res;
    };
    int elemsz;
    int size;
    T *array;
};

#endif	/* _ARRAYS_H */

