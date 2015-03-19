/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Thomas Krennwallner
 * Copyright (C) 2009-2015 Peter Schüller
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
 * @file   TestBenchmarking.cpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Test multiply nested benchmarking.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "dlvhex2/Benchmarking.h"
#include "dlvhex2/Logger.h"

#define BOOST_TEST_MODULE "TestBenchmarking"
#include <boost/test/unit_test.hpp>

#include <iostream>

#include <time.h>

LOG_INIT(Logger::ERROR | Logger::WARNING)

DLVHEX_NAMESPACE_USE

typedef benchmark::ID BID;

void millisleep(unsigned ms) {
  struct timespec ts;
  ts.tv_sec = 0;
  while( ms > 1000 ) {
    ts.tv_sec ++;
    ms -= 1000;
  }
  ts.tv_nsec = ms*1000L*1000L;
  nanosleep(&ts, NULL);
}

BOOST_AUTO_TEST_CASE(nestingAware) 
{
  // create controller singleton
  typedef benchmark::nestingAware::NestingAwareController Controller;
  Controller& ctrl = Controller::Instance();

  BID id1 = ctrl.getInstrumentationID("1");
  BID id2 = ctrl.getInstrumentationID("2");

  ctrl.start(id2);
  millisleep(100); // assume this is not exact
  {
    ctrl.start(id1);
    millisleep(100); // assume this is not exact
    // start again!
    ctrl.start(id1);
    millisleep(100); // assume this is not exact
    ctrl.stop(id1);
    millisleep(100); // assume this is not exact
    ctrl.stop(id1);
  }
  millisleep(100); // assume this is not exact
  ctrl.stop(id2);

  std::stringstream ss;
  ss << ctrl.duration("1", 1) << " " << ctrl.duration("2", 1);
  float f1, f2;
  ss >> f1 >> f2;
  LOG(WARNING, "got durations 1:" << f1 << " 2:" << f2);
  BOOST_CHECK(f1 > 0.095*3); 
  BOOST_CHECK(f1 < 0.105*3); 
  BOOST_CHECK(f2 > 0.095*2); 
  BOOST_CHECK(f2 < 0.105*2); 
}

// Local Variables:
// mode: C++
// End:
