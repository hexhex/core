/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005-2007 Roman Schindlauer
 * Copyright (C) 2006-2015 Thomas Krennwallner
 * Copyright (C) 2009-2015 Peter Schüller
 * Copyright (C) 2011-2015 Christoph Redl
 *
 * This file is part of dlvhex.
 *
 * dlvhex is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * dlvhex is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with dlvhex; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA.
 */

/**
 * @file   Benchmarking.h
 * @author Peter Schller
 * @date   Sat Nov  5 15:26:18 CET 2005
 *
 * @brief  Benchmarking features.
 *
 */
#ifndef DLVHEX_H_BENCHMARKING_INCLUDED_1555
#define DLVHEX_H_BENCHMARKING_INCLUDED_1555

#include "dlvhex2/PlatformDefinitions.h"

#include <boost/scope_exit.hpp>
                                 // seems to be required for scope_exit
#include <boost/typeof/typeof.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/mutex.hpp>
#include <vector>
#include <ostream>

// Benchmarking is always compiled into dlvhex,
// but benchmarking of dlvhex itself is only
// activated if you configure with --enable-debug
//
// Plugins can use benchmarking of dlvhex by doing
// #define DLVHEX_BENCHMARK
// #include <dlvhex2/Benchmarking.h>
// in a .cpp file that wants to use benchmarking.
//
// usage example:
//
// DLVHEX_BENCHMARK_REGISTER(sid1,"calling dlv");
// DLVHEX_BENCHMARK_REGISTER(sid2,"fork+exec dlv");
// DLVHEX_BENCHMARK_REGISTER(sid3,"parse dlv result");
//
// DLVHEX_BENCHMARK_START(sid1)
//   DLVHEX_BENCHMARK_START(sid2)
//   // fork and exec
//   DLVHEX_BENCHMARK_STOP(sid2)
//
//   {
//      DLVHEX_BENCHMARK_SCOPE(sid3)
//      parse result
//      ...
//   }
// DLVHEX_BENCHMARK_STOP(sid1)
// DLVHEX_BENCHMARK_COUNT(sid4,someinteger)
// DLVHEX_BENCHMARK_REGISTER_AND_START(sid6,"reg start")
// {
//   DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid5,"reg scope")
//   ...
// }
// DLVHEX_BENCHMARK_STOP(sid6)
//
// if you want to have a benchmark scope within a template, you need to use
// DLVHEX_BENCHMARK_REGISTER_AND_SCOPE_TPL(sid7,"reg scope in template")
//
// you can also manage the stat IDs yourself
// (e.g., for creating one instrumentation per custom external atom,
//  not only one for some base class)
// #if defined(DLVHEX_BENCHMARK)
//   dlvhex::benchmark::ID myStoredSid =
//     dlvhex::benchmark::BenchmarkController::Instance().getInstrumentationID("my message");
// #endif
//
// invalidate(sid) and INVALIDATE is used to abort instrumentations that were started but should not be counted
// (e.g., if there is no model, we will not have a time to first model)
// invalidating a non-running counter does nothing
//
// there are two benchmark controllers that can be used:
// * simple: (#define DLVHEX_BENCHMARK_SIMPLE)
//   + fast and simple
//   - counts overlapping instrumentalizations twice
//     (not if they have the same name)
//   - gives less intuitive timing results
// * nesting-aware: (#define DLVHEX_BENCHMARK_NESTINGAWARE)
//   - a bit more complex
//   + counts "pure" time where an instrumentalization
//     was the last activated one (uses a stack of instrumentalizations)
//   + gives more intuitive timing results

//#define DLVHEX_BENCHMARK_SIMPLE
#define DLVHEX_BENCHMARK_NESTINGAWARE

#if defined(DLVHEX_BENCHMARK)
# define DLVHEX_BENCHMARK_REGISTER(sid,msg) \
    static DLVHEX_NAMESPACE benchmark::ID sid = DLVHEX_NAMESPACE benchmark::BenchmarkController::Instance().getInstrumentationID(msg)
# define DLVHEX_BENCHMARK_START(sid) \
    DLVHEX_NAMESPACE benchmark::BenchmarkController::Instance().start(sid)
# define DLVHEX_BENCHMARK_STOP(sid) \
    DLVHEX_NAMESPACE benchmark::BenchmarkController::Instance().stop(sid)
# define DLVHEX_BENCHMARK_INVALIDATE(sid) \
    DLVHEX_NAMESPACE benchmark::BenchmarkController::Instance().invalidate(sid)
# define DLVHEX_BENCHMARK_SUSPEND(sid) \
    DLVHEX_NAMESPACE benchmark::BenchmarkController::Instance().stop(sid,false)
# define DLVHEX_BENCHMARK_COUNT(sid,num) \
    DLVHEX_NAMESPACE benchmark::BenchmarkController::Instance().count(sid,num)
# define DLVHEX_BENCHMARK_SCOPE(sid) \
    DLVHEX_NAMESPACE benchmark::BenchmarkController::Instance().start(sid); \
    BOOST_SCOPE_EXIT( (sid) ) \
    { \
        DLVHEX_NAMESPACE benchmark::BenchmarkController::Instance().stop(sid); \
    } BOOST_SCOPE_EXIT_END
# define DLVHEX_BENCHMARK_SUSPEND_SCOPE(sid) \
    DLVHEX_NAMESPACE benchmark::BenchmarkController::Instance().stop(sid,false); \
    BOOST_SCOPE_EXIT( (sid) ) \
    { \
        DLVHEX_NAMESPACE benchmark::BenchmarkController::Instance().start(sid); \
    } BOOST_SCOPE_EXIT_END
# define DLVHEX_BENCHMARK_SCOPE_TPL(sid) \
    DLVHEX_NAMESPACE benchmark::BenchmarkController::Instance().start(sid); \
    BOOST_SCOPE_EXIT_TPL( (sid) ) \
    { \
        DLVHEX_NAMESPACE benchmark::BenchmarkController::Instance().stop(sid); \
    } BOOST_SCOPE_EXIT_END
# define DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,msg) \
    DLVHEX_BENCHMARK_REGISTER(sid,msg); DLVHEX_BENCHMARK_SCOPE(sid);
# define DLVHEX_BENCHMARK_REGISTER_AND_SCOPE_TPL(sid,msg) \
    DLVHEX_BENCHMARK_REGISTER(sid,msg); DLVHEX_BENCHMARK_SCOPE_TPL(sid);
# define DLVHEX_BENCHMARK_REGISTER_AND_START(sid,msg) \
    DLVHEX_BENCHMARK_REGISTER(sid,msg); DLVHEX_BENCHMARK_START(sid);
# define DLVHEX_BENCHMARK_REGISTER_AND_COUNT(sid,msg,num) \
    DLVHEX_BENCHMARK_REGISTER(sid,msg); DLVHEX_BENCHMARK_COUNT(sid,num);
#else
# define DLVHEX_BENCHMARK_REGISTER(sid,msg)               do { } while(0)
# define DLVHEX_BENCHMARK_START(sid)                      do { } while(0)
# define DLVHEX_BENCHMARK_STOP(sid)                       do { } while(0)
# define DLVHEX_BENCHMARK_INVALIDATE(sid)                 do { } while(0)
# define DLVHEX_BENCHMARK_SUSPEND(sid)                    do { } while(0)
# define DLVHEX_BENCHMARK_SUSPEND_SCOPE(sid)              do { } while(0)
# define DLVHEX_BENCHMARK_COUNT(sid,num)                  do { } while(0)
# define DLVHEX_BENCHMARK_SCOPE(sid)                      do { } while(0)
# define DLVHEX_BENCHMARK_SCOPE_TPL(sid)                  do { } while(0)
# define DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,msg)     do { } while(0)
# define DLVHEX_BENCHMARK_REGISTER_AND_SCOPE_TPL(sid,msg) do { } while(0)
# define DLVHEX_BENCHMARK_REGISTER_AND_START(sid,msg)     do { } while(0)
# define DLVHEX_BENCHMARK_REGISTER_AND_COUNT(sid,msg,num) do { } while(0)
#endif                           // defined(DLVHEX_BENCHMARK)

DLVHEX_NAMESPACE_BEGIN

namespace benchmark
{

    typedef unsigned ID;
    typedef unsigned Count;
    typedef boost::posix_time::ptime Time;
    typedef boost::posix_time::time_duration Duration;

    namespace simple
    {

        /** \brief Supports benchmarking of different components of dlvhex. */
        class DLVHEX_EXPORT BenchmarkController
        {
            public:
                /** \brief Information about a single benchmark value. */
                struct Stat
                {
                    /** \brief Counter name. */
                    std::string name;
                    /** \brief Number of calls. */
                    Count count;
                    /** \brief Nesting level (1 = started once, 2 = started twice, ...) */
                    Count level;
                    /** \brief Number of times the counter was started since it was printed last. */
                    Count prints;
                    /** \brief Timestamp when the counter was started. */
                    Time start;
                    /** \brief Sum of durations the counter was run so far. */
                    Duration duration;
                    /** \brief Flag whether the counter currently runs. */
                    bool running;

                    /** \brief Constructor.
                     * @param name See Stat::name. */
                    Stat(const std::string& name);
                };

            public:
                /** \brief Prints a value in seconds.
                 * @param o Stream to print to.
                 * @param d Duration value.
                 * @param width Width of the output in characters for alignment. */
                inline std::ostream& printInSecs(std::ostream& o, const Duration& d, int width=0) const;

                // comfort formatting
                /** \brief Outputs a count value.
                 * @param identifier Benchmark value to output.
                 * @param width See BenchmarkController::printInSecs. */
                std::string count(const std::string& identifier, int width=0) const;
                /** \brief Outputs a duration value.
                 * @param identifier Benchmark value to output.
                 * @param width See BenchmarkController::printInSecs. */
                std::string duration(const std::string& identifier, int width=0) const;

                /** \brief Print information about stat.
                 * @param st Identifies a benchmark value. */
                                 // inline for performance
                inline void printInformation(const Stat& st);
                /** \brief Print continuous information about stat.
                 * @param st Identifies a benchmark value. */
                inline void printInformationContinous(
                                 // inline for performance
                    Stat& st, const Duration& dur);

            public:
                //
                /** \brief Singleton access.
                 * @return Single instance of BenchmarkController. */
                static BenchmarkController& Instance();

                /** \brief Delete the singleton instance.
                 *
                 * Causes destructor to be called. */
                static void finish();

                /** \brief Destructor.
                 *
                 * Output benchmark results, destruct. */
                ~BenchmarkController();

                //
                // configure
                //

                /** \brief Sets the output stream for printing.
                 * @param o Output stream to use in the following. */
                void setOutput(std::ostream* o);
                /** \brief Amount of accumulated output (default: each call).
                 * @param skip Interval. */
                void setPrintInterval(Count skip);

                //
                // instrumentation points
                //

                /** \brief Get ID or register new one.
                 * @param name Identifier.
                 * @return ID of \p name. */
                ID getInstrumentationID(const std::string& name);
                /** \brief Print only count of ID.
                 * @param out Output stream.
                 * @param id ID whose information is to be printed.
                 * @return \p out. */
                                 // inline for performance
                inline std::ostream& printCount(std::ostream& out, ID id);
                /** \brief Print only duration of ID.
                 * @param out Output stream.
                 * @param id ID whose duration is to be printed. */
                                 // inline for performance
                inline std::ostream& printDuration(std::ostream& out, ID id);
                /** \brief Retrieve Stat of \p id.
                 * @param id ID whose Stat is to be retrieved.
                 * @return Stat of \p id. */
                inline const Stat& getStat(ID id) const { return instrumentations[id]; }

                //
                // record measured things
                //

                // stop and resume benchmarking
                /** \brief Stop all benchmarking temporarily. */
                void suspend();
                /** \brief Resume all benchmarking. */
                void resume();

                // start timer
                /** \brief Start a benchmark.
                 * @param id ID of the benchmark to start. */
                                 // inline for performance
                inline void start(ID id);
                /** \brief Stop and record elapsed time, print stats.
                 *
                 * @param id ID of the benchmark to stop.
                 * @param count If count is false, stop time but do not count (for suspending timer).
                 */
                                 // inline for performance
                inline void stop(ID id, bool count=true);
                /** \brief Record count (no time), print stats.

                 * @param id ID of the benchmark to count.
                 * @param increment Increment the count by this value.
                 */
                                 // inline for performance
                inline void count(ID id, Count increment=1);
                /** \brief Stop and do not record anything.
                 *
                 * If not running, do not do anything.
                 * @param id ID of the benchmark to stop. */
                void invalidate(ID id);

                /** \brief Copy data from one id to another id and call stop() on that other id.
                 *
                 * E.g. do this for several interesting benchmarks at first model.
                 * @param id ID to copy.
                 * @param intoID ID to copy \p id into. */
                void snapshot(ID id, ID intoID);

                /** \brief Copy data from one benchmark to another and call stop() on that other benchmark.
                 *
                 * E.g. do this for several interesting benchmarks at first model.
                 * @param fromstr Benchmark to copy.
                 * @param tostr Benchmark to copy \p fromstr into. */
                void snapshot(const std::string& fromstr, const std::string& tostr);

            private:
                /** \brief Constructor.
                 *
                 * Init, display start of benchmarking. */
                BenchmarkController();

                /** \brief ID of the benchmark which measures the BenchmarkController itself. */
                ID myID;
                /** \brief ID to be used for the next benchmark registered. */
                ID maxID;
                /** \brief Vector of benchmark statistics. */
                std::vector<Stat> instrumentations;
                /** \brief Map from benchmark names to IDs. */
                std::map<std::string, ID> name2id;

                /** \brief Output stream to be used. */
                std::ostream* output;
                /** \brief Counter for skipping benchmark output. */
                Count printSkip;

                /** \brief Mutex for multithreading access. */
                boost::mutex mutex;
                /** \brief Level of suspending benchmarking (0 = not suspended). */
                unsigned sus;
        };

        //inline
        std::ostream& BenchmarkController::printInSecs(std::ostream& out, const Duration& td, int width) const
        {
            long in_secs = (long)td.total_milliseconds();

            long secs = in_secs / 1000;
            long rest = in_secs % 1000;

            out << std::setw(width) << secs << ".";

            if (rest < 10) {
                out << "00";
            }
            else if (rest < 100) {
                out << "0";
            }

            return out << rest;
        }

        // print information about stat
        // inline for performance
        void BenchmarkController::printInformation(const Stat& st) {
            if( output ) {
                (*output) <<
                    "BM:" << std::setw(30) << st.name <<
                    ": count:" << std::setw(8) << st.count;
                (*output) <<
                    " total:";
                printInSecs(*output, st.duration, 4) <<
                    "s avg:";
                if( st.count > 0 ) {
                    printInSecs(*output, st.duration/st.count, 4);
                }
                else {
                    (*output) << "   -.---";
                }
                (*output) << std::endl;
            }
        }

        // print only count of ID
        std::ostream& BenchmarkController::printCount(std::ostream& out, ID id) {
            boost::mutex::scoped_lock lock(mutex);
            Stat& st = instrumentations[id];
            return out << st.count;
        }

        std::ostream& BenchmarkController::printDuration(std::ostream& out, ID id) {
            boost::mutex::scoped_lock lock(mutex);
            Stat& st = instrumentations[id];
            return printInSecs(out, st.duration);
        }

        // print continuous information about stat
        // inline for performance
        void BenchmarkController::printInformationContinous(Stat& st, const Duration& dur) {
            if( st.prints >= printSkip ) {
                st.prints = 0;
                if( output ) {
                    (*output) <<
                        "BM:" << std::setw(30) << st.name <<
                        ": count:" << std::setw(8) << st.count <<
                        " total:";
                    printInSecs(*output, st.duration, 4) << "s" <<
                        " last:";
                    printInSecs(*output, dur, 2) << "s" << ((st.running)?"(runs)":"") << std::endl;
                }
            }
            else {
                st.prints++;
            }
        }

        // inline for performance
        // start timer
        void BenchmarkController::start(ID id) {
            if (sus) return;
            boost::mutex::scoped_lock lock(mutex);
            Stat& st = instrumentations[id];
            if( !st.running ) {
                st.start = boost::posix_time::microsec_clock::local_time();
                st.running = true;
                // running once -> level 1
                st.level = 1;
            }
            else {
                // increase nesting level
                st.level++;
            }
        }

        // inline for performance
        // stop and record elapsed time, print stats
        void BenchmarkController::stop(ID id, bool count) {
            if (sus) return;
            boost::mutex::scoped_lock lock(mutex);
            Stat& st = instrumentations[id];

            if( st.running && st.level == 1 ) {
                Duration dur = boost::posix_time::microsec_clock::local_time() - st.start;
                st.duration += dur;
                st.running = false;
                if( count ) {
                    st.count++;
                    printInformationContinous(st,dur);
                }
            }
            else if( st.running ) {
                // decrease nesting level
                st.level--;
            }
        }

        // record count (no time), print stats
        // inline for performance
        void BenchmarkController::count(ID id, Count increment) {
            if (sus) return;
            boost::mutex::scoped_lock lock(mutex);
            Stat& s = instrumentations[id];
            s.count += increment;
            // only count how often we count, otherwise we might spam
            s.prints ++;
            printInformationContinous(s,Duration());
        }

    }                            // namespace simple

    namespace nestingAware
    {

        /** \brief Supports benchmarking of different components of dlvhex. */
        class DLVHEX_EXPORT NestingAwareController
        {
            public:
                /** \brief Information about a single benchmark value. */
                struct Stat
                {
                    /** \brief Counter name. */
                    std::string name;
                    /** \brief Number of calls (outermost). */
                    Count count;
                    /** \brief Sum of durations the stat was run so far. */
                    Duration duration;
                    /** \brief Sum of durations the stat was run without overlay by a subsequently started stat. */
                    Duration pureDuration;
                    /** \brief Level of nesting of this benchmark (0 = not running) */
                    Count level;
                    /** \brief Timepoint after which this stats should be printed again */
                    Time nextPrint;

                    /** \brief Constructor.
                     * @param name See Stat::name. */
                    Stat(const std::string& name, Duration printInterval);
                };
                struct Current
                {
                    /** \brief instrumentation ID of this record */
                    ID which;
                    /** \brief Timestamp when the counter was started first for this Current record. */
                    Time firststart;
                    /** \brief Timestamp when the counter was started most recently. */
                    Time start;

                    Current(ID which);
                };

            public:
                /** \brief Prints a value in seconds.
                 * @param o Stream to print to.
                 * @param d Duration value.
                 * @param width Width of the output in characters for alignment. */
                inline std::ostream& printInSecs(std::ostream& o, const Duration& d, int width=0) const;

                // comfort formatting
                /** \brief Outputs a count value.
                 * @param identifier Benchmark value to output.
                 * @param width See NestingAwareController::printInSecs. */
                std::string count(const std::string& identifier, int width=0) const;
                /** \brief Outputs a duration value.
                 * @param identifier Benchmark value to output.
                 * @param width See NestingAwareController::printInSecs. */
                std::string duration(const std::string& identifier, int width=0) const;

                /** \brief Print information about stat.
                 * @param st Identifies a benchmark value. */
                void printInformation(const Stat& st);
                /** \brief Print continuous information about stat.
                 * @param st Identifies a benchmark value. */
                inline void printInformationContinous(Stat& st, const Duration& dur);

            public:
                //
                /** \brief Singleton access.
                 * @return Single instance of NestingAwareController. */
                static NestingAwareController& Instance();

                /** \brief Delete the singleton instance.
                 *
                 * Causes destructor to be called. */
                static void finish();

                /** \brief Destructor.
                 *
                 * Output benchmark results, destruct. */
                ~NestingAwareController();

                //
                // configure
                //

                /** \brief Sets the output stream for printing.
                 * @param o Output stream to use in the following. */
                void setOutput(std::ostream* o);
                /** \brief Amount of accumulated output (default: each call).
                 * @param skip Interval. */
                void setPrintInterval(Count skip);

                //
                // instrumentation points
                //

                /** \brief Get ID or register new one.
                 * @param name Identifier.
                 * @return ID of \p name. */
                ID getInstrumentationID(const std::string& name);
                /** \brief Print only count of ID.
                 * @param out Output stream.
                 * @param id ID whose information is to be printed.
                 * @return \p out. */
                                 // inline for performance
                inline std::ostream& printCount(std::ostream& out, ID id);
                /** \brief Print only duration of ID.
                 * @param out Output stream.
                 * @param id ID whose duration is to be printed. */
                                 // inline for performance
                inline std::ostream& printDuration(std::ostream& out, ID id);
                /** \brief Retrieve Stat of \p id.
                 * @param id ID whose Stat is to be retrieved.
                 * @return Stat of \p id. */
                inline const Stat& getStat(ID id) const { return instrumentations[id]; }

                //
                // record measured things
                //

                // stop and resume benchmarking
                /** \brief Stop all benchmarking temporarily. */
                void suspend();
                /** \brief Resume all benchmarking. */
                void resume();

                // start timer
                /** \brief Start a benchmark.
                 * @param id ID of the benchmark to start. */
                                 // inline for performance
                inline void start(ID id);
                /** \brief Stop and record elapsed time, print stats.
                 *
                 * @param id ID of the benchmark to stop.
                 * @param count If count is false, stop time but do not count (for suspending timer).
                 */
                                 // inline for performance
                inline void stop(ID id, bool count=true);
                /** \brief Record count (no time), print stats.

                 * @param id ID of the benchmark to count.
                 * @param increment Increment the count by this value.
                 */
                                 // inline for performance
                inline void count(ID id, Count increment=1);
                /** \brief Stop and do not record anything.
                 *
                 * If not running, do not do anything.
                 * @param id ID of the benchmark to stop. */
                void invalidate(ID id);

                /** \brief Copy data from one id to another id and call stop() on that other id.
                 *
                 * E.g. do this for several interesting benchmarks at first model.
                 * @param id ID to copy.
                 * @param intoID ID to copy \p id into. */
                void snapshot(ID id, ID intoID);

                /** \brief Copy data from one benchmark to another and call stop() on that other benchmark.
                 *
                 * E.g. do this for several interesting benchmarks at first model.
                 * @param fromstr Benchmark to copy.
                 * @param tostr Benchmark to copy \p fromstr into. */
                void snapshot(const std::string& fromstr, const std::string& tostr);

            private:
                /** \brief Constructor.
                 *
                 * Init, display start of benchmarking. */
                NestingAwareController();

                /** \brief ID of the benchmark which measures the NestingAwareController itself. */
                ID myID;
                /** \brief ID to be used for the next benchmark registered. */
                ID maxID;
                /** \brief Vector of benchmark statistics. */
                std::vector<Stat> instrumentations;
                /** \brief Map from benchmark names to IDs. */
                std::map<std::string, ID> name2id;
                /** \brief Stack of currently running instrumentations. */
                std::vector<Current> current;

                /** \brief Interval for printing continuous benchmarks. */
                Duration printInterval;
                /** \brief Output stream to be used. */
                std::ostream* output;

                /** \brief Mutex for multithreading access. */
                boost::mutex mutex;
                /** \brief Level of suspending benchmarking (0 = not suspended). */
                unsigned sus;
        };

        //inline
        std::ostream& NestingAwareController::printInSecs(std::ostream& out, const Duration& td, int width) const
        {
            long in_secs = (long)td.total_milliseconds();

            long secs = in_secs / 1000;
            long rest = in_secs % 1000;

            out << std::setw(width) << secs << ".";

            if (rest < 10) {
                out << "00";
            }
            else if (rest < 100) {
                out << "0";
            }

            return out << rest;
        }

