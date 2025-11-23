#include "case.h"


namespace Jikes { // Open namespace Jikes block
char Case::lower[128] =
{
      0,   1,   2,   3,   4,   5,   6,   7,   8,   9,
     10,  11,  12,  13,  14,  15,  16,  17,  18,  19,
     20,  21,  22,  23,  24,  25,  26,  27,  28,  29,
     30,  31,  32,  33,  34,  35,  36,  37,  38,  39,
     40,  41,  42,  43,  44,  45,  46,  47,  48,  49,
     50,  51,  52,  53,  54,  55,  56,  57,  58,  59,
     60,  61,  62,  63,  64, U_a, U_b, U_c, U_d, U_e,
    U_f, U_g, U_h, U_i, U_j, U_k, U_l, U_m, U_n, U_o,
    U_p, U_q, U_r, U_s, U_t, U_u, U_v, U_w, U_x, U_y,
    U_z,  91,  92,  93,  94,  95,  96, U_a, U_b, U_c,
    U_d, U_e, U_f, U_g, U_h, U_i, U_j, U_k, U_l, U_m,
    U_n, U_o, U_p, U_q, U_r, U_s, U_t, U_u, U_v, U_w,
    U_x, U_y, U_z, 123, 124, 125, 126, 127
};

char Case::upper[128] =
{
      0,   1,   2,   3,   4,   5,   6,   7,   8,   9,
     10,  11,  12,  13,  14,  15,  16,  17,  18,  19,
     20,  21,  22,  23,  24,  25,  26,  27,  28,  29,
     30,  31,  32,  33,  34,  35,  36,  37,  38,  39,
     40,  41,  42,  43,  44,  45,  46,  47,  48,  49,
     50,  51,  52,  53,  54,  55,  56,  57,  58,  59,
     60,  61,  62,  63,  64, U_A, U_B, U_C, U_D, U_E,
    U_F, U_G, U_H, U_I, U_J, U_K, U_L, U_M, U_N, U_O,
    U_P, U_Q, U_R, U_S, U_T, U_U, U_V, U_W, U_X, U_Y,
    U_Z,  91,  92,  93,  94,  95,  96, U_A, U_B, U_C,
    U_D, U_E, U_F, U_G, U_H, U_I, U_J, U_K, U_L, U_M,
    U_N, U_O, U_P, U_Q, U_R, U_S, U_T, U_U, U_V, U_W,
    U_X, U_Y, U_Z, 123, 124, 125, 126, 127
};


} // Close namespace Jikes block

