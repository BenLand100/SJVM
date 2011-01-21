/* 
 * File:   Inflate.h
 * Author: Benjamin J. Land
 *
 * Created on December 4, 2008, 7:28 PM
 */

#ifndef _INFLATE_H
#define	_INFLATE_H

#ifdef __cplusplus
#include <cstring>
extern "C" {
#endif

struct _huft;

typedef struct {
	unsigned char*	slide;
	unsigned 	hufts;		/* track memory usage */
	struct _huft*	fixed_tl;
	struct _huft*	fixed_td;
	int		fixed_bl;
	int		fixed_bd;
	unsigned	wp;		/* current position in slide */
	unsigned int 	bb;		/* bit buffer */
	unsigned	bk;		/* bits in bit buffer */
	unsigned char*	inbuf;
	int		insz;
	unsigned char*	outbuf;
	int		outsz;
} inflateInfo;

inflateInfo* inflate_new(void);
int inflate_free(inflateInfo*);
int inflate_oneshot(unsigned char*, int, unsigned char*, int);

#ifdef __cplusplus
};
#endif

#endif	/* _INFLATE_H */

