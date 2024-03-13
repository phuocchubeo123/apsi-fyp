/*
 * typedefs.h
 *
 *  Created on: Jul 1, 2014
 *      Author: mzohner
 */

#ifndef TYPEDEFS_H_
#define TYPEDEFS_H_

//#define DEBUG
//#define BATCH
//#define TIMING
//#define AES256_HASH
//#define USE_PIPELINED_AES_NI

#include <iostream>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <vector>
#include <algorithm>
#include <pthread.h>
#include <stdint.h>

using namespace std;

#define ceil_divide(x, y) ((x) > 0? ( ((x) - 1) / (y) )+1 : 0)

#endif /*TYPEDEFS_H_*/