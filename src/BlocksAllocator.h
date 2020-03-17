#pragma once

#include <cstddef>

namespace common {
namespace blocks_allocator {

typedef void* BufPtr;

BufPtr allocate( const size_t _size );

bool deallocate( BufPtr ptr );

BufPtr reallocate( BufPtr ptr, const size_t size );

size_t calculateRealSize( BufPtr ptr );

void printStat();

}  // namespace blocks_allocator
}  // namespace common
