#pragma once

#define BIT_SET(val, bitIndex) val |= (1 << (bitIndex))
#define BIT_CLEAR(val, bitIndex) val &= ~(1 << (bitIndex))
#define BIT_TOGGLE(val, bitIndex) val ^= (1 << (bitIndex))
#define BIT_IS_SET(val, bitIndex) (val & (1 << (bitIndex)))