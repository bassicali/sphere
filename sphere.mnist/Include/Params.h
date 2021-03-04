
#pragma once

#define WORD_NUM_DIMENSIONS		784
#define RANGE_BIT_LEN			4
#define QUANTIZATION_LEVELS		16
#define RADIUS					188

#define TEST_MODE 0

#if TEST_MODE
#define NUM_HARD_LOC			10'000
#define TRAINING_SET_LIMIT		1000
#else
#define NUM_HARD_LOC			1'000'000
#define TRAINING_SET_LIMIT		60'000
#endif