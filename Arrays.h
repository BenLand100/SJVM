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

