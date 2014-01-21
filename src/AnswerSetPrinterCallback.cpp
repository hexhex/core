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
 * @file   AnswerSetPrinterCallback.cpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Implementation of default answer set printer callback.
 */

#include "dlvhex2/AnswerSetPrinterCallback.h"

// activate benchmarking if activated by configure option --enable-debug
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "dlvhex2/Benchmarking.h"
#include "dlvhex2/AnswerSet.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/PredicateMask.h"

DLVHEX_NAMESPACE_BEGIN

AnswerSetPrinterCallback::AnswerSetPrinterCallback(ProgramCtx& ctx)
{
  RegistryPtr reg = ctx.registry();

  if( !ctx.config.getFilters().empty() )
  {
    filterpm.reset(new PredicateMask);

    // setup mask with registry
    filterpm->setRegistry(reg);

    // setup mask with predicates
    std::vector<std::string>::const_iterator it;
    for(it = ctx.config.getFilters().begin();
        it != ctx.config.getFilters().end(); ++it)
    {
      // retrieve/register ID for this constant
      ID pred = reg->storeConstantTerm(*it);
      filterpm->addPredicate(pred);
    }
  }
}

bool AnswerSetPrinterCallback::operator()(
    AnswerSetPtr as)
{
  DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"AnswerSetPrinterCallback");

  // uses the Registry to print the interpretation, including
  // possible influence from AuxiliaryPrinter objects (if any are registered)

  Interpretation::Storage::enumerator it, it_end;

  RegistryPtr reg = as->interpretation->getRegistry();
  Interpretation::Storage filteredbits; // must be in this scope!
  if( !filterpm )
  {
    const Interpretation::Storage& bits =
      as->interpretation->getStorage();
    it = bits.first();
    it_end = bits.end();
  }
  else
  {
    filterpm->updateMask();
    filteredbits =
      as->interpretation->getStorage() & filterpm->mask()->getStorage();
    it = filteredbits.first();
    it_end = filteredbits.end();
  }

  std::ostream& o = std::cout;

  #warning TODO think about more efficient printing
  o << '{';
  if( it != it_end )
  {
    bool gotOutput =
      reg->printAtomForUser(o, *it);
    //DBGLOG(DBG,"printed with prefix ''  and output " << gotOutput << " " <<
    //    printToString<RawPrinter>(ID(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, *it), reg));
    it++;
    for(; it != it_end; ++it)
    {
      if( gotOutput )
      {
        gotOutput |=
          reg->printAtomForUser(o, *it, ",");
        //DBGLOG(DBG,"printed with prefix ',' and output " << gotOutput << " " <<
        //    printToString<RawPrinter>(ID(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, *it), reg));
      }
      else
      {
        gotOutput |=
          reg->printAtomForUser(o, *it);
        //DBGLOG(DBG,"printed with prefix ''  and output " << gotOutput << " " <<
        //    printToString<RawPrinter>(ID(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, *it), reg));
      }
    }
  }
  o << '}';
  as->printWeightVector(o);
  o << std::endl;

  // never abort
  return true;
}

bool AnswerSetPrinterCallback::operator()(
    HTModelPtr model)
{
  DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"AnswerSetPrinterCallback");

  // uses the Registry to print the interpretation, including
  // possible influence from AuxiliaryPrinter objects (if any are registered)

  HTInterpretation::Storage::enumerator h_it, h_it_end, t_it, t_it_end;

  RegistryPtr reg = model->interpretation->registry();
  HTInterpretation::Storage filteredbits; // must be in this scope!
  if( !filterpm )
  {
    const HTInterpretation::Storage& h_bits =
      model->interpretation->here();
    h_it = h_bits.first();
    h_it_end = h_bits.end();
    const HTInterpretation::Storage& t_bits =
      model->interpretation->there();
    t_it = t_bits.first();
    t_it_end = t_bits.end();
  }
  else
  {
	// TODO: filter
  #if 0
    filterpm->updateMask();
    filteredbits =
      as->interpretation->getStorage() & filterpm->mask()->getStorage();
    it = filteredbits.first();
    it_end = filteredbits.end();
  #endif
  }

  std::ostream& o = std::cout;

  #warning TODO think about more efficient printing
  o << "({";
  if( h_it != h_it_end )
  {
    bool gotOutput =
      reg->printAtomForUser(o, *h_it);
    h_it++;
    for(; h_it != h_it_end; ++h_it)
    {
      if( gotOutput )
      {
        gotOutput |=
          reg->printAtomForUser(o, *h_it, ",");
      }
      else
      {
        gotOutput |=
          reg->printAtomForUser(o, *h_it);
      }
    }
  }
  o << "},{";
  if( t_it != t_it_end )
  {
    bool gotOutput =
      reg->printAtomForUser(o, *t_it);
   	t_it++;
    for(; t_it != t_it_end; ++t_it)
    {
      if( gotOutput )
      {
        gotOutput |=
          reg->printAtomForUser(o, *t_it, ",");
      }
      else
      {
        gotOutput |=
          reg->printAtomForUser(o, *t_it);
      }
    }
  }
  o << "})";
  o << std::endl;

  // never abort
  return true;
}

DLVHEX_NAMESPACE_END

