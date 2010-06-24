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

#include <boost/date_time/posix_time/posix_time.hpp>
#include <vector>
#include <ostream>

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

  static BenchmarkController* instance;

  ID maxID;
  std::vector<Stat> instrumentations;
  std::map<std::string, ID> name2id;

  std::ostream* output;
  Count printSkip;

  // print information about stat
  void printInformation(const Stat& st); // inline for performance
  // print continuous information about stat
  void printInformationContinous(
    Stat& st, const Duration& dur); // inline for performance
  
public:
  // singleton access
  static BenchmarkController& Instance();

  // delete singleton-> causes destructor to be called
  void finish();

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
  void printInformation(ID id); // inline for performance

  // 
  // record measured things
  //
  
  // start timer
  void start(ID id); // inline for performance
  // stop and record elapsed time, print stats
  void stop(ID id); // inline for performance
  // record count (no time), print stats
  void count(ID id, Count increment=1); // inline for performance
};

// print information about stat
// inline for performance
void BenchmarkController::printInformation(const Stat& st)
{
  if( output )
    (*output) <<
    "BM:" << std::setw(30) << st.name <<
    " count:" << std::setw(6) << st.count <<
    " avg:" << std::setw(6) << (st.duration/st.count) << "s"
    " total:" << std::setw(6) << st.duration << "s" << std::endl;
}

// print continuous information about stat
// inline for performance
void BenchmarkController::printInformationContinous(Stat& st, const Duration& dur)
{
  if( st.prints >= printSkip )
  {
    st.prints = 0;
    if( output )
      (*output) <<
      "BM:" << std::setw(30) << st.name <<
      " count:" << std::setw(6) << st.count <<
      " total:" << std::setw(6) << st.duration << "s" <<
      " last:" << std::setw(6) << dur << "s" << std::endl;
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
  printInformationContinous(s,Duration());
}


} // namespace benchmark

DLVHEX_NAMESPACE_END

//DLVHEX_BENCHMARK_DO(static dlvhex::benchmark::ID sid = dlvhex::benchmark::register("calling dlv process"));

//#define DLVHEX_BENCHMARK_DO(code) \
//  do { code; } while(0)
//
//
//#define DLVHEX_BENCHMARK_START(sid) \
//  do { 
//DLVHEX_BENCHMARK_START(sid)
//DLVHEX_BENCHMARK_STOP(sid)
//DLVHEX_BENCHMARK_COUNT(sid,num)
//
////#if defined(DLVHEX_DEBUG)
//
//#define DEBUG_START_TIMER						\
//  boost::posix_time::ptime boosttimerstart;				\
//  boost::posix_time::ptime boosttimerend;				\
//  do {									\
//    boosttimerstart = boost::posix_time::microsec_clock::local_time();	\
//  } while(0)
//#define DEBUG_RESTART_TIMER						\
//  do {									\
//    boosttimerstart = boost::posix_time::microsec_clock::local_time();	\
//  } while(0)
//#define DEBUG_STOP_TIMER(msg)						\
//  do {									\
//    boosttimerend = boost::posix_time::microsec_clock::local_time();	\
//    if (Globals::Instance()->doVerbose(Globals::PROFILING)) {		\
//      boost::posix_time::time_duration diff = boosttimerend - boosttimerstart; \
//      Globals::Instance()->getVerboseStream() << msg  << diff << "s" << std::endl; \
//      boosttimerstart = boosttimerend; }				\
//  } while(0)
//
//#else
//#define DEBUG_START_TIMER do { } while(0)
//#define DEBUG_RESTART_TIMER do { } while(0)
//#define DEBUG_STOP_TIMER(msg) do { } while(0)
//#endif // DLVHEX_DEBUG

#endif // DLVHEX_H_BENCHMARKING_INCLUDED_1555
