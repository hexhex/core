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


/*
template<typename T>
inline std::ostream& printopt_main(std::ostream& o, const T& t)
{
  if( !t )
    return o << "unset";
  else
    return o << t;
}
template<typename TT>
inline std::ostream& printopt_main(std::ostream& o, const boost::shared_ptr<TT>& t)
{
  if( t == 0 )
    return o << "null";
  else
    return o << t;
}
template<typename TT>
inline std::ostream& printopt_main(std::ostream& o, const boost::optional<TT>& opt)
{
  typename std::list<TT>::const_iterator>
  if( !it )
    return o << "unset";
  else
    return o << *it;
}
template<typename TT>
inline std::ostream& printopt_main(std::ostream& o, boost::optional<typename std::list<TT>::iterator> it)
{
  if( !it )
    return o << "unset";
  else
    return o << *it;
}

template<typename T>
struct printopt_container
{
  const T& t;
  printopt_container(const T& t): t(t) {}
};

template<typename T>
inline std::ostream& operator<<(std::ostream& o, printopt_container<T> c)
{
  return printopt_main(o, c.t);
}

template<typename T>
inline printopt_container<T> printopt(const T& t)
{
  return printopt_container<T>(t);
}
*/

// ******************** //

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

/*
template<typename T>
struct print_stream_container_noref:
  public print_container
{
  T t;
  print_stream_container_noref(T t): t(t) {}
  virtual ~print_stream_container_noref() {}
  virtual std::ostream& print(std::ostream& o) const
    { return o << t; }
};
*/

template<typename T>
struct print_method_container:
  public print_container
{
  const T& t;
  print_method_container(const T& t): t(t) {}
  virtual ~print_method_container() {}
  virtual std::ostream& print(std::ostream& o) const
    { return t.print(o); }
};

template<typename T>
inline print_container* print_method(const T& t)
{
  return new print_method_container<const T&>(t);
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

/*
// display optional models as addresses of models
template<typename T>
inline print_container* printoptmodel(const boost::optional<T>& t)
{
  if( !!t )
	{
		T model = t.get();
    return new print_stream_container<const void*>(
        reinterpret_cast<const void*>(&t.get()));
	}
  else
    return new print_stream_container<const char*>("unset");
}
*/

// display optional iterators to models as addresses of models (as in omodel_l_current)
template<typename T>
inline print_container* printoptitermodel(const boost::optional<T>& t)
{
  if( !!t )
	{
		const T& iter = t.get();
		const typename T::value_type& ref = *iter;
    return new print_stream_container<const void*>(
        reinterpret_cast<const void*>(&ref));
	}
  else
    return new print_stream_container<const char*>("unset");
}


#endif // LOGGER_HPP_INCLUDED__17092010