        // print only count of ID
        std::ostream& NestingAwareController::printCount(std::ostream& out, ID id) {
            boost::mutex::scoped_lock lock(mutex);
            Stat& st = instrumentations[id];
            return out << st.count;
        }

        std::ostream& NestingAwareController::printDuration(std::ostream& out, ID id) {
            boost::mutex::scoped_lock lock(mutex);
            Stat& st = instrumentations[id];
            return printInSecs(out, st.duration);
        }

        // print continuous information about stat
        // inline for performance
        void NestingAwareController::printInformationContinous(Stat& st, const Duration& dur) {
            if( !output )
                return;
            Time now = boost::posix_time::microsec_clock::local_time();
            if( now > st.nextPrint ) {
                st.nextPrint = now + printInterval;
                (*output) <<
                    "BM:" << std::setw(30) << st.name <<
                    ": count:" << std::setw(8) << st.count <<
                    " total:";
                printInSecs(*output, st.duration, 4) << "s" <<
                    " pure:";
                printInSecs(*output, st.pureDuration, 4) << "s" <<
                    " last:";
                printInSecs(*output, dur, 2) << "s" << std::endl;
            }
        }

        // inline for performance
        // start timer
        void NestingAwareController::start(ID id) {
            boost::mutex::scoped_lock lock(mutex);
            Stat& st = instrumentations[id];

            Time now = boost::posix_time::microsec_clock::local_time();

            st.level++;          // not running = 0, 1 = not nested
            if( !current.empty() ) {
                // suspend previously active instrumentation (and record pure time elapsed)
                Current& prevc = current.back();
                Stat& prevst = instrumentations[prevc.which];
                prevst.pureDuration += now - prevc.start;
            }

            // start new instrumentation
            current.push_back(Current(id));
            Current& c = current.back();
            c.firststart = now;
            c.start = now;
        }

        // inline for performance
        // stop and record elapsed time, print stats
        void NestingAwareController::stop(ID id, bool count) {
            boost::mutex::scoped_lock lock(mutex);
            Stat& st = instrumentations[id];

            Time now = boost::posix_time::microsec_clock::local_time();

            // currently active instrumentation
            Current& c = current.back();
            if( id != c.which )
                // warn but ignore
                std::cerr << "inconsistency: instrumentation '" << st.name <<
                    "' currently running but got stop() for '" << instrumentations[id].name <<
                    "'! (might cause subsequent errors)" << std::endl;

            // update pure duration
            st.pureDuration += now - c.start;

            // update overall duration, but only if not nested with itself
            // (this way neither pure nor overall duration is counted twice)
            Duration thisDuration;
            if( st.level == 1 ) {
                // the continuously logged duration must be non-pure, otherwise we need
                // to separately accumulate pureDuration since most recent start()
                thisDuration = now - c.firststart;
                st.duration += thisDuration;
            }
            if( count ) {
                st.count++;
                printInformationContinous(st, thisDuration);
            }

            // remove current instrumentation
            current.pop_back();
            st.level--;

            // leave function if there is no higher-level instrumentation running
            if( current.empty() )
                return;

            // resume previous instrumentation
            Current& prevc = current.back();
            prevc.start = now;
        }

        // record count (no time), print stats
        // inline for performance
        void NestingAwareController::count(ID id, Count increment) {
            boost::mutex::scoped_lock lock(mutex);
            Stat& s = instrumentations[id];
            s.count += increment;
            printInformationContinous(s,Duration());
        }

    }                            // namespace nestingAware

    #if defined(DLVHEX_BENCHMARK_SIMPLE)
    typedef simple::BenchmarkController BenchmarkController;
    #elif defined(DLVHEX_BENCHMARK_NESTINGAWARE)
    typedef nestingAware::NestingAwareController BenchmarkController;
    #else
    # error Benchmarking is defined but neither SIMPLE nor NESTINGAWARE is chosen!
    #endif

}                                // namespace benchmark


DLVHEX_NAMESPACE_END
#endif                           // DLVHEX_H_BENCHMARKING_INCLUDED_1555


// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
