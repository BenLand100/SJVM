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

