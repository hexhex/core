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
 * @file   PredicateMask.cpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Incrementally managed bitmask for projecting ground
 *         interpretations to certain predicates.
 */

#include "dlvhex/PredicateMask.hpp"
#include "dlvhex/Interpretation.hpp"
#include "dlvhex/Logger.hpp"
#include "dlvhex/Printhelpers.hpp"
#include "dlvhex/Registry.hpp"
#include "dlvhex/Printer.hpp"
#include "dlvhex/OrdinaryAtomTable.hpp"

#include <boost/foreach.hpp>

DLVHEX_NAMESPACE_BEGIN

PredicateMask::PredicateMask():
  maski(),
  knownAddresses(0)
{
}

PredicateMask::~PredicateMask()
{
}

void PredicateMask::setRegistry(RegistryPtr reg)
{
  assert((!maski || maski->getRegistry() == reg) && "PredicateMask cannot change registry!");
  if( !maski )
  {
    maski.reset(new Interpretation(reg));
  }
}

void PredicateMask::addPredicate(ID pred)
{
  assert(knownAddresses == 0 && "TODO implement incremental addition of predicates to mask"); // should be easy
  assert(pred.isTerm() && pred.isConstantTerm() && "predicate masks can only be done on constant terms");
  predicates.insert(pred.address);
}

void PredicateMask::updateMask()
{
  DBGLOG_VSCOPE(DBG,"PM::uM",this,true);
  DBGLOG(DBG,"= PredicateMask::updateMask for predicates " <<
      printset(predicates));

  assert(!!maski);
  RegistryPtr reg = maski->getRegistry();
  Interpretation::Storage& bits = maski->getStorage();

  // get range over all ogatoms
  OrdinaryAtomTable::AddressIterator it_begin, it, it_end;
  boost::tie(it_begin, it_end) = reg->ogatoms.getAllByAddress();

  // check if we have unknown atoms
  DBGLOG(DBG,"already inspected ogatoms with address < " << knownAddresses <<
      ", iterator range has size " << (it_end - it_begin));
  if( (it_end - it_begin) == knownAddresses )
    return;
  // if not equal, it must be larger -> we must inspect
  assert((it_end - it_begin) > knownAddresses);

  // advance iterator to first ogatom unknown to predicateInputMask
  it = it_begin;
  it += knownAddresses;

  unsigned missingBits = it_end - it;
  DBGLOG(DBG,"need to inspect " << missingBits << " missing bits");

  // check all new ogatoms till the end
  #ifndef NDEBUG
  {
    std::stringstream s;
    s << "relevant predicate constants are ";
    RawPrinter printer(s, reg);
    BOOST_FOREACH(IDAddress addr, predicates)
    {
      printer.print(ID(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, addr));
      s << ", ";
    }
    DBGLOG(DBG,s.str());
  }
  #endif
  assert(knownAddresses == (it - it_begin));
  for(;it != it_end; ++it)
  {
    const OrdinaryAtom& oatom = *it;
    //DBGLOG(DBG,"checking " << oatom.tuple.front());
    IDAddress addr = oatom.tuple.front().address;
    if( predicates.find(addr) != predicates.end() )
    {
      bits.set(it - it_begin);
    }
  }
  DBGLOG(DBG,"updateMask created new set of relevant ogatoms: " << *maski);
}

DLVHEX_NAMESPACE_END


// Local Variables:
// mode: C++
// End:
