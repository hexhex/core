/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
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
 * @file   Literal.tcc
 * @author Roman Schindlauer
 * @author Thomas Krennwallner
 * @date   Sun Sep 4 12:39:40 2005
 * 
 * @brief  Literal class.
 * 
 * 
 */

#if !defined(_DLVHEX_LITERAL_TCC)
#define _DLVHEX_LITERAL_TCC

#include "dlvhex/BaseVisitor.h"

DLVHEX_NAMESPACE_BEGIN


template<typename T>
Literal<T>::Literal(const AtomPtr& at)
  : atom(at)
{ }


template<typename T>
Literal<T>&
Literal<T>::operator=(const Literal<T>& lit2)
{
  if (this != &lit2)
    {
      this->atom = lit2.atom;
    }
  return *this;
}


template<typename T> template<typename U>
Literal<T>&
Literal<T>::operator=(const Literal<U>& lit2)
{
  if (this != &lit2)
    {
      this->atom = lit2.atom;
    }
  return *this;
}


template<typename T>
const AtomPtr&
Literal<T>::getAtom() const
{
  return atom;
}


template<typename T>
AtomPtr&
Literal<T>::getAtom()
{
  return atom;
}


template<typename T>
bool
Literal<T>::unifiesWith(const BaseLiteral& l) const
{
  if (typeid(*this) == typeid(l))
    {
      return atom->unifiesWith(*l.getAtom());
    }
  return false;
}


template<typename T>
int
Literal<T>::compare(const BaseLiteral& lit2) const
{
  const std::type_info& type1 = typeid(*this);
  const std::type_info& type2 = typeid(lit2);

  if (type1 == type2)
    {
      // both have same negation trait: the atoms must be equal
      return atom->compare(*lit2.getAtom());
    }

  return type1.before(type2) ? -1 : 1;
}
  

/**
 * @brief Accepts a visitor.
 *
 * According to the visitor pattern, accept simply calls the respective
 * visitor with the Literal itself as parameter.
 *
 * \sa http://en.wikipedia.org/wiki/Visitor_pattern
 */
template<typename T>
void
Literal<T>::accept(BaseVisitor* const v)
{
  v->visit(this);
}


DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_LITERAL_TCC */

/* vim: set noet sw=4 ts=4 tw=80: */


// Local Variables:
// mode: C++
// End:
