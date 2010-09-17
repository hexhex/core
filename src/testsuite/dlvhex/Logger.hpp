/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2010 Peter Schüller
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

#ifndef LOGGER_HPP_INCLUDED__17092010
#define LOGGER_HPP_INCLUDED__17092010

#include <iostream>
#include <sstream>
#include <boost/preprocessor/cat.hpp>

// singleton logger class
class Logger
{
private:
  std::ostream& out;
  std::string indent;

private:
  // @todo: make this depend on some global config/options (early initialization!)
  Logger():
    out(std::cerr), indent() {}

  ~Logger()
    {
      stream() << std::endl;
      startline();
      stream() << "clean exit!" << std::endl;
    }

public:
  static Logger& Instance();

  inline std::ostream& stream()
    { return out; }
  inline void startline()
    { out << indent; }

  friend class Closure;
  class Closure
  {
  private:
    Logger& l;
    unsigned cutoff;
    bool message;

    inline void sayHello()
    {
      // hello message
      if( message )
      {
        l.startline();
        l.stream() << "ENTRY" << std::endl;
      }
    }

    inline void sayGoodbye()
    {
      // goodbye message
      if( message )
      {
        l.startline();
        l.stream() << "EXIT" << std::endl;
      }
    }

  public:
    // generic
    Closure(Logger& l, const std::string& str, bool message):
      l(l), cutoff(l.indent.size()), message(message)
    {
      l.indent += str + " ";
      sayHello();
    }

    // with object
    Closure(Logger& l, const std::string& str, const void* const ptr, bool message):
      l(l), cutoff(l.indent.size()), message(message)
    {
      std::stringstream ss;
      ss << str << "@" << ptr << " ";
      l.indent += ss.str();
      sayHello();
    }

    ~Closure()
    {
      sayGoodbye();
      // restore indentation level
      l.indent.erase(cutoff);
    }
  };
};

namespace
{
  Logger* instance = 0;
}

Logger& Logger::Instance()
{
  if( instance == 0 )
    instance = new Logger();
  return *instance;
}

#ifndef NDEBUG
#  define LOG(streamout) do { \
       Logger::Instance().startline(); \
       Logger::Instance().stream() << streamout << std::endl; \
     } while(false);
#    define LOG_CLOSURE_ID BOOST_PP_CAT(log_closure_,__LINE__)
#  define LOG_INDENT()             Logger::Closure LOG_CLOSURE_ID (Logger::Instance(), "  ", false)
#  define LOG_SCOPE(name,msg)      Logger::Closure LOG_CLOSURE_ID (Logger::Instance(), name, msg)
#  define LOG_PSCOPE(name,ptr,msg) Logger::Closure LOG_CLOSURE_ID (Logger::Instance(), name, static_cast<const void* const>(ptr), msg)
#else
#  define LOG(streamout)           do { } while(false)
#  define LOG_INDENT()             do { } while(false)
#  define LOG_SCOPE(name,msg)      do { } while(false)
#  define LOG_PSCOPE(name,ptr,msg) do { } while(false)
#endif
#define LOG_FUNCTION(func)        LOG_SCOPE(func,true)
#define LOG_METHOD(method,object) LOG_PSCOPE(method,object,true)

#endif // LOGGER_HPP_INCLUDED__17092010
