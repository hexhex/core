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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "dlvhex2/PredicateMask.h"
#include "dlvhex2/Interpretation.h"
#include "dlvhex2/Logger.h"
#include "dlvhex2/Printhelpers.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/OrdinaryAtomTable.h"
#include "dlvhex2/PluginInterface.h"

#include <boost/foreach.hpp>

DLVHEX_NAMESPACE_BEGIN

PredicateMask::PredicateMask():
  maski(),
  knownAddresses(0)
{
}

PredicateMask::PredicateMask(const PredicateMask& other):
  predicates(other.predicates),
  maski(other.maski),
  knownAddresses(other.knownAddresses),
  updateMutex() // must not copy mutex!
{
  if( !!other.maski )
    LOG(WARNING,"copied PredicateMask with non-NULL mask!");
}

PredicateMask&
PredicateMask::operator=(const PredicateMask& other)
{
  predicates = other.predicates;
  maski = other.maski;
  knownAddresses = other.knownAddresses;
  // must not copy mutex!
  if( !!other.maski )
    LOG(WARNING,"assigned PredicateMask with non-NULL mask!");
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
  DBGLOG_VSCOPE(DBG,"PM::uM",this,false);
  DBGLOG(DBG,"= PredicateMask::updateMask for predicates " <<
      printset(predicates));

  assert(!!maski);
  RegistryPtr reg = maski->getRegistry();
  Interpretation::Storage& bits = maski->getStorage();

  unsigned maxaddr = 0;
  OrdinaryAtomTable::AddressIterator it_begin;
  {
    // get one state of it_end, encoded in maxaddr
    // (we must not use it_end, as it is a generic object but denotes
    // different endpoints if ogatoms changes during this method)
    OrdinaryAtomTable::AddressIterator it_end;
    boost::tie(it_begin, it_end) = reg->ogatoms.getAllByAddress();
    maxaddr = it_end - it_begin;
  }

  boost::mutex::scoped_lock lock(updateMutex);

  // check if we have unknown atoms
  DBGLOG(DBG,"already inspected ogatoms with address < " << knownAddresses <<
      ", iterator range has size " << maxaddr);
  if( maxaddr == knownAddresses )
    return;

  // if not equal, it must be larger -> we must inspect
  assert(maxaddr > knownAddresses);

  // advance iterator to first ogatom unknown to predicateInputMask
  OrdinaryAtomTable::AddressIterator it = it_begin;
  it += knownAddresses;

  unsigned missingBits = maxaddr - knownAddresses;
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
  for(;missingBits != 0; it++, missingBits--)
  {
    assert(it != reg->ogatoms.getAllByAddress().second);
    const OrdinaryAtom& oatom = *it;
    //DBGLOG(DBG,"checking " << oatom.tuple.front());
    IDAddress addr = oatom.tuple.front().address;
    if( predicates.find(addr) != predicates.end() )
    {
      bits.set(it - it_begin);
    }
  }
  knownAddresses += missingBits;
  DBGLOG(DBG,"updateMask created new set of relevant ogatoms: " << *maski << " and knownAddresses is " << knownAddresses);
}

ExternalAtomMask::ExternalAtomMask() : PredicateMask(), eatom(0){
}

ExternalAtomMask::~ExternalAtomMask(){
}

