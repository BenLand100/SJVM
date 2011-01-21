/* 
 * File:   Native.h
 * Author: Benjamin J. Land
 *
 * Created on November 29, 2008, 8:30 PM
 */

#ifndef _NATIVE_H
#define	_NATIVE_H

#include <map>
#include <string>

class LibraryPool {
private:
    std::map<std::string,void*> libraries;
public:
    LibraryPool();
    ~LibraryPool();
    int loadlib(const char *library);
    void* address(const char *name);
};


#endif	/* _NATIVE_H */

