#include "bs.h"
bs_t* bs_init(bs_t* b, uint8_t* buf, size_t size)
{
    b->start = buf;
    b->p = buf;
    b->end = buf + size;
    b->bits_left = 8;
    return b;
}
bs_t* bs_new(uint8_t* buf, size_t size)
{
    bs_t* b = (bs_t*)malloc(sizeof(bs_t));
    bs_init(b, buf, size);
    return b;
}
uint32_t bs_read_u(bs_t* b, int n)
{
    uint32_t r = 0;
    int i;
    for (i = 0; i < n; i++)
    {
        r |= ( bs_read_u1(b) << ( n - i - 1 ) );
    }
    return r;
}
int bs_eof(bs_t* b) { if (b->p >= b->end) { return 1; } else { return 0; } }
uint32_t bs_read_u8(bs_t* b)
{
    //#ifdef FAST_U8
    if (b->bits_left == 8 && ! bs_eof(b)) // can do fast read
    {
        uint32_t r = b->p[0];
        b->p++;
        return r;
    }
    //#endif
    return bs_read_u(b, 8);
}
uint32_t bs_read_u1(bs_t* b)
{
    uint32_t r = 0;

    b->bits_left--;

    if (! bs_eof(b))
    {
        r = ((*(b->p)) >> b->bits_left) & 0x01;
    }

    if (b->bits_left == 0) { b->p ++; b->bits_left = 8; }

    return r;
}
void bs_skip_u(bs_t* b, int n)
{
    int i;
    for ( i = 0; i < n; i++ )
    {
        bs_skip_u1( b );
    }
}
void bs_skip_u1(bs_t* b)
{
    b->bits_left--;
    if (b->bits_left == 0) { b->p ++; b->bits_left = 8; }
}

uint32_t bs_read_ue(bs_t* b)
{
    int32_t r = 0;
    int i = 0;

    while( (bs_read_u1(b) == 0) && (i < 32) && (!bs_eof(b)) )
    {
        i++;
    }
    r = bs_read_u(b, i);
    r += (1 << i) - 1;
    return r;
}
int32_t bs_read_se(bs_t* b)
{
    int32_t r = bs_read_ue(b);
    if (r & 0x01)
    {
        r = (r+1)/2;
    }
    else
    {
        r = -(r/2);
    }
    return r;
}
