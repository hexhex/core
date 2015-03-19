/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Thomas Krennwallner
 * Copyright (C) 2009-2015 Peter Schüller
 * Copyright (C) 2011, 2012, 2013 Christoph Redl
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
 * @file   Benchmarking.cpp
 * @author Peter Schüller
 * @date   Sat Nov  5 15:26:18 CET 2005
 * 
 * @brief  Benchmarking features (implementation).
 * 
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "dlvhex2/Benchmarking.h"
#include <boost/foreach.hpp>
#include <iostream>
#include <set>
#include <boost/thread/mutex.hpp>

DLVHEX_NAMESPACE_BEGIN

namespace benchmark
{

namespace simple
{

BenchmarkController::Stat::Stat(const std::string& name):
  name(name), count(0), prints(0), start(), duration(), running(false)
{
}

// init, display start of benchmarking
BenchmarkController::BenchmarkController():
  myID(0), maxID(0), instrumentations(), name2id(), output(&(std::cerr)), printSkip(0), sus(0)
{
  myID = getInstrumentationID("BenchmarkController lifetime");
  start(myID);
}

// destruct, output benchmark results
BenchmarkController::~BenchmarkController()
{
  stop(myID);

  BOOST_FOREACH(const Stat& st, instrumentations)
  {
    printInformation(st);
  }
}

namespace
{
BenchmarkController* instance = 0;
}

void BenchmarkController::finish()
{
  if( instance )
    delete instance;
  instance = 0;
}

BenchmarkController& BenchmarkController::Instance()
{
  if( instance == 0 )
    instance = new BenchmarkController;
  return *instance;
}

// output stream
void BenchmarkController::setOutput(std::ostream* o)
{
  output = o;
}

// amount of accumulated output (default: each call)
void BenchmarkController::setPrintInterval(Count skip)
{
  printSkip = skip;
}

// get ID or register new one
ID BenchmarkController::getInstrumentationID(const std::string& name)
{
  boost::mutex::scoped_lock lock(mutex);
  std::map<std::string, ID>::const_iterator it = name2id.find(name);
  if( it == name2id.end() )
  {
    ID newid = maxID;
    instrumentations.push_back(Stat(name));
    name2id[name] = newid;
    maxID++;
    return newid;
  }
  else
  {
    return it->second;
  }
}

void BenchmarkController::suspend(){
  if( sus == 0 ) {
    myID = getInstrumentationID("BMController suspend");
    start(myID);
  }
  boost::mutex::scoped_lock lock(mutex);
  sus++;
}

void BenchmarkController::resume(){
  boost::mutex::scoped_lock lock(mutex);
  sus--;
  if( sus == 0 ){
    myID = getInstrumentationID("BMController suspend");
    stop(myID);
  }
}

std::string BenchmarkController::count(const std::string& name, int width) const
{
  if (sus > 0) return "-";
  std::map<std::string, ID>::const_iterator it = name2id.find(name);
  if( it == name2id.end() )
    return "-";
  benchmark::ID id = it->second;
  std::ostringstream oss;
  oss << std::setw(width) << getStat(id).count;
  return oss.str();
}

std::string BenchmarkController::duration(const std::string& name, int width) const
{
  if (sus > 0) return "-";
  std::map<std::string, ID>::const_iterator it = name2id.find(name);
  if( it == name2id.end() )
    return "-";
  benchmark::ID id = it->second;
  std::ostringstream oss;
  printInSecs(oss, getStat(id).duration, width);
  return oss.str();
}

// stop and do not record, handle non-started id's gracefully
void BenchmarkController::invalidate(ID id)
{
  if (sus > 0) return;
  boost::mutex::scoped_lock lock(mutex);
  Stat& st = instrumentations[id];

  if( st.running )
    st.running = false;
}

// copy data from one id to another id and call stop() on that other id
// e.g. do this for several interesting benchmarks at first model
void BenchmarkController::snapshot(ID id, ID intoID)
{
  if (sus > 0) return;
	{
	  boost::mutex::scoped_lock lock(mutex);
	  Stat& st = instrumentations[id];
	  Stat& intost = instrumentations[intoID];

	  // copy (overwrite old snapshot!)
	  // do not copy name!
	  intost.count = st.count;
	  // do not copy prints! (might produce duplicates)
	  intost.start = st.start;
	  intost.duration = st.duration;
	  intost.running = st.running;
	}
	// stop but do not count
	stop(intoID, false);
}

void BenchmarkController::snapshot(const std::string& fromstr, const std::string& tostr)
{
  if (sus > 0) return;
	ID idfrom = getInstrumentationID(fromstr);
	ID idto = getInstrumentationID(tostr);
	snapshot(idfrom, idto);
}

} // namespace simple

namespace nestingAware
{

NestingAwareController::Stat::Stat(const std::string& name, Duration printInterval):
  name(name), count(0), duration(), pureDuration(), level(0),
  nextPrint(boost::posix_time::microsec_clock::local_time() + printInterval) {
}

NestingAwareController::Current::Current(ID which):
  which(which), firststart(), start() {}

// init, display start of benchmarking
NestingAwareController::NestingAwareController():
  myID(0), maxID(0), instrumentations(), name2id(), current(), output(&(std::cerr)),
  printInterval(boost::posix_time::seconds(10.0)) // print continuously all 10 seconds
{
  myID = getInstrumentationID("BenchmarkController lifetime");
  start(myID);
}

// destruct, output benchmark results
NestingAwareController::~NestingAwareController()
{
  stop(myID);
  if( !current.empty() && output )
    // better not throw from destructor
    (*output) << "destructing NestingAwareController but current is not empty!" << std::endl;

  // sort by pure duration, descending
  class PureSortPredicate {
    public:
      static bool isHigher(const Stat& s1, const Stat& s2) {
        return s1.pureDuration > s2.pureDuration;
      }
  };
  std::sort(instrumentations.begin(), instrumentations.end(), PureSortPredicate::isHigher);

  Duration total;
  BOOST_FOREACH(const Stat& st, instrumentations)
  {
    printInformation(st);
    if( st.count != 0 ) // this way we do not count snapshot "... to first model" pure durations
      total += st.pureDuration;
  }
  #ifndef NDEBUG
  // to verify if all partial times sum up to the total runtime
  if( output ) {
    (*output) << "Sum of pure durations = ";
    printInSecs(std::cerr, total) << "s." << std::endl;
  }
  #endif
}

namespace
{
NestingAwareController* instance = 0;
}

void NestingAwareController::finish()
{
  if( instance )
    delete instance;
  instance = 0;
}

NestingAwareController& NestingAwareController::Instance()
{
  if( instance == 0 )
    instance = new NestingAwareController;
  return *instance;
}

// output stream
void NestingAwareController::setOutput(std::ostream* o)
{
  output = o;
}

// amount of accumulated output (default: each call)
void NestingAwareController::setPrintInterval(Count skip)
{
  // TODO
  std::cerr << "not implemented NestingAwareController::setPrintInterval" << std::endl;
}

// get ID or register new one
ID NestingAwareController::getInstrumentationID(const std::string& name)
{
  boost::mutex::scoped_lock lock(mutex);
  std::map<std::string, ID>::const_iterator it = name2id.find(name);
  if( it == name2id.end() )
  {
    ID newid = maxID;
    instrumentations.push_back(Stat(name, printInterval));
    name2id[name] = newid;
    maxID++;
    return newid;
  }
  else
  {
    return it->second;
  }
}

void NestingAwareController::suspend(){
  if( sus == 0 ) {
    myID = getInstrumentationID("BMController suspend");
    start(myID);
  }
  boost::mutex::scoped_lock lock(mutex);
  sus++;
}

void NestingAwareController::resume(){
  boost::mutex::scoped_lock lock(mutex);
  sus--;
  if( sus == 0 ){
    myID = getInstrumentationID("BMController suspend");
    stop(myID);
  }
}

std::string NestingAwareController::count(const std::string& name, int width) const
{
  std::map<std::string, ID>::const_iterator it = name2id.find(name);
  if( it == name2id.end() )
    return "-";
  benchmark::ID id = it->second;
  std::ostringstream oss;
  oss << std::setw(width) << getStat(id).count;
  return oss.str();
}

std::string NestingAwareController::duration(const std::string& name, int width) const
{
  std::map<std::string, ID>::const_iterator it = name2id.find(name);
  if( it == name2id.end() )
    return "-";
  benchmark::ID id = it->second;
  std::ostringstream oss;
  // we return pure duration as it is the purpose of NestingAwareController
  printInSecs(oss, getStat(id).pureDuration, width);
  return oss.str();
}

// stop and do not record, handle non-started id's gracefully
void NestingAwareController::invalidate(ID id)
{
  if( !current.empty() && current.back().which == id ) {
    // save start time of pure period (we do not want to lose this)
    Time start = current.back().start;
    // destroy current without counting it
    current.pop_back();
    // set start time to new top-of-stack
    if( current.size() > 1 ) {
      current[current.size()-2].start = start;
    }
  }
}

// copy data from one id to another id and call stop() on that other id
// e.g. do this for several interesting benchmarks at first model
void NestingAwareController::snapshot(ID id, ID intoID)
{
  boost::mutex::scoped_lock lock(mutex);
  Stat& st = instrumentations[id];
  Stat& intost = instrumentations[intoID];

  // copy (overwrites old snapshot!)

  Time now = boost::posix_time::microsec_clock::local_time();
  intost.count = st.count;
  intost.duration = st.duration;
  intost.pureDuration = st.pureDuration;
  if( current.back().which == id ) {
    // if top level entry in current has which=id then add to pureDuration
    intost.pureDuration += now - current.back().start;
  }
  // find bottom-most level entry in current where which=id and add to duration (if exists)
  for(unsigned u = 0; u < current.size(); ++u) {
    if( current[u].which == id ) {
      intost.duration += now - current[u].firststart;
      break;
    }
  }
}

// print information about stat
void NestingAwareController::printInformation(const Stat& st)
{
  if( output )
  {
    (*output) <<
      "BM:" << // std::setw(2) << int(&st-instrumentations.data()) << " " <<
      std::setw(30) << st.name <<
      ": count:" << std::setw(8) << st.count;
		(*output) << " total:";
    printInSecs(*output, st.duration, 4) << "s pure:";
    printInSecs(*output, st.pureDuration, 4) << "s avg:";
		if( st.count > 0 )
		{
			printInSecs(*output, st.duration/st.count, 4) << "s";
		}
		else
		{
			(*output) << "  -.---s";
		}
    (*output) << std::endl;
  }
}

void NestingAwareController::snapshot(const std::string& fromstr, const std::string& tostr)
{
	ID idfrom = getInstrumentationID(fromstr);
	ID idto = getInstrumentationID(tostr);
	snapshot(idfrom, idto);
}

} // namespace simple

} // namespace benchmark

DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
