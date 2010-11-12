/* dlvhex-mcs-equilibrium-plugin
 * Calculate Equilibrium Semantics of Multi Context Systems in dlvhex
 *
 * Copyright (C) 2009,2010  Markus Boegl
 * 
 * This file is part of dlvhex-mcs-equilibrium-plugin.
 *
 * dlvhex-mcs-equilibrium-plugin is free software; 
 * you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * dlvhex-mcs-equilibrium-plugin is distributed in the hope that it will
 *  be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with dlvhex-dlplugin; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */


/**
 * @file   OutputRewriter.h
 * @author Markus Boegl
 * @date   Sun Jan 24 13:40:51 2010
 * 
 * @brief  OutputRewriter to extract diagnosis, output projected equilibria,
 *         calculate explanations and print them
 */
#ifndef _DLVHEX_MCSDIAGEXPL_OUTPUTREWRITER_H_
#define _DLVHEX_MCSDIAGEXPL_OUTPUTREWRITER_H_

#include "dlvhex/OutputBuilder.h"
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_comparison.hpp>
#include "dlvhex/AtomSet.h"


namespace dlvhex {
  namespace mcsdiagexpl {

  typedef std::list< boost::tuple<AtomSet,AtomSet,AnswerSetPtr> > ResultList;

  class OutputRewriter : public OutputBuilder {

    private:
      bool checkAddMinimalResult(ResultList& rs, AtomSet& d1, AtomSet& d2);
      std::vector<AtomSet> getExplaination(ResultList& minRes);

    public:
      /// Dtor
      virtual
      ~OutputRewriter() {};

      /// Ctor
      OutputRewriter() {};

      virtual void
      buildResult(std::ostream&, const ResultContainer&);
   };

  } // END namespace mcsdiagexpl
} // END namespace dlvhex

#endif // _DLVHEX_MCSDIAGEXPL_OUTPUTREWRITER_H_
