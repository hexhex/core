/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2008 Thomas Krennwallner
 * 
 * This file is part of dlvhex.
 *
 * dlvhex is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * dlvhex is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with dlvhex; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */


/**
 * @file EvaluateExtatom.h
 * @author Thomas Krennnwallner
 * @date Sat Jan 19 20:23:12 CEST 2008
 *
 * @brief Evaluate external atoms.
 *
 *
 */


#if !defined(_DLVHEX_EVALUATEEXTATOM_H)
#define _DLVHEX_EVALUATEEXTATOM_H

#include "dlvhex/PlatformDefinitions.h"

#include "dlvhex/ExternalAtom.h"
#include "dlvhex/AtomSet.h"
#include "dlvhex/Term.h"
#include "dlvhex/PluginContainer.h"

#include <vector>

DLVHEX_NAMESPACE_BEGIN

/**
 * @brief 
 */
class DLVHEX_EXPORT EvaluateExtatom
{
 public:
  EvaluateExtatom(const ExternalAtom*, PluginContainer&);
  
  /**
   */
  void evaluate(const AtomSet&, AtomSet&) const;
  

protected:
  void groundInputList(const AtomSet&, std::vector<Tuple>&) const;

  const ExternalAtom* externalAtom;

  PluginContainer& container;
};


DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_EVALUATEEXTATOM_H */


// Local Variables:
// mode: C++
// End:
