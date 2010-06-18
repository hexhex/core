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
#include "dlvhex/Error.h"
#include "dlvhex/AtomSet.h"
#include "dlvhex/Term.h"
#include "dlvhex/PluginContainer.h"

#include <vector>

DLVHEX_NAMESPACE_BEGIN


/**
 * @brief Evaluates the external atom w.r.t. to an interpretation.
 *
 * The prediate name of the returned ground atoms will be the replacement
 * name of this atom.  What the evaluation object basically does, is to
 * pass the list of ground input parameters and part of the interpretation
 * to the plugin and let it evaluate its external atom function there. The
 * ground input parameters are either the ones originally specified in the
 * hex-program or produced from auxiliary predicates if they were
 * non-ground. The passed part of the interpretation is determined by those
 * input parameters that are of type PREDICATE.
 */
class DLVHEX_EXPORT EvaluateExtatom
{
 public:
  EvaluateExtatom(const ExternalAtom*, PluginContainer&);
  
  /**
   */
  void evaluate(const AtomSet&, AtomSet&) const throw(PluginError);
  

protected:
  /**
   * @brief Grounds the input arguments w.r.t. a specified interpretation.
   *
   * If the input list of an external atom in a hex-program is not
   * completely ground, auxiliary predicates are generated, grounding
   * the list from the remaining body atoms. This function creates
   * ground tuples from the auxiliary atoms in the interpretation or
   * simply returns the original input list, if it was fully ground.
   */
  void
  groundInputList(const AtomSet&, std::vector<Tuple>&) const;

  /// evaluate this ExternalAtom
  const ExternalAtom* externalAtom;

  /// container holds all known external atoms
  PluginContainer& container;
};


DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_EVALUATEEXTATOM_H */


// Local Variables:
// mode: C++
// End:
