/* 
 * parsing primitives.
 */

#define _GNU_SOURCE
#include <string.h>
#include "drv.h"

void mkSlice(struct slice *b, void *base, size_t sz)
{
    b->cur = (u_int8_t *)base;
    b->end = b->cur + sz;
}

unsigned char *sliceBuf(struct slice *b)
{
    return b->cur;
}

size_t sliceSize(struct slice *b)
{
    return b->end - b->cur;
}

int getEOF(struct slice *b)
{
    if(b->cur != b->end)
        return -1;
    return 0;
}

int getU8(struct slice *b, u_int8_t *x)
{
    if(b->cur >= b->end) return -1;
    *x = *b->cur;
    b->cur++;
    return 0;
}

int getU16(struct slice *b, u_int16_t *x)
{
    u_int8_t h, l;
    if(getU8(b, &h) == -1
    || getU8(b, &l) == -1)
        return -1;
    *x = (h << 8) | l;
    return 0;
}

int getU32(struct slice *b, u_int32_t *x)
{
    u_int16_t h, l;
    if(getU16(b, &h) == -1
    || getU16(b, &l) == -1)
        return -1;
    *x = (h << 16) | l;
    return 0;
}

int getU64(struct slice *b, u_int64_t *x)
{
    u_int32_t h, l;
    if(getU32(b, &h) == -1
    || getU32(b, &l) == -1)
        return -1;
    *x = ((u_int64_t)h << 32) | l;
    return 0;
}


/* split a slice up into up to max slices */
int getDelimSlices(struct slice *b, char *delim, int delsz, size_t max, struct slice *x, size_t *nx)
{
    unsigned char *ep;
    size_t i;

    for(i = 0; i < max && b->cur != b->end; i++) {
        ep = memmem(b->cur, b->end - b->cur, delim, delsz);
        x[i].cur = b->cur;
        if(ep) {
            b->cur = ep + delsz;
        } else {
            b->cur = b->end;
            ep = b->end;
        }
        x[i].end = ep;
    }

    if(b->cur != b->end)
        return -1;
    *nx = i;
    return 0;
}