void ExternalAtomMask::setEAtom(const ExternalAtom& eatom, const std::vector<ID>& groundidb){

    this->eatom = &eatom;
    setRegistry(eatom.pluginAtom->getRegistry());

    // mask contains input (predicate and constant) and output of the external atom
    if (eatom.auxInputPredicate != ID_FAIL){
      DBGLOG(DBG, "Adding auxiliary input predicate");
      addPredicate(eatom.auxInputPredicate);
    }
    int i = 0;
    BOOST_FOREACH (ID p, eatom.inputs){
      if (eatom.pluginAtom->getInputType(i++) == PluginAtom::PREDICATE){
        DBGLOG(DBG, "Adding input predicate " << p);
        addPredicate(p);
      }
    }
    ID posreplacement = eatom.pluginAtom->getRegistry()->getAuxiliaryConstantSymbol('r', eatom.predicate);
    ID negreplacement = eatom.pluginAtom->getRegistry()->getAuxiliaryConstantSymbol('n', eatom.predicate);
    // find all output atoms which possibly belong to this external atom
    BOOST_FOREACH (ID rId, groundidb){
      const Rule& rule = eatom.pluginAtom->getRegistry()->rules.getByID(rId);
      BOOST_FOREACH (ID h, rule.head){
        if (h.isExternalAuxiliary()){
          const OrdinaryAtom& atom = eatom.pluginAtom->getRegistry()->ogatoms.getByID(h);
          if (atom.tuple[0] == posreplacement || atom.tuple[0] == negreplacement){
            outputAtoms.insert(h.address);
          }
        }
      }
      BOOST_FOREACH (ID b, rule.body){
        if (b.isExternalAuxiliary()){
          const OrdinaryAtom& atom = eatom.pluginAtom->getRegistry()->ogatoms.getByID(b);
          if (atom.tuple[0] == posreplacement || atom.tuple[0] == negreplacement){
            outputAtoms.insert(b.address);
          }
        }
      }
    }
    DBGLOG(DBG, "Watching " << outputAtoms.size() << " output atoms");

    BOOST_FOREACH (IDAddress outputAtom, outputAtoms){
      const OrdinaryAtom& oatom = eatom.pluginAtom->getRegistry()->ogatoms.getByAddress(outputAtom);
      if (matchOutputAtom(oatom.tuple)){
        DBGLOG(DBG, "Output atom " << outputAtom << " matches the external atom");
        maski->setFact(outputAtom);
      }else{
        DBGLOG(DBG, "Output atom " << outputAtom << " does not match the external atom");
      }
    }
}

bool ExternalAtomMask::matchOutputAtom(const Tuple& togatom){

    assert(eatom);

#ifndef NDEBUG
    std::stringstream ss;
    ss << "Comparing togatom tuple (";
    for (int i = 0; i < togatom.size(); ++i){
      ss << (i > 0 ? ", " : "");
      if (togatom[i].isIntegerTerm()){
        ss << togatom[i].address;
      }else{
        ss << eatom->pluginAtom->getRegistry()->terms.getByID(togatom[i]).symbol;
      }
    }
    ss << ") to external atom " << eatom->pluginAtom->getRegistry()->terms.getByID(eatom->predicate).symbol << " (input: ";
    for (int i = 0; i < eatom->inputs.size(); ++i){
      ss << (i > 0 ? ", " : "");
      if (eatom->inputs[i].isIntegerTerm()){
        ss << eatom->inputs[i].address;
      }else{
        ss << eatom->pluginAtom->getRegistry()->terms.getByID(eatom->inputs[i]).symbol;
      }
    }
    ss << "; output: ";
    for (int i = 0; i < eatom->tuple.size(); ++i){
      ss << (i > 0 ? ", " : "");
      if (eatom->tuple[i].isIntegerTerm()){
        ss << eatom->tuple[i].address;
      }else{
        ss << eatom->pluginAtom->getRegistry()->terms.getByID(eatom->tuple[i]).symbol;
      }
    }
    ss << ")";
    DBGLOG(DBG, ss.str());
#endif

    std::map<ID, ID> varBinding;

    // check predicate and constant input
    for (int p = 0; p < eatom->inputs.size(); ++p){
      if (eatom->pluginAtom->getInputType(p) == PluginAtom::PREDICATE ||
          eatom->pluginAtom->getInputType(p) == PluginAtom::CONSTANT && !eatom->inputs[p].isVariableTerm()){
        if (togatom[p + 1] != eatom->inputs[p]){
          DBGLOG(DBG, "Predicate or constant input mismatch");
          return false;
        }
      }
    }

    // check auxiliary input
    bool inputmatch = false;
    if (eatom->auxInputPredicate == ID_FAIL){
      inputmatch = true;
    }else{
      BOOST_FOREACH (Tuple tinp, auxInputTuples){
        // check if tinp corresponds to togatom
        bool match = true;
        for (int i = 0; i < tinp.size(); ++i){
          BOOST_FOREACH (unsigned pos, eatom->auxInputMapping[i]){
            if (togatom[1 + pos] != tinp[i]){
              match = false;
              break;
            }
            // remember matched variables
            varBinding[eatom->inputs[pos]] = tinp[i];
          }
          if (!match) break;
        }
        if (match){
          inputmatch = true;
          break;
        }
      }
      if (!inputmatch) DBGLOG(DBG, "Auxiliary input mismatch");
    }
    if (!inputmatch) return false;

    // check output tuple
    for (int o = 0; o < eatom->tuple.size(); ++o){
      if (eatom->tuple[o].isVariableTerm()){
        if (varBinding.find(eatom->tuple[o]) == varBinding.end()){
          varBinding[eatom->tuple[o]] = togatom[eatom->inputs.size() + o];
        }else{
          if (varBinding[eatom->tuple[o]] != togatom[eatom->inputs.size() + o]){
            return false;
          }
        }
      }else if (eatom->tuple[o].isConstantTerm()){
        if (togatom[eatom->inputs.size() + o] != eatom->tuple[0]){
          return false;
        }
      }else{
        assert(false);
      }
    }
}

  // this method ensures that the mask captures:
  //  * predicate input of the external atom
  //  * auxiliary input to the external atom
  //  * all ground output atoms (replacements) of the external atom
  //      this is the set of all ogatoms which use the (positive or negative) auxiliary of this external atom
  //      and where the input list matches
  //
  // the update strategy is as follows:
  // 1. update as usual
  // 2. if an auxiliary input atom was added, consider all ogatoms over the positive or negative replacement as new
  // 3. for all new ogatoms over the positive or negative replacement, check if the input list matches (if not: remove the atom from the mask)
