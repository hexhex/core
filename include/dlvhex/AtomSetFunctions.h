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
 * @file   AtomSet.h
 * @author Roman Schindlauer
 * @author Thomas Krennwallner
 * @date   Tue Feb  7 17:20:32 CET 2006
 * 
 * @brief  AtomSet class.
 * @todo   Introduce a proper Interpretation class.
 * 
 */

#if !defined(_DLVHEX_ATOMSETFUNCTIONS_H)
#define _DLVHEX_ATOMSETFUNCTIONS_H

#include "dlvhex/PlatformDefinitions.h"

#include "dlvhex/AtomSet.h"

#include <algorithm>
#include <iterator>

DLVHEX_NAMESPACE_BEGIN

AtomSet
matchPredicate(const AtomSet& as, const Term& p)
{
  AtomSet tmp;

  for (AtomSet::const_iterator it = as.begin(); it != as.end(); ++it)
    {
      if ((*it)->getPredicate() == p)
	{
	  tmp.insert(*it);
	}
    }

  return tmp;
}


AtomSet
removePredicate(const AtomSet& as, const Term& p)
{
  AtomSet tmp;

  for (AtomSet::const_iterator it = as.begin(); it != as.end(); ++it)
    {
      if ((*it)->getPredicate() != p)
	{
	  tmp.insert(*it);
	}
    }

  return tmp;
}


AtomSet
difference(const AtomSet& as1, const AtomSet& as2)
{
  AtomSet tmp;
  
  std::set_difference(as1.begin(), as1.end(),
		      as2.begin(), as2.end(),
		      std::inserter(tmp, tmp.begin()));

  return tmp;
}


AtomSet
keepPositive(const AtomSet& as)
{
  AtomSet tmp;

  for (AtomSet::const_iterator it = as.begin(); it != as.end(); ++it)
    {
      if (typeid(**it) == typeid(Atom<Positive>))
	{
	  tmp.insert(*it);
	}
    }

  return tmp;
}


AtomSet
filterPredicates(const AtomSet& as, const std::vector<std::string>& predicates)
{
  AtomSet tmp;

  for (AtomSet::const_iterator it = as.begin(); it != as.end(); ++it)
    {
      const std::string& p = (*it)->getPredicate().getString();

      if (std::find(predicates.begin(), predicates.end(), p) != predicates.end())
	{
	  tmp.insert(*it);
	}
    }

  return tmp;
}

DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_ATOMSETFUNCTIONS_H */


// Local Variables:
// mode: C++
// End:
