
#pragma once

#include "Word.h"

#define WORD_NUM_DIMENSIONS		784
#define DATA_NUM_DIMENSIONS		1
#define RANGE_BIT_LEN			4
#define QUANTIZATION_LEVELS		16

#if MANHATTAN_DISTANCE
#define RADIUS					2933
#else
#define RADIUS					185
#endif

#define NUM_HARD_LOC			1'000'000
#define TRAINING_SET_LIMIT		60'000
