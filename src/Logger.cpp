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
 * @file   Logger.cpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Implementation of logging facility.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "dlvhex2/Logger.h"

namespace
{
  Logger* instance = 0;
}

namespace
{
  boost::mutex* mutex = 0;
}

Logger& Logger::Instance()
{
  if( instance == 0 )
    instance = new Logger();
  return *instance;
}

boost::mutex& Logger::Mutex()
{
  if( mutex == 0 )
  {
#ifdef NDEBUG
    // rationale behind this message: if we use NDEBUG in dlvhex,
    // this message will never appear (because Logger.h does not use the mutex).
    // if we use NDEBUG in dlvhex but DEBUG in plugin, this appears and
    // this might hit performance therefore we give a warning (once)
    if( Logger::Instance().shallPrint(Logger::WARNING) ) {
      Logger::Instance().stream() <<
        "Logger (performance) warning: use NDEBUG "
        "to deactivate logging mutex in plugin!" << std::endl;
    }
#endif
    mutex = new boost::mutex();
  }
  return *mutex;
}

void Logger::setPrintLevels(Levels levels)
{
  if( (levels & ERROR) != ERROR )
    out << "Logger warning: deactivated ERROR level" << std::endl;
  printlevels = levels;
}

void Logger::setPrintLevelWidth(int width)
{
  assert(width >= 0 );
  levelwidth = width;
}

Logger::Levels Logger::getPrintLevels() const{
  return printlevels;
}
