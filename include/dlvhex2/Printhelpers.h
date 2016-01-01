/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005-2007 Roman Schindlauer
 * Copyright (C) 2006-2015 Thomas Krennwallner
 * Copyright (C) 2009-2016 Peter Schüller
 * Copyright (C) 2011-2016 Christoph Redl
 * Copyright (C) 2015-2016 Tobias Kaminski
 * Copyright (C) 2015-2016 Antonius Weinzierl
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
 * @file   Printhelpers.h
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 *
 * @brief  Helpers for printing objects to streams.
 */

#ifndef PRINTHELPERS_HPP_INCLUDED__11122011
#define PRINTHELPERS_HPP_INCLUDED__11122011

#include <boost/range/iterator_range.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/optional.hpp>

#include <iostream>
#include <sstream>
#include <set>
#include <vector>

/** With this class, you can make your own classes ostream-printable.
 *
 * Usage:
 * * derive YourType from ostream_printable<YourType>
 * * implement std::ostream& YourType::print(std::ostream& o) const;
 * * now you can << YourType << and it will use the print() function.
 * see http://en.wikipedia.org/wiki/BartonNackman_trick. */
template<typename T>
class ostream_printable
{
    /** \brief Streaming operator.
     * @param o Output stream to print to.
     * @param t Element to print.
     * @return \p o. */
    friend std::ostream& operator<<(std::ostream& o, const T& t)
        { return t.print(o); }
    // to be defined in derived class:
    //std::ostream& print(std::ostream& o) const;
};

// if some class has a method "std::ostream& print(std::ostream&) const"
// and you have an object o of this type
// then you can do "std::cerr << ... << print_method(o) << ... " to print it

// if some other method is used to print T foo
// e.g. std::ostream& BAR::printFOO(std::ostream& o, const FOO& p) const
// then you can do
// std::cerr << ... << print_function(boost::bind(&BAR::printFOO, &bar, _1, foo)) << ...
// e.g. std::ostream& printFOO(std::ostream& o, const FOO& p) const
// then you can do
// std::cerr << ... << print_function(boost::bind(&printFoo, _1, foo)) << ...

// std::cerr << ... << printopt(boost::optional<T>) << ...
// gives "unset" or prints T's contents

// std::cerr << ... << printptr(T* or boost::shared_ptr<T>) << ...
// gives "null" or prints T* as a void*

// std::cerr << ... << printrange(Range r) << ... prints generic ranges enclosed in "<>"
// std::cerr << ... << printvector(std::vector<T>) << ... prints generic vectors enclosed in "[]"
// std::cerr << ... << printset(std::set<T>) << ... prints generic sets enclosed in "{}"

/** \brief Printable object base class. */
struct print_container
{
    /** \brief Destructor. */
    virtual ~print_container() {}
    /** \brief Printer for an object to be implement by subclasses.
     * @param o Output stream to print to.
     * @return \p o. */
    virtual std::ostream& print(std::ostream& o) const = 0;
};

/** \brief Streaming operator.
 * @param o Output stream to print to.
 * @param c Container to print using print_container::print.
 * @return \p o. */
inline std::ostream& operator<<(std::ostream& o, print_container* c)
{
    assert(c);
    std::ostream& ret = c->print(o);
    delete c;
    return ret;
}


/** \brief Printable object base class. */
template<typename T>
struct print_stream_container:
public print_container
{
    /** Element to print. */
    T t;
    /** \brief Copy-constructor.
     * @param t print_container. */
    print_stream_container(const T& t): t(t) {}
    /** \brief Destructor. */
    virtual ~print_stream_container() {}
    virtual std::ostream& print(std::ostream& o) const
        { return o << t; }
};

/** \brief Printable object base class. */
struct print_method_container:
public print_container
{
    typedef boost::function<std::ostream& (std::ostream&)>
        PrintFn;
    /** \brief Printer function. */
    PrintFn fn;
    /** \brief Copy-constructor.
     * @param t print_method_container. */
    print_method_container(const PrintFn& fn): fn(fn) {}
    /** \brief Destructor. */
    virtual ~print_method_container() {}
    virtual std::ostream& print(std::ostream& o) const
        { return fn(o); }
};

/** \brief This can be used if T contains a method "ostream& print(ostream&) const".
 * @param t Object to create a container for.
 * @return print_container for \p t. */
template<typename T>
inline print_container* print_method(const T& t)
{
    return new print_method_container(
        boost::bind(&T::print, &t, _1));
}


/** \brief This can be used if some third party method is used to print T.
 *
 * E.g. std::ostream& BAR::printFOO(std::ostream& o, const FOO& p) const
 * is printed as
 * ... << print_function(boost::bind(&BAR::printFOO, &bar, _1, foo)) << ... .
 * @param fn print_method_container to create a print_container for.
 * @return print_container for \p fn. */
inline print_container* print_function(
const print_method_container::PrintFn& fn)
{
    return new print_method_container(fn);
}


/** \brief Create a print_container for an object only if it is defined.
 * @param t Object to print, can be undefined using boost::optional.
 * @return print_container for \p t if defined and a dummy container otherwise. */
template<typename T>
inline print_container* printopt(const boost::optional<T>& t)
{
    if( !!t )
        return new print_stream_container<const T&>(t.get());
    else
        return new print_stream_container<const char*>("unset");
}


/** \brief Create a print_container pointer for an object only if it is defined.
 * @param t Object to print, can be undefined using boost::optional.
 * @return print_container pointer for \p t if defined and a dummy container otherwise. */
template<typename T>
inline print_container* printptr(const boost::shared_ptr<T>& t)
{
    if( !!t )
        return new print_stream_container<const void*>(
            reinterpret_cast<const void*>(t.get()));
    else
        return new print_stream_container<const char*>("null");
}


/** \brief Create a print_container pointer for an object only if it is defined.
 * @param t Object to print, can be undefined using boost::optional.
 * @return print_container pointer for \p t if defined and a dummy container otherwise. */
template<typename T>
inline print_container* printptr(const boost::shared_ptr<const T>& t)
{
    if( t != 0 )
        return new print_stream_container<const void*>(
            reinterpret_cast<const void*>(t.get()));
    else
        return new print_stream_container<const char*>("null");
}


/** \brief Create a print_container pointer for an object only if it is defined.
 * @param t Object to print, can be undefined using boost::optional.
 * @return print_container pointer for \p t if defined and a dummy container otherwise. */
template<typename T>
inline print_container* printptr(const T* const t)
{
    if( t != 0 )
        return new print_stream_container<const void*>(
            reinterpret_cast<const void* const>(t));
    else
        return new print_stream_container<const char*>("null");
}


/** \brief Creates a print_container pointer for multiple objects.
 * @param r Objects to print, can be undefined using boost::optional.
 * @param open Begin character.
 * @param sep Character to be printed between the objects in \p r.
 * @param close End character.
 * @return print_container pointer for \p r.
 *
 * Example:
 * std::cerr << ... << printrange(Range r) << ...;
 */
template<typename Range>
inline print_container* printrange(Range r,
const char* open="<", const char* sep=",", const char* close=">")
{
    std::ostringstream o;
    o << open;
    typename Range::const_iterator it = boost::begin(r);
    typename Range::const_iterator itend = boost::end(r);
    if( it != itend ) {
        o << *it;
        it++;
    }
    for(; it != itend; ++it)
        o << sep << *it;
    o << close;
    return new print_stream_container<std::string>(o.str());
}


/** \brief Creates a print_container pointer for multiple objects in set notation.
 * @param t Objects to print, can be undefined using boost::optional.
 * @param open Begin character.
 * @param sep Character to be printed between the objects in \p r.
 * @param close End character.
 * @return print_container pointer for \p r.
 *
 * Example:
 * std::cerr << ... << printset(std::set<T>) << ...;
 */
template<typename T>
inline print_container* printset(const std::set<T>& t,
const char* open="{", const char* sep=",", const char* close="}")
{
    return printrange(t, open, sep, close);
}


/** \brief Creates a print_container pointer for multiple objects in vector notation.
 * @param t Objects to print, can be undefined using boost::optional.
 * @param open Begin character.
 * @param sep Character to be printed between the objects in \p r.
 * @param close End character.
 * @return print_container pointer for \p r.
 *
 * Example:
 * std::cerr << ... << printvector(std::vector<T>) << ...;
 */
template<typename T>
inline print_container* printvector(const std::vector<T>& t,
const char* open="[", const char* sep=",", const char* close="]")
{
    return printrange(t, open, sep, close);
}
#endif                           // PRINTHELPERS_HPP_INCLUDED__11122011

// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
