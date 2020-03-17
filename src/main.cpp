#include <iostream>
#include <functional>
#include <array>
#include <random>
#include <iterator>
#include <chrono>

#include "BlocksAllocator.h"
#include "TimeProf.h"

// Took this amazing code from https://codereview.stackexchange.com/questions/79612/c-ifying-a-capturing-lambda
namespace
{
	std::default_random_engine generator(std::chrono::steady_clock::now().time_since_epoch().count());
	std::uniform_int_distribution<std::size_t> dist( 1, 200 );

	constexpr auto ARR_SIZE = 4 * 1024;
	std::array<int, ARR_SIZE> blocks;
	std::array<void*, ARR_SIZE> pointers;
} // private namespace


void memoryTest( std::function<void*(std::size_t)> allocMemClbk, std::function<void( void* ptr )> freeMemClbk) {
	for (auto i = 0; i < ARR_SIZE; ++i ) {
		pointers[i] = allocMemClbk(blocks[i]);
	}
	for (auto i = 0; i < ARR_SIZE; ++i ) {
		freeMemClbk(pointers[i]);
	}
}

void* dummyAlloc( const std::size_t _size ){
	return nullptr;
}

bool dummyRelease( void* ptr ) {
	// Nothing to do
}

int main(int argc, char**argv) {
	std::cout << "Start test between BlockAllocator and Malloc" << std::endl;
	common::debug::TimeProf tProf(std::chrono::seconds(10));

	auto baGuardId = tProf.initGuard("BlockA");
	auto stGuardId = tProf.initGuard("Malloc");
	auto dummyGuardId = tProf.initGuard("Dummy");

	while(true) {
		for (auto i = 0; i < ARR_SIZE; ++i ) {
			blocks[i] = dist(generator);
		}

		tProf.beginMeasure();
		{
			auto guard = tProf.getGuard(baGuardId);
			memoryTest(common::blocks_allocator::allocate, common::blocks_allocator::deallocate);
		}
		{
			auto guard = tProf.getGuard(stGuardId);
			memoryTest(std::malloc, std::free);
		}
		{
			auto guard = tProf.getGuard(dummyGuardId);
			memoryTest(dummyAlloc, dummyRelease);
		}
		tProf.endMeasure();

		tProf.tryPrintInfo();
	}

	return 0;
}

