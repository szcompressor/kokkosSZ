//  Created by JianNan Tian on 6/8/19.
//  Copyright © 2019 JianNan Tian. All rights reserved.
//

#include "constants.hh"
#include <cstddef>

const size_t DIM0   = 0;
const size_t DIM1   = 1;
const size_t DIM2   = 2;
const size_t DIM3   = 3;
const size_t nBLK0  = 4;
const size_t nBLK1  = 5;
const size_t nBLK2  = 6;
const size_t nBLK3  = 7;
const size_t nDIM   = 8;
const size_t LEN    = 12;
const size_t CAP    = 13;
const size_t RADIUS = 14;

const size_t EB     = 0;
const size_t EBr    = 1;
const size_t EBx2   = 2;
const size_t EBx2_r = 3;

#if defined(_1D64)
const int B_1d = 64;
#elif defined(_1D128)
const int B_1d = 128;
#else
const int B_1d = 32;
#endif
const int B_2d = 16;
const int B_3d = 8;
