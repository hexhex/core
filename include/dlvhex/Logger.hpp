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
 * @file   Logger.hpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Logging facility with comfortable indentation and closures.
 */

#ifndef LOGGER_HPP_INCLUDED__17092010
#define LOGGER_HPP_INCLUDED__17092010
#include <boost/preprocessor/cat.hpp>
#include <boost/optional.hpp>
#include <boost/cstdint.hpp>

#include <iostream>
#include <iomanip>
#include <sstream>

// singleton logger class
class Logger
{
public:
  // levels are specified and can be activated via bitmasks
  // (all 32 bits may be used)
  // logger itself logs on DBG
  typedef uint32_t Levels;
  static const Levels DBG =     0x01;
  static const Levels INFO =    0x02;
  static const Levels WARNING = 0x04;
  static const Levels ERROR =   0x08;

  // this is now very dlvhex-specific
  static const Levels PLUGIN =  0x10; // plugin related things
  static const Levels ANALYZE = 0x20; // program analysis
  static const Levels MODELB =  0x40; // model building
  static const Levels STATS  =  0x80; // statistic information

private:
  std::ostream& out;
  std::string indent;
  Levels printlevels;
  // width of field for level printing, if 0, level is not printed
  int levelwidth;

private:
  // default output is std::cerr, change this later with stream() = ...
  // default output is all output levels, change this later with printlevels() = ...
  // default output is i hex character of level printed, change this later with levelwidth() = ...
  Logger():
    out(std::cerr), indent(), printlevels(~static_cast<Levels>(0)), levelwidth(1) {}

  ~Logger()
    {
      stream() << std::endl;
      startline(DBG);
      #ifndef NDEBUG
      stream() << "clean exit!" << std::endl;
      #endif
    }

public:
  static Logger& Instance();

  inline std::ostream& stream()
    { return out; }

  void setPrintLevels(Levels levels);
  void setPrintLevelWidth(int width);
  Levels getPrintLevels() const;

  // this method does not ask shallPrint!
  inline void startline(Levels forlevel)
    {
      if( levelwidth == 0 )
        out << indent;
      else
        out << std::hex << std::setw(levelwidth) << forlevel << std::dec << " " << indent;
    }

  inline bool shallPrint(Levels forlevel)
    { return (printlevels & forlevel) != 0; }

  friend class Closure;
  class Closure
  {
  private:
    Logger& l;
    Levels level;
    unsigned cutoff;
    bool message;

    inline void sayHello()
    {
      // hello message
      if( message )
      {
        l.startline(level);
        l.stream() << "ENTRY" << std::endl;
      }
    }

    inline void sayGoodbye()
    {
      // goodbye message
      if( message )
      {
        l.startline(level);
        l.stream() << "EXIT" << std::endl;
      }
    }

  public:
    // generic
    Closure(Logger& l, Levels level, const std::string& str, bool message):
      l(l), level(level), cutoff(l.indent.size()), message(message)
    {
      if( l.shallPrint(level) )
      {
        l.indent += str + " ";
        sayHello();
      }
    }

    // with value (converted/reinterpret-casted to const void* const)
    Closure(Logger& l, Levels level, const std::string& str, const void* const val, bool message):
      l(l), level(level), cutoff(l.indent.size()), message(message)
    {
      if( l.shallPrint(level) )
      {
        std::stringstream ss;
        ss << str << "/" << val << " ";
        l.indent += ss.str();
        sayHello();
      }
    }

    ~Closure()
    {
      if( l.shallPrint(level) )
      {
        sayGoodbye();
        // restore indentation level
        l.indent.erase(cutoff);
      }
    }
  };

  class Init
  {
  public:
    Init(Levels levels)
    {
      Logger::Instance().setPrintLevels(levels);
    }
  };
};

// the following will always be realized
//#ifndef NDEBUG
#  define LOG(level,streamout) do { if( Logger::Instance().shallPrint(Logger:: level) ) { \
       Logger::Instance().startline(Logger:: level); \
       Logger::Instance().stream() << streamout << std::endl; \
     } } while(false);
#    define LOG_CLOSURE_ID BOOST_PP_CAT(log_closure_,__LINE__)
#  define LOG_INDENT(level)              Logger::Closure LOG_CLOSURE_ID (Logger::Instance(), Logger:: level, "  ", false)
#  define LOG_SCOPE(level,name,msg)      Logger::Closure LOG_CLOSURE_ID (Logger::Instance(), Logger:: level, name, msg)
#  define LOG_VSCOPE(level,name,val,msg) Logger::Closure LOG_CLOSURE_ID (Logger::Instance(), Logger:: level, name, reinterpret_cast<const void* const>(val), msg)
/*
#else
#  define LOG(level,streamout) 			do { } while(false)
#  define LOG_CLOSURE_ID BOOST_PP_CAT(log_closure_,__LINE__) do { } while(false)
#  define LOG_INDENT(level)              	do { } while(false)
#  define LOG_SCOPE(level,name,msg)      	do { } while(false)
#  define LOG_VSCOPE(level,name,val,msg) 	do { } while(false)
#endif
*/

#  define LOG_INIT(setlevel)             namespace { Logger::Init LOG_CLOSURE_ID (setlevel); }


// the following are debug-flag dependant
#ifndef NDEBUG
#  define DBGLOG(level,streamout)           LOG(level,streamout)
#  define DBGLOG_INDENT(level)              LOG_INDENT(level)
#  define DBGLOG_SCOPE(level,name,msg)      LOG_SCOPE(level,name,msg)
#  define DBGLOG_VSCOPE(level,name,val,msg) LOG_VSCOPE(level,name,val,msg)
#else
#  define DBGLOG(level,streamout)           do { } while(false)
#  define DBGLOG_INDENT(level)              do { } while(false)
#  define DBGLOG_SCOPE(level,name,msg)      do { } while(false)
#  define DBGLOG_VSCOPE(level,name,val,msg) do { } while(false)
#endif

#endif // LOGGER_HPP_INCLUDED__17092010
