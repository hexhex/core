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
#include <boost/thread/mutex.hpp>

DLVHEX_NAMESPACE_BEGIN

namespace benchmark
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
  boost::mutex::scoped_lock lock(mutex);
  sus++;
}

void BenchmarkController::resume(){
  boost::mutex::scoped_lock lock(mutex);
  sus--;
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

} // namespace benchmark

DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
