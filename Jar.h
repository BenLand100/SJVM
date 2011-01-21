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
 * File:   Zip.h
 * Author: Benjamin J. Land
 *
 * Created on December 3, 2008, 5:10 PM
 */

#ifndef _ZIP_H
#define	_ZIP_H

#include <string>
#include <map>

class ClassLoader;

#define COMPRESSION_STORED	0
#define COMPRESSION_SHRUNK	1
#define COMPRESSION_REDUCED1	2
#define COMPRESSION_REDUCED2	3
#define COMPRESSION_REDUCED3	4
#define COMPRESSION_REDUCED4	5
#define COMPRESSION_IMPLODED	6
#define COMPRESSION_TOKENIZED	7
#define COMPRESSION_DEFLATED	8

typedef struct {
	unsigned short int bitflags;
	unsigned short int comp_method;
	unsigned long int comp_size;
	unsigned long int uncompr_size;
	unsigned short int fname_len;
	unsigned long int relative_offset;
	char* file_name;
} FileHeader;

#include "ClassLoader.h"

class ClassPath;

class JarFile : public ClassPath {
private:
    std::string path;
    std::map<std::string,FileHeader*> files;
public:
    JarFile(char* fpath);
    virtual ~JarFile();

    std::string getPath();
    virtual int request(const char* name, unsigned char *&data);
    virtual void release(unsigned char *data);
    
private:
    int open();
    void readCentralDirectory(unsigned char * data, long len);
    int readFileHeader(unsigned char * data, FileHeader * hdr);

};

#endif	/* _ZIP_H */

