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

