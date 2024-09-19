//
// Created by ajl on 2021/7/14.
//select from https://github.com/aizvorski/h264bitstream
//

#ifndef HG_RTP_APP_UE_FUN_H
#define HG_RTP_APP_UE_FUN_H

//Table 7-1 NAL unit type codes
#define HG_NAL_UNIT_TYPE_UNSPECIFIED                    0    // Unspecified
#define HG_NAL_UNIT_TYPE_CODED_SLICE_NON_IDR            1    // Coded slice of a non-IDR picture
#define HG_NAL_UNIT_TYPE_CODED_SLICE_DATA_PARTITION_A   2    // Coded slice data partition A
#define HG_NAL_UNIT_TYPE_CODED_SLICE_DATA_PARTITION_B   3    // Coded slice data partition B
#define HG_NAL_UNIT_TYPE_CODED_SLICE_DATA_PARTITION_C   4    // Coded slice data partition C
#define HG_NAL_UNIT_TYPE_CODED_SLICE_IDR                5    // Coded slice of an IDR picture
#define HG_NAL_UNIT_TYPE_SEI                            6    // Supplemental enhancement information (SEI)
#define HG_NAL_UNIT_TYPE_SPS                            7    // Sequence parameter set
#define HG_NAL_UNIT_TYPE_PPS                            8    // Picture parameter set
#define HG_NAL_UNIT_TYPE_AUD                            9    // Access unit delimiter
#define HG_NAL_UNIT_TYPE_END_OF_SEQUENCE               10    // End of sequence
#define HG_NAL_UNIT_TYPE_END_OF_STREAM                 11    // End of stream
#define HG_NAL_UNIT_TYPE_FILLER                        12    // Filler data
#define HG_NAL_UNIT_TYPE_SPS_EXT                       13    // Sequence parameter set extension
#define HG_NAL_UNIT_TYPE_PREFIX_NAL                    14    // Prefix NAL unit
#define HG_NAL_UNIT_TYPE_SUBSET_SPS                    15    // Subset Sequence parameter set
#define HG_NAL_UNIT_TYPE_DPS                           16    // Depth Parameter Set
// 17..18    // Reserved
#define HG_NAL_UNIT_TYPE_CODED_SLICE_AUX               19    // Coded slice of an auxiliary coded picture without partitioning
#define HG_NAL_UNIT_TYPE_CODED_SLICE_SVC_EXTENSION     20    // Coded slice of SVC extension
// 20..23    // Reserved
// 24..31    // Unspecified

#define HG_SH_SLICE_TYPE_P        0        // P (P slice)
#define HG_SH_SLICE_TYPE_B        1        // B (B slice)
#define HG_SH_SLICE_TYPE_I        2        // I (I slice)
#define HG_SH_SLICE_TYPE_EP       0        // EP (EP slice)
#define HG_SH_SLICE_TYPE_EB       1        // EB (EB slice)
#define HG_SH_SLICE_TYPE_EI       2        // EI (EI slice)
#define HG_SH_SLICE_TYPE_SP       3        // SP (SP slice)
#define HG_SH_SLICE_TYPE_SI       4        // SI (SI slice)
#ifdef __cplusplus
extern "C"{
#endif
typedef struct {
    uint8_t *start;
    uint8_t *p;
    uint8_t *end;
    int bits_left;
} bs_t;

const uint8_t nal_pict_slice_type[5] = {HG_SH_SLICE_TYPE_P, HG_SH_SLICE_TYPE_B, HG_SH_SLICE_TYPE_I,
                                        HG_SH_SLICE_TYPE_SP, HG_SH_SLICE_TYPE_SI};

static bs_t* bs_init(bs_t* b, uint8_t* buf, size_t size);
static bs_t* bs_new(uint8_t* buf, size_t size);
static uint32_t bs_read_u(bs_t* b, int n);
static uint32_t bs_read_u1(bs_t* b);
static int bs_eof(bs_t* b);
static inline void bs_skip_u1(bs_t* b)
{
    b->bits_left--;
    if (b->bits_left == 0) { b->p ++; b->bits_left = 8; }
}
static inline void bs_skip_u(bs_t* b, int n)
{
    int i;
    for ( i = 0; i < n; i++ )
    {
        bs_skip_u1( b );
    }
}

static inline bs_t* bs_init(bs_t* b, uint8_t* buf, size_t size)
{
    b->start = buf;
    b->p = buf;
    b->end = buf + size;
    b->bits_left = 8;
    return b;
}
static inline bs_t* bs_new(uint8_t* buf, size_t size)
{
    bs_t* b = (bs_t*)malloc(sizeof(bs_t));
    bs_init(b, buf, size);
    return b;
}
static inline uint32_t bs_read_u(bs_t* b, int n)
{
    uint32_t r = 0;
    int i;
    for (i = 0; i < n; i++)
    {
        r |= ( bs_read_u1(b) << ( n - i - 1 ) );
    }
    return r;
}
static inline uint32_t bs_read_u1(bs_t* b)
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
static inline int bs_eof(bs_t* b) { if (b->p >= b->end) { return 1; } else { return 0; } }
static inline uint32_t bs_read_ue(bs_t* b)
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
#ifdef __cplusplus
}
#endif

#endif //HG_RTP_APP_UE_FUN_H
