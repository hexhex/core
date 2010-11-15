/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Thomas Krennwallner
 * Copyright (C) 2009, 2010 Peter Schüller
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
 * @author Peter Schüller
 * @date   Sat Nov  5 15:26:18 CET 2005
 * 
 * @brief  Benchmarking features.
 * 
 */
#ifndef DLVHEX_H_BENCHMARKING_INCLUDED_1555
#define DLVHEX_H_BENCHMARKING_INCLUDED_1555

#include <dlvhex/PlatformDefinitions.h>
#include <boost/scope_exit.hpp>
#include <boost/typeof/typeof.hpp> // seems to be required for scope_exit
#include <boost/date_time/posix_time/posix_time.hpp>
#include <vector>
#include <ostream>

// Benchmarking is always compiled into dlvhex,
// but benchmarking of dlvhex itself is only
// activated if you configure with --enable-debug
//
// Plugins can use benchmarking of dlvhex by doing
// #define DLVHEX_BENCHMARK
// #include <dlvhex/Benchmarking.h>
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

#if defined(DLVHEX_BENCHMARK)
# define DLVHEX_BENCHMARK_REGISTER(sid,msg) \
    static DLVHEX_NAMESPACE benchmark::ID sid = DLVHEX_NAMESPACE benchmark::BenchmarkController::Instance().getInstrumentationID(msg)
# define DLVHEX_BENCHMARK_START(sid) \
    DLVHEX_NAMESPACE benchmark::BenchmarkController::Instance().start(sid)
# define DLVHEX_BENCHMARK_STOP(sid) \
    DLVHEX_NAMESPACE benchmark::BenchmarkController::Instance().stop(sid)
# define DLVHEX_BENCHMARK_COUNT(sid,num) \
    DLVHEX_NAMESPACE benchmark::BenchmarkController::Instance().count(sid,num)
# define DLVHEX_BENCHMARK_SCOPE(sid) \
    DLVHEX_NAMESPACE benchmark::BenchmarkController::Instance().start(sid); \
    BOOST_SCOPE_EXIT( (sid) ) { \
      DLVHEX_NAMESPACE benchmark::BenchmarkController::Instance().stop(sid); \
    } BOOST_SCOPE_EXIT_END
# define DLVHEX_BENCHMARK_SCOPE_TPL(sid) \
    DLVHEX_NAMESPACE benchmark::BenchmarkController::Instance().start(sid); \
    BOOST_SCOPE_EXIT_TPL( (sid) ) { \
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
# define DLVHEX_BENCHMARK_COUNT(sid,num)                  do { } while(0)
# define DLVHEX_BENCHMARK_SCOPE(sid)                      do { } while(0)
# define DLVHEX_BENCHMARK_SCOPE_TPL(sid)                  do { } while(0)
# define DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,msg)     do { } while(0)
# define DLVHEX_BENCHMARK_REGISTER_AND_SCOPE_TPL(sid,msg) do { } while(0)
# define DLVHEX_BENCHMARK_REGISTER_AND_START(sid,msg)     do { } while(0)
# define DLVHEX_BENCHMARK_REGISTER_AND_COUNT(sid,msg,num) do { } while(0)

#endif // defined(DLVHEX_BENCHMARK)

DLVHEX_NAMESPACE_BEGIN

namespace benchmark
{

typedef unsigned ID;
typedef unsigned Count;
typedef boost::posix_time::ptime Time;
typedef boost::posix_time::time_duration Duration;

class BenchmarkController
{
private:
  // init, display start of benchmarking
  BenchmarkController();

  struct Stat
  {
    std::string name;
    Count count;
    Count prints;
    Time start;
    Duration duration;

    Stat(const std::string& name);
  };

  ID myID;
  ID maxID;
  std::vector<Stat> instrumentations;
  std::map<std::string, ID> name2id;

  std::ostream* output;
  Count printSkip;

  inline std::ostream& printInSecs(std::ostream& o, const Duration& d, int width=0) const;

  // print information about stat
  inline void printInformation(const Stat& st); // inline for performance
  // print continuous information about stat
  inline void printInformationContinous(
    Stat& st, const Duration& dur); // inline for performance
  
public:
  // singleton access
  static BenchmarkController& Instance();

  // delete singleton-> causes destructor to be called
  static void finish();

  // output benchmark results, destruct
  ~BenchmarkController();

  //
  // configure
  //

  // output stream
  void setOutput(std::ostream* o);
  // amount of accumulated output (default: each call)
  void setPrintInterval(Count skip);

  //
  // instrumentation points
  //

  // get ID or register new one
  ID getInstrumentationID(const std::string& name);
  // print information about ID
  inline void printInformation(ID id); // inline for performance

  // 
  // record measured things
  //
  
  // start timer
  inline void start(ID id); // inline for performance
  // stop and record elapsed time, print stats
  inline void stop(ID id); // inline for performance
  // record count (no time), print stats
  inline void count(ID id, Count increment=1); // inline for performance
};

//inline
std::ostream& BenchmarkController::printInSecs(std::ostream& out, const Duration& td, int width) const
{
  long in_secs = td.total_milliseconds();

  long secs = in_secs / 1000;
  long rest = in_secs % 1000;

  out << std::setw(width) << secs << ".";

  if (rest < 10)
    {
      out << "00";
    }
  else if (rest < 100)
    {
      out << "0";
    }

  return out << rest;
}

// print information about stat
// inline for performance
void BenchmarkController::printInformation(const Stat& st)
{
  if( output )
  {
    (*output) <<
      "BM:" << std::setw(30) << st.name <<
      ": count:" << std::setw(6) << st.count <<
      " avg:";
		if( st.count > 0 )
		{
			printInSecs(*output, st.duration/st.count, 4);
		}
		else
		{
			(*output) << "   -.---";
		}
		(*output) << "s" << " total:";
    printInSecs(*output, st.duration, 6) << "s" << std::endl;
  }
}

// print continuous information about stat
// inline for performance
void BenchmarkController::printInformationContinous(Stat& st, const Duration& dur)
{
  if( st.prints >= printSkip )
  {
    st.prints = 0;
    if( output )
    {
      (*output) <<
        "BM:" << std::setw(30) << st.name <<
        ": count:" << std::setw(6) << st.count <<
        " total:";
      printInSecs(*output, st.duration, 6) << "s" <<
        " last:";
      printInSecs(*output, dur, 2) << "s" << std::endl;
    }
  }
  else
  {
    st.prints++;
  }
}

// inline for performance
// start timer
void BenchmarkController::start(ID id)
{
  Stat& st = instrumentations[id];
  st.start = boost::posix_time::microsec_clock::local_time();
}

// inline for performance
// stop and record elapsed time, print stats
void BenchmarkController::stop(ID id)
{
  Stat& st = instrumentations[id];

  Duration dur = boost::posix_time::microsec_clock::local_time() - st.start;
  st.count++;
  st.duration += dur;
  printInformationContinous(st,dur);
}

// record count (no time), print stats
// inline for performance
void BenchmarkController::count(ID id, Count increment)
{
  Stat& s = instrumentations[id];
  s.count += increment;
  s.prints += increment - 1;
  printInformationContinous(s,Duration());
}

} // namespace benchmark

DLVHEX_NAMESPACE_END

#endif // DLVHEX_H_BENCHMARKING_INCLUDED_1555


// Local Variables:
// mode: C++
// End:
