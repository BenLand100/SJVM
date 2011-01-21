#include "Jar.h"
#include "Inflate.h"
#include "ClassLoader.h"

#include <iostream>
#include <fstream>
#include <map>
#include <cstring>

//#define DEBUG_OUTPUT

#ifdef DEBUG_OUTPUT
#define debug(v) std::cout << v;
#else
#define debug(v)
#endif

JarFile::JarFile(char* fpath) : path(fpath) {
    open();
}

JarFile::~JarFile() {
    for (std::map<std::string, FileHeader*>::iterator iter = files.begin(); iter != files.end(); iter++) {
        FileHeader *info = iter->second;
        if (info->fname_len > 0) delete [] info->file_name;
        delete info;
    }
    files.clear();
}

std::string JarFile::getPath() {
    return path;
}

int JarFile::open() {
    std::ifstream file(path.c_str(), std::ios::in | std::ios::binary | std::ios::ate);
    if (!file.is_open())
        return -1;
    int size = file.tellg();
    unsigned char *data = new unsigned char[size];
    file.seekg(0, std::ios::beg);
    file.read((char*) data, size);
    file.close();
    if (*(unsigned int*) data != 0x04034b50) {
        return -1;
        delete[] data;
    }
    readCentralDirectory(data, size);
    delete[] data;
    return 0;
}

void JarFile::readCentralDirectory(unsigned char * data, long len) {
    unsigned char * tmp = data;
    unsigned long int * tmp2 = (unsigned long int *) tmp;
    len -= 4;
    while ((*tmp2) != 0x02014b50 && len) {
        tmp++;
        tmp2 = (unsigned long int *) tmp;
        len--;
    }
    int size;
    do {
        FileHeader fhdr;
        size = readFileHeader(tmp, &fhdr);
        if (size) {
            if (files.find(fhdr.file_name) == files.end()) {
                FileHeader *pfhdr = new FileHeader;
                *pfhdr = fhdr;
                files[fhdr.file_name] = pfhdr;
            } else {
                if (fhdr.fname_len > 0) delete [] fhdr.file_name;
            }
        }
        tmp += size;
    } while (size != 0);
}

int JarFile::readFileHeader(unsigned char * data, FileHeader * hdr) {
#define U1(buf,o) buf[o]
#define U2(buf,o) (buf[o+1] << 8) | buf[o]
#define U3(buf,o) (buf[o+2] << 16) | (buf[o+1] << 8) | buf[o]
#define U4(buf,o) (buf[o+3] << 24) | (buf[o+2] << 16) | (buf[o+1] << 8) | buf[o]
    unsigned char * origdata = data;
    std::string m_stData, st_temp;
    unsigned int sig = U4(data, 0);
    if (sig != 0x02014b50) return 0;
    hdr->bitflags = U2(data, 8);
    hdr->comp_method = U2(data, 10);
    hdr->comp_size = U4(data, 20);
    hdr->uncompr_size = U4(data, 24);
    hdr->fname_len = U2(data, 28);
    int extralen = U2(data, 30);
    int commentlen = U2(data, 32);
    hdr->relative_offset = U4(data, 42);
    data += 46;
    if (hdr->fname_len > 0) {
        char *fn;
        fn = new char[hdr->fname_len + 1];
        strncpy(fn, (char*) data, hdr->fname_len);
        fn[hdr->fname_len] = '\0';
        hdr->file_name = fn;
    }
    return (data + hdr->fname_len + commentlen + extralen - origdata);
}

int JarFile::request(const char* name, unsigned char *&data) {
    std::string sname(name);
    sname.append(".class");
    if (files.find(sname) == files.end()) {
        data = 0;
        return -1;
    }
    FileHeader *header = files[sname];
    int offset = header->relative_offset;
    int cprSize = header->comp_size;
    if (cprSize == 0) {
        data = new unsigned char[4]; //Give it something
        return 0; //essentially a success
    }
    int ucprSize = header->uncompr_size;
    debug(name << ' ' << offset << ' ' << cprSize << ' ' << ucprSize << '\n');
    unsigned char *buffer = new unsigned char[cprSize];
    std::ifstream file(path.c_str(), std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        data = 0;
        return -1;
    }
    debug("grabbing data");
    unsigned short int extralen, namelen;
    file.seekg(header->relative_offset + 26, std::ios::beg);
    file.read((char*) & namelen, 2);
    file.read((char*) & extralen, 2);
    debug(' ' << extralen << ' ' << namelen << '\n');
    file.seekg(namelen + extralen, std::ios::cur);
    file.read((char*) buffer, cprSize);
    file.close();
    switch (header->comp_method) {
        case COMPRESSION_STORED:
            data = buffer;
            return 0;
        case COMPRESSION_DEFLATED:
            data = new unsigned char[ucprSize];
            if (inflate_oneshot(buffer, cprSize, data, ucprSize) == 0) {
                delete [] buffer;
                return 0;
            }
        case COMPRESSION_SHRUNK:
        case COMPRESSION_REDUCED1:
        case COMPRESSION_REDUCED2:
        case COMPRESSION_REDUCED3:
        case COMPRESSION_REDUCED4:
        case COMPRESSION_IMPLODED:
        case COMPRESSION_TOKENIZED:
        default:
            delete [] buffer;
            return -1;
    }
}

void JarFile::release(unsigned char *data) {
    delete[] data;
}

