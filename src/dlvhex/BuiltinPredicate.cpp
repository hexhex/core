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
 * @file BuiltinPredicate.cpp
 * @author Roman Schindlauer
 * @author Thomas Krennwallner
 * @date Wed Oct 18 16:04:54 CEST 2006
 *
 * @brief Builtin predicate class.
 *
 *
 */


#include "dlvhex/BuiltinPredicate.h"
#include "dlvhex/BaseVisitor.h"

DLVHEX_NAMESPACE_BEGIN

BuiltinPredicate::BuiltinPredicate(const Term& t1,
				   const Term& b,
				   const Term& t2)
  : builtin(b)
{
  args.push_back(t1);
  args.push_back(t2);
}


int
BuiltinPredicate::compare(const BaseAtom& atom2) const
{
  const std::type_info& type1 = typeid(*this);
  const std::type_info& type2 = typeid(atom2);

  if (type1 == type2)
    {
      const BuiltinPredicate& bp2 = static_cast<const BuiltinPredicate&>(atom2);

      int b = builtin.compare(bp2.builtin);

      if (b == 0)
	{
	  int l = args[0].compare(bp2.args[0]);
	  int r = args[1].compare(bp2.args[1]);

	  return l ? l : r;
	}

      return b;
    }
  return type1.before(type2) ? -1 : 1;
}


const Term&
BuiltinPredicate::getPredicate() const
{
  return builtin;
}
	

void
BuiltinPredicate::setPredicate(const Term& b)
{
  builtin = b;
}


const Tuple&
BuiltinPredicate::getArguments() const
{
  return args;
}


void
BuiltinPredicate::setArguments(const Tuple& nargs)
{
  assert(nargs.size() == 2);
  args = nargs;
}


const Term&
BuiltinPredicate::operator[] (unsigned i) const
{
  assert(i < 2);
  if (i == 0)
    {
      return builtin;
    }
  return args[i];
}

 
Term&
BuiltinPredicate::operator[] (unsigned i)
{
  assert(i < 2);
  if (i == 0)
    {
      return builtin;
    }
  return args[i];
}
	

unsigned
BuiltinPredicate::getArity() const
{
  return 2;
}


bool
BuiltinPredicate::unifiesWith(const BaseAtom&) const
{
  ///@todo implement me
  return false;
}


bool
BuiltinPredicate::isGround() const
{
  ///@todo implement me
  return false;
}


void
BuiltinPredicate::accept(BaseVisitor* const v)
{
  v->visit(this);
}


DLVHEX_NAMESPACE_END


// Local Variables:
// mode: C++
// End:
