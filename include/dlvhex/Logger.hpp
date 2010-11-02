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
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/optional.hpp>

#include <iostream>
#include <sstream>
#include <set>
#include <vector>

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



// 'print<foo>' usage:
// if some class has a method "std::ostream& print(std::ostream&) const"
// and you have an object o of this type
// then you can simply do "std::cerr << ... << print_method(o) << ... " to print it

struct print_container
{
  virtual ~print_container() {}
  virtual std::ostream& print(std::ostream& o) const = 0;
};

inline std::ostream& operator<<(std::ostream& o, print_container* c)
{
  std::ostream& ret = c->print(o);
  delete c;
  return ret;
}

template<typename T>
struct print_stream_container:
  public print_container
{
  T t;
  print_stream_container(const T& t): t(t) {}
  virtual ~print_stream_container() {}
  virtual std::ostream& print(std::ostream& o) const
    { return o << t; }
};

struct print_method_container:
  public print_container
{
  typedef boost::function<std::ostream& (std::ostream&)>
    PrintFn;
  PrintFn fn;
  print_method_container(const PrintFn& fn): fn(fn) {}
  virtual ~print_method_container() {}
  virtual std::ostream& print(std::ostream& o) const
    { return fn(o); }
};

// this can be used if T contains a method "ostream& print(ostream&) const"
template<typename T>
inline print_container* print_method(const T& t)
{
  return new print_method_container(
      boost::bind(&T::print, &t, _1));
}

// this can be used if some third party method is used to print T
// e.g. std::ostream& BAR::printFOO(std::ostream& o, const FOO& p) const
// is printed as
// ... << print_function(boost::bind(&BAR::printFOO, &bar, _1, foo)) << ...
inline print_container* print_function(
    const print_method_container::PrintFn& fn)
{
  return new print_method_container(fn);
}

template<typename T1, typename T2>
inline print_container* printalt(bool condition, const T1& alt1, const T2& alt2)
{
  if( condition )
    return new print_stream_container<const T1&>(alt1);
  else
    return new print_stream_container<const T2&>(alt2);
}

template<typename T>
inline print_container* printopt(const boost::optional<T>& t)
{
  if( !!t )
    return new print_stream_container<const T&>(t.get());
  else
    return new print_stream_container<const char*>("unset");
}

template<typename T>
inline print_container* printptr(const boost::shared_ptr<T>& t)
{
  if( !!t )
    return new print_stream_container<const void*>(
        reinterpret_cast<const void*>(t.get()));
  else
    return new print_stream_container<const char*>("null");
}

template<typename T>
inline print_container* printptr(const boost::shared_ptr<const T>& t)
{
  if( t != 0 )
    return new print_stream_container<const void*>(
        reinterpret_cast<const void*>(t.get()));
  else
    return new print_stream_container<const char*>("null");
}

template<typename T>
inline print_container* printptr(const T* const t)
{
  if( t != 0 )
    return new print_stream_container<const void*>(
        reinterpret_cast<const void* const>(t));
  else
    return new print_stream_container<const char*>("null");
}

template<typename T>
inline print_container* printset(const std::set<T>& t)
{
  std::ostringstream o;
  o << "{";
  typename std::set<T>::const_iterator it = t.begin();
  if( it != t.end() )
  {
    o << *it;
    it++;
  }
  for(; it != t.end(); ++it)
    o << "," << *it;
  o << "}";
  return new print_stream_container<std::string>(o.str());
}

template<typename T>
inline print_container* printvector(const std::vector<T>& t)
{
  std::ostringstream o;
  o << "<";
  typename std::vector<T>::const_iterator it = t.begin();
  if( it != t.end() )
  {
    o << *it;
    it++;
  }
  for(; it != t.end(); ++it)
    o << "," << *it;
  o << ">";
  return new print_stream_container<std::string>(o.str());
}

// usage:
//   derive YourType from ostream_printable<YourType>
//   implement std::ostream& YourType::print(std::ostream& o) const;
//   now you can << YourType << and it will use the print() function
// see http://en.wikipedia.org/wiki/Barton–Nackman_trick
template<typename T>
class ostream_printable
{
  friend std::ostream& operator<<(std::ostream& o, const T& t) const
    { return t.print(o); }
  // to be defined in derived class
  //std::ostream& print(std::ostream& o) const;
};

#endif // LOGGER_HPP_INCLUDED__17092010
