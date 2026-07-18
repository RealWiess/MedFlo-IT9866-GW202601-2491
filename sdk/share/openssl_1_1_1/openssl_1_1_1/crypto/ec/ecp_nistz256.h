// ecp_nistz256.h
#ifndef ECP_NISTZ256_H
#define ECP_NISTZ256_H

#if BN_BITS2 != 64
# define TOBN(hi,lo)    lo,hi
#else
# define TOBN(hi,lo)    ((BN_ULONG)hi<<32|lo)
#endif

#define P256_LIMBS (256/BN_BITS2)

typedef struct {
    BN_ULONG X[P256_LIMBS];
    BN_ULONG Y[P256_LIMBS];
    BN_ULONG Z[P256_LIMBS];
} P256_POINT;

// 新增仿射坐標點結構定義
typedef struct {
    BN_ULONG X[P256_LIMBS];
    BN_ULONG Y[P256_LIMBS];
} P256_POINT_AFFINE;

#endif // ECP_NISTZ256_H
