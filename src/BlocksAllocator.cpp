#include "BlocksAllocator.h"

#include <mutex>
#include <string>
#include <cstdlib>
#include <cstring>
#include <array>
#include <sstream>

#define COLLECT_STAT_INFO

#ifdef COLLECT_STAT_INFO
#include <iostream>
#include <iomanip>
#endif

namespace common {

namespace blocks_allocator {

namespace {
#ifdef COLLECT_STAT_INFO
template <typename Pt1, typename Pt2, typename Pt3, typename Pt4, typename Pt5, typename Pt6>
void table_print( const unsigned _field_width,
                  const Pt1& _pt1,
                  const Pt2& _pt2,
                  const Pt3& _pt3,
                  const Pt4& _pt4,
                  const Pt5& _pt5,
                  const Pt6& _pt6 ) {
    std::cout << std::setiosflags( std::ios::fixed ) << std::setw( _field_width ) << std::left << _pt1
              << std::setw( _field_width ) << std::left << _pt2 << std::setw( _field_width ) << std::left << _pt3
              << std::setw( _field_width ) << std::left << _pt4 << std::setw( _field_width ) << std::left << _pt5
              << std::setw( _field_width ) << std::left << _pt6 << std::endl;
}

struct StatInfoData {
    unsigned total_allocs_count;
    unsigned total_free_count;
    unsigned max_sim_allocs_count;
    unsigned max_alloc_size;
    unsigned min_alloc_size;

    StatInfoData()
    : total_allocs_count( 0 )
    , total_free_count( 0 )
    , max_sim_allocs_count( 0 )
    , max_alloc_size( 0 )
    , min_alloc_size( 0 ) {}

    template <typename _Buf>
    void onAlloc( const _Buf& _buf, const size_t _size ) {
        total_allocs_count += 1;
        if( max_sim_allocs_count < _buf.used_count )
            max_sim_allocs_count = _buf.used_count;
        if( max_alloc_size < _size )
            max_alloc_size = _size;
        if( min_alloc_size > _size || 0 == min_alloc_size )
            min_alloc_size = _size;
    }

    template <typename _Buf>
    void onFree( const _Buf& _buf ) {
        total_free_count += 1;
    }

    template <typename _Buf>
    void printStat( const unsigned _filed_width, const _Buf& _buf ) {
        table_print( _filed_width,
                     std::string( _buf ),
                     total_allocs_count,
                     total_free_count,
                     max_sim_allocs_count,
                     max_alloc_size,
                     min_alloc_size );
    }
};
#endif

struct BufArrayBase {
    typedef std::mutex Mutex;
    typedef std::lock_guard<Mutex> LockGuard;

    size_t used_count;
    Mutex mutex;
#ifdef COLLECT_STAT_INFO
    StatInfoData stat_info_data;
#endif

    BufArrayBase()
    : used_count( 0 ) {}

    bool isEmpty() const {
        return 0 == used_count;
    }
};

template <size_t _Size, size_t _Count>
struct StatBufArray : BufArrayBase {
    static constexpr auto Size = _Size;
    static constexpr auto Count = _Count;

    struct Buffer {
        char buf[Size];
        bool is_used;
    };
    std::array<Buffer, Count> buffers;

    StatBufArray();

    BufPtr alloc( const size_t _size );

    bool free( BufPtr _ptr );

    bool isOwner( BufPtr _ptr );

    bool isFull() const {
        return Count == used_count;
    }

    operator std::string() const {
        std::stringstream ss;
        ss << "SB[" << Size << "|" << Count << "]";
        return ss.str();
    }

#ifdef COLLECT_STAT_INFO
    void printStat( const unsigned _field_width ) {
        stat_info_data.printStat( _field_width, *this );
    }
#endif
};

template <size_t _Size, size_t _Count>
StatBufArray<_Size, _Count>::StatBufArray()
: buffers( {} ) {}

template <size_t _Size, size_t _Count>
BufPtr StatBufArray<_Size, _Count>::alloc( const size_t _size ) {
    if( Size < _size || isFull() ) {
        return nullptr;
    }

    LockGuard guard( mutex );
    for( auto& buf : buffers ) {
        if( !buf.is_used ) {
            buf.is_used = true;
            used_count += 1;
#ifdef COLLECT_STAT_INFO
            stat_info_data.onAlloc( *this, _size );
#endif
            return buf.buf;
        }
    }
    return nullptr;
}

template <size_t _Size, size_t _Count>
bool StatBufArray<_Size, _Count>::free( BufPtr _ptr ) {
    if( isEmpty() || buffers[0].buf > _ptr || buffers[Count - 1].buf < _ptr )
        return false;

    LockGuard guard( mutex );
    for( auto& buf : buffers ) {
        if( buf.buf == _ptr ) {
            buf.is_used = false;
            used_count -= 1;
#ifdef COLLECT_STAT_INFO
            stat_info_data.onFree( *this );
#endif
            return true;
        }
    }
    return false;
}

template <size_t _Size, size_t _Count>
bool StatBufArray<_Size, _Count>::isOwner( BufPtr _ptr ) {
    if( isEmpty() || buffers[0].buf > _ptr || buffers[Count - 1].buf < _ptr )
        return false;

    for( auto& buf : buffers ) {
        if( buf.buf == _ptr )
            return true;
    }
    return false;
}

template <size_t _Size, size_t _Count>
struct MallocBufArray : BufArrayBase {
    static constexpr auto Size = _Size;
    static constexpr auto Count = _Count;

    std::array<BufPtr, Count> buffers;

    MallocBufArray();

    BufPtr alloc( const size_t _size );

    bool free( BufPtr _ptr );

    bool isOwner( BufPtr _ptr );

    bool isFull() const {
        return Count == used_count;
    }

    operator std::string() const {
        std::stringstream ss;
        ss << "MB[" << Size << "|" << Count << "]";
        return ss.str();
    }

#ifdef COLLECT_STAT_INFO
    void printStat( const unsigned _field_width ) {
        stat_info_data.printStat( _field_width, *this );
    }
#endif
};

template <size_t _Size, size_t _Count>
MallocBufArray<_Size, _Count>::MallocBufArray()
: buffers( {} ) {}

template <size_t _Size, size_t _Count>
BufPtr MallocBufArray<_Size, _Count>::alloc( const size_t _size ) {
    if( Size < _size || isFull() ) {
        return nullptr;
    }

    LockGuard guard( mutex );
    for( auto& buf : buffers ) {
        if( !buf ) {
            buf = static_cast<BufPtr>( std::malloc( Size ) );
            used_count += 1;
#ifdef COLLECT_STAT_INFO
            stat_info_data.onAlloc( *this, _size );
#endif
            return buf;
        }
    }
    return nullptr;
}

template <size_t _Size, size_t _Count>
bool MallocBufArray<_Size, _Count>::free( BufPtr _ptr ) {
    if( isEmpty() )
        return false;

    LockGuard guard( mutex );
    for( auto& buf : buffers ) {
        if( buf == _ptr ) {
            std::free( _ptr );
            buf = nullptr;
            used_count -= 1;
#ifdef COLLECT_STAT_INFO
            stat_info_data.onFree( *this );
#endif
            return true;
        }
    }
    return false;
}

template <size_t _Size, size_t _Count>
bool MallocBufArray<_Size, _Count>::isOwner( BufPtr _ptr ) {
    if( isEmpty() )
        return false;

    for( auto& buf : buffers ) {
        if( buf == _ptr )
            return true;
    }
    return false;
}

template <typename... Ts>
struct Buffers {};

template <typename _BufArray>
struct Buffers<_BufArray> {
    _BufArray buf_array;

    BufPtr alloc( const size_t _size ) {
        return buf_array.alloc( _size );
    }

    bool free( BufPtr _ptr ) {
        return buf_array.free( _ptr );
    }

    size_t promoteSize( BufPtr _buf ) {
        return buf_array.isOwner( _buf ) ? buf_array.Size : 0;
    }

#ifdef COLLECT_STAT_INFO
    void printStat( const unsigned _field_width ) {
        buf_array.printStat( _field_width );
    }
#endif
};

template <typename _BufArray, typename... Ts>
struct Buffers<_BufArray, Ts...> : Buffers<Ts...> {
    typedef Buffers<Ts...> Tails;

    _BufArray buf_array;

    BufPtr alloc( const size_t _size ) {
        BufPtr ptr = buf_array.alloc( _size );
        return ptr ? ptr : Tails::alloc( _size );
    }

    bool free( BufPtr _ptr ) {
        return buf_array.free( _ptr ) || Tails::free( _ptr );
    }

    size_t promoteSize( BufPtr _buf ) {
        return buf_array.isOwner( _buf ) ? buf_array.Size : Tails::promoteSize( _buf );
    }

#ifdef COLLECT_STAT_INFO
    void printStat( const unsigned _field_width ) {
        buf_array.printStat( _field_width );
        Tails::printStat( _field_width );
    }
#endif
};

typedef StatBufArray<16, 64> SB_16;
typedef StatBufArray<32, 64> SB_32;
typedef StatBufArray<64, 64> SB_64;
typedef StatBufArray<128, 16> SB_128;
typedef StatBufArray<256, 8> SB_256;
typedef StatBufArray<512, 4> SB_512;
typedef MallocBufArray<768, 16> MB_768;
typedef MallocBufArray<1024, 16> MB_1024;
typedef MallocBufArray<2048, 16> MB_2048;
typedef MallocBufArray<4096, 16> MB_4096;
typedef MallocBufArray<8192, 16> MB_8192;
typedef MallocBufArray<12288, 16> MB_12288;

Buffers<SB_16, SB_32, SB_64, SB_128, SB_256, SB_512, MB_768, MB_1024, MB_2048, MB_4096, MB_8192, MB_12288> buffers;
}  // namespace

BufPtr allocate( const size_t _size ) {
    return buffers.alloc( _size );
}

bool deallocate( BufPtr _ptr ) {
    return buffers.free( _ptr );
}

BufPtr reallocate( BufPtr ptr, const size_t size ) {
    if( 0 == size ) {
        deallocate( ptr );
        return nullptr;
    }

    if( !ptr ) {
        return allocate( size );
    }

    auto real_size = calculateRealSize( ptr );
    if( real_size >= size ) {
        return ptr;
    }

    auto bigger_chunk = allocate( size );
    if( !bigger_chunk ) {
        return nullptr;
    }
    std::memcpy( bigger_chunk, ptr, real_size );
    deallocate( ptr );
    return bigger_chunk;
}

size_t calculateRealSize( BufPtr _ptr ) {
    return buffers.promoteSize( _ptr );
}

void printStat() {
#ifdef COLLECT_STAT_INFO
    std::cout << "\n********************** Stat from BlocksAllocator ****************************************"
              << std::endl;
    const unsigned field_width = 16;
    table_print( field_width, "Name", "TotAlloc", "TotFree", "SimAlloc", "MaxSize", "MinSize" );
    buffers.printStat( field_width );
    std::cout << "*****************************************************************************************\n"
              << std::endl;
#endif
}

}  // namespace blocks_allocator

}  // namespace common
