#ifndef SHADERGEN_H
#define SHADERGEN_H

#include <vector>
#include <stdint.h>
#include <stddef.h>

std::vector<uint8_t> gen_fragshader(const char* code, size_t len);

#endif
