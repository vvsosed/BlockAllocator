#pragma once

#include <chrono>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <string>
#include <functional>

namespace common {
namespace debug {

class TimeProf {
public:
    using ClockType = std::chrono::high_resolution_clock;
    using TimePoint = ClockType::time_point;

    class GuardType {
    public:
        using UPtr = std::unique_ptr<GuardType>;

        GuardType( ClockType::duration& duration )
        : m_duration( duration )
        , m_timePoint( ClockType::now() ) {}

        static inline UPtr create( ClockType::duration& duration, char* memBlock ) {
            UPtr ptr;
            ptr.reset( new( memBlock ) GuardType( duration ) );
            return ptr;
        }

        ~GuardType() {
            m_duration += ( ClockType::now() - m_timePoint );
        }

        inline void operator delete( void* ) {}

    private:
        ClockType::duration& m_duration;
        TimePoint m_timePoint;
    };

    struct GuardInfo {
        char guardMemblock[sizeof( GuardType )];
        ClockType::duration duration = ClockType::duration::zero();
        std::string guardName;

        GuardInfo( std::string&& name )
        : guardName( std::move( name ) ) {}

        inline auto createGuard() {
            return GuardType::create( duration, guardMemblock );
        }
    };
    using GuardId = unsigned int;
    using GuardMap = std::unordered_map<GuardId, GuardInfo>;

    TimeProf( const std::chrono::seconds& printDelay, const TimePoint& now = ClockType::now() )
    : m_printDelay( printDelay )
    , m_printTp( now + printDelay )
    , m_createTp( now ) {
        m_guards.reserve( 20 );
    }

    void inline beginMeasure() {
        m_beginTp = ClockType::now();
    }

    void inline endMeasure() {
        m_cyclesDuration += ( ClockType::now() - m_beginTp );
        ++m_updatesCount;
    }

    void inline tryPrintInfo() {
        if( m_printTp > ClockType::now() ) {
            return;
        }
        printInfo();
        m_printTp = ClockType::now() + m_printDelay;
    }

    void inline printInfo() {
        std::cout << "<---------------------------------------------------" << std::endl;

        auto now = ClockType::now();
        auto totalTime = now - m_createTp;
        auto usefulTimePercent = double( m_cyclesDuration.count() * 100 ) / totalTime.count();
        auto cyclesAvgTime =
            double( std::chrono::duration_cast<std::chrono::milliseconds>( m_cyclesDuration ).count() ) /
            m_updatesCount;

        auto totalTimeSec = std::chrono::duration_cast<std::chrono::seconds>( totalTime );
        auto cyclesDurationSec = std::chrono::duration_cast<std::chrono::seconds>( m_cyclesDuration );
        std::cout << "TotalTime=" << totalTimeSec.count() << "s, CyclesTotalTime=" << cyclesDurationSec.count()
                  << "s, CyclesAvgTime=" << cyclesAvgTime << "ms, Percent=" << usefulTimePercent << std::endl;

        for( const auto& itr : m_guards ) {
            auto percent = double( itr.second.duration.count() * 100 ) / m_cyclesDuration.count();
            std::cout << "GuardName=" << itr.second.guardName << ", Percent=" << percent << std::endl;
        }

        std::cout << "--------------------------------------------------->" << std::endl;
    }

    void inline reset() {
        std::cout << "--------------- reset" << std::endl;
        auto now = ClockType::now();
        m_printTp = now + m_printDelay;
        m_createTp = now;
        m_cyclesDuration = decltype( m_cyclesDuration )::zero();
        m_updatesCount = 0;
    }

    GuardId initGuard( std::string&& guardName ) {
        auto id = std::hash<std::string>{}( guardName );
        m_guards.emplace( id, GuardInfo( std::move( guardName ) ) );
        return id;
    }

    inline GuardType::UPtr getGuard( const GuardId& id ) {
        const auto itr = m_guards.find( id );
        return itr->second.createGuard();
    }

private:
    std::chrono::seconds m_printDelay;
    TimePoint m_printTp;

    TimePoint m_createTp;
    TimePoint m_beginTp;

    ClockType::duration m_cyclesDuration = ClockType::duration::zero();
    unsigned long long m_updatesCount = 0;

    GuardMap m_guards;
};

}  // namespace debug
}  // namespace common