void ExternalAtomMask::updateMask(){

    assert(eatom);
    DBGLOG(DBG, "ExternalAtomMask::updateMask");

    // remember what changed
    InterpretationPtr change = InterpretationPtr(new Interpretation(eatom->pluginAtom->getRegistry()));
    change->getStorage() |= maski->getStorage();
    PredicateMask::updateMask();
    change->getStorage() ^= maski->getStorage();

    if (change->getStorage().count() == 0) return;

    // check if an atom over the auxiliary input predicate was added
    bool auxAdded = false;
    bm::bvector<>::enumerator en = change->getStorage().first();
    bm::bvector<>::enumerator en_end = change->getStorage().end();
    while (en < en_end){
      const OrdinaryAtom& oatom = eatom->pluginAtom->getRegistry()->ogatoms.getByAddress(*en);
      if (oatom.tuple[0] == eatom->auxInputPredicate){
        //remember the auxiliary input tuple
        Tuple inp = oatom.tuple;
        inp.erase(inp.begin());
        auxInputTuples.push_back(inp);
        auxAdded = true;
      }
      en++;
    }

    // if an auxiliary input atom was added, we have to recheck all output atoms
    if (auxAdded){
      DBGLOG(DBG, "Auxiliary input changed");
      // recheck all output atoms
      BOOST_FOREACH (IDAddress outputAtom, outputAtoms){
        const OrdinaryAtom& oatom = eatom->pluginAtom->getRegistry()->ogatoms.getByAddress(outputAtom);
        if (matchOutputAtom(oatom.tuple)){
          DBGLOG(DBG, "Output atom " << outputAtom << " matches the external atom");
          maski->setFact(outputAtom);
        }else{
          DBGLOG(DBG, "Output atom " << outputAtom << " does not match the external atom");
        }
      }
    }
}

const std::vector<Tuple>& ExternalAtomMask::getAuxInputTuples() const{
    return auxInputTuples;
}

DLVHEX_NAMESPACE_END


// Local Variables:
// mode: C++
// End:
