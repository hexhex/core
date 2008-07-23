/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 *               2007, 2008 Thomas Krennwallner
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
 * @file ProgramNode.h
 * @author Thomas Krennwallner
 * @date Sat Jul 12 12:12:08 CET 2008
 *
 * @brief Program node class.
 *
 */


#if !defined(_DLVHEX_PROGRAMNODE_H)
#define _DLVHEX_PROGRAMNODE_H

#include "dlvhex/PlatformDefinitions.h"

#include <string>

DLVHEX_NAMESPACE_BEGIN

// forward declaration
class BaseVisitor;

/**
 * @brief Abstract base class for all objects that are part of a
 * program and dynamically created.
 *
 * \ingroup dlvhextypes
 *
 * This class is used as a common base class for 
 * dlvhex' data structures.
 */
class DLVHEX_EXPORT ProgramNode
{
 protected:

  /// source of the ProgramNode
  std::string source;

  /// line number
  int line;

  /// column
  int col;

  /// protected default ctor (by default unspecified)
  ProgramNode()
    : source(), line(-1), col(-1)
  { }



 public:

  /// dtor
  virtual
  ~ProgramNode()
  { }


  /**
   * The accept method is part of the visitor pattern and used to
   * double dispatch the correct type of the child, i.e., if someone
   * calls accept on a subclass Atom with BaseVisitor* v,
   * Atom::accept() will call v->visit(this) and *v can decide what to
   * do. This is useful in situation where we have an Atom* pointer to
   * an ExternalAtom object and want to pretty print the ExternalAtom
   * in its different representations (say in raw, first order or
   * higher order mode). For each representation form we implement the
   * corresponding concrete Visitor class.
   *
   * Please note that we need a pointer argument for the accept()
   * method to allow following beautiful beasts:
   *
   * \code
   * std::list<Foo> foos(10, Foo());
   * //...
   * MyVisitor v;
   * std::for_each(foos.begin(), foos.end(), 
   *               std::bind2nd(std::mem_fun_ref(&Foo::accept), &v)
   *               );
   * \endcode
   *
   * Above shortcoming is actually a flaw in STL's std::bind2nd
   * implementation, but we can live with that for now.
   */
  virtual void
  accept(BaseVisitor* const) = 0;
  

  /// set source and position of this ProgramNode
  inline void
  setSourcePosition(const std::string& s, int l, int c)
  {
    source = s;
    line = l;
    col = c;
  }

  inline int
  getLine() const
  {
    return line;
  }

  inline int
  getColumn() const
  {
    return col;
  }

  inline std::string
  getSource() const
  {
    return source;
  }
};


template<typename InputIterator1, typename InputIterator2>
int
lexicographical_compare_3way(InputIterator1 first1, InputIterator1 last1,
			     InputIterator2 first2, InputIterator2 last2)
{
  while (first1 != last1 && first2 != last2 && *first1 == *first2)
    {
      ++first1;
      ++first2;
    }

  int res = 2 * (first1 == last1) - (first2 == last2);

  // did we find a mismatch?
  switch (res)
    {
    case 1: // first1,last1 == first2,last2
      res = 0;
      break;

    case 2: // first1,last1 < first2,last2
      res = -1;
      break;

    case -1: // first1,last1 > first2,last2
      res = 1;
      break;

    default: // res == 0: mismatch in the middle
      if (*first1 < *first2)
	res = -1;
      else // *first1 > *first2, because *first1 != *first2
	res = 1;
      break;
    }

  return res;
}




DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_PROGRAMNODE_H */

/* vim: set noet sw=4 ts=4 tw=80: */


// Local Variables:
// mode: C++
// End:
