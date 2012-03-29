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
 * @file BaseModelGenerator.cpp
 * @author Peter Schüller
 *
 * @brief Implementation of common model generator functionalities.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "dlvhex2/BaseModelGenerator.h"
#include "dlvhex2/Logger.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/ASPSolver.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/PluginInterface.h"
#include "dlvhex2/Benchmarking.h"
#include "dlvhex2/Atoms.h"

#include <boost/foreach.hpp>

DLVHEX_NAMESPACE_BEGIN

BaseModelGenerator::ExternalAnswerTupleCallback::
~ExternalAnswerTupleCallback()
{
}

BaseModelGenerator::
IntegrateExternalAnswerIntoInterpretationCB::
IntegrateExternalAnswerIntoInterpretationCB(
    InterpretationPtr outputi):
  outputi(outputi),
  reg(outputi->getRegistry()),
  replacement(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG | ID::PROPERTY_AUX)
{
}

bool
BaseModelGenerator::IntegrateExternalAnswerIntoInterpretationCB::
eatom(const ExternalAtom& eatom)
{
  replacement.tuple.resize(1);
  replacement.tuple[0] = 
    reg->getAuxiliaryConstantSymbol('r', eatom.predicate);

  // never abort
  return true;
}

// remembers input
bool
BaseModelGenerator::IntegrateExternalAnswerIntoInterpretationCB::
input(const Tuple& input)
{
  assert(replacement.tuple.size() >= 1);
  // shorten
  replacement.tuple.resize(1);
  // add
  replacement.tuple.insert(replacement.tuple.end(),
      input.begin(), input.end());

  // never abort
  return true;
}

// creates replacement ogatom and activates respective bit in output interpretation
bool
BaseModelGenerator::IntegrateExternalAnswerIntoInterpretationCB::
output(const Tuple& output)
{
  assert(replacement.tuple.size() >= 1);
  // add, but remember size to reset it later
  unsigned size = replacement.tuple.size();
  replacement.tuple.insert(replacement.tuple.end(),
      output.begin(), output.end());

  // this replacement might already exists
  LOG(DBG,"integrating eatom tuple " << printrange(replacement.tuple));
  ID idreplacement = reg->storeOrdinaryGAtom(replacement);
  DBGLOG(DBG,"got replacement ID " << idreplacement);
  outputi->setFact(idreplacement.address);
  DBGLOG(DBG,"output interpretation is now " << *outputi);

  // shorten it, s.t. we can add the next one
  replacement.tuple.resize(size);

  // never abort
  return true;
}

BaseModelGenerator::VerifyExternalAnswerAgainstPosNegGuessInterpretationCB::
VerifyExternalAnswerAgainstPosNegGuessInterpretationCB(
    InterpretationPtr _guess_pos,
    InterpretationPtr _guess_neg):
  reg(_guess_pos->getRegistry()),
  guess_pos(_guess_pos),
  guess_neg(_guess_neg),
  replacement(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG | ID::PROPERTY_AUX)
{
  assert(guess_pos->getRegistry() == guess_neg->getRegistry());
}

bool
BaseModelGenerator::VerifyExternalAnswerAgainstPosNegGuessInterpretationCB::
eatom(const ExternalAtom& eatom)
{
  pospred = 
    reg->getAuxiliaryConstantSymbol('r', eatom.predicate);
  negpred =
    reg->getAuxiliaryConstantSymbol('n', eatom.predicate);
  replacement.tuple.resize(1);

  // never abort
  return true;
}

bool
BaseModelGenerator::VerifyExternalAnswerAgainstPosNegGuessInterpretationCB::
input(const Tuple& input)
{
  assert(replacement.tuple.size() >= 1);

  // shorten
  replacement.tuple.resize(1);

  // add
  replacement.tuple.insert(replacement.tuple.end(),
      input.begin(), input.end());

  // never abort
  return true;
}

bool
BaseModelGenerator::VerifyExternalAnswerAgainstPosNegGuessInterpretationCB::
output(const Tuple& output)
{
  assert(replacement.tuple.size() >= 1);

  // add, but remember size to reset it later
  unsigned size = replacement.tuple.size();
  replacement.tuple.insert(replacement.tuple.end(),
      output.begin(), output.end());

  // build pos replacement, register, and clear the corresponding bit in guess_pos
  replacement.tuple[0] = pospred;
  ID idreplacement_pos = reg->storeOrdinaryGAtom(replacement);
  DBGLOG(DBG,"pos replacement ID = " << idreplacement_pos);
  if( !guess_pos->getFact(idreplacement_pos.address) )
  {
    // check whether neg is true, if yes we bailout
    replacement.tuple[0] = negpred;
    ID idreplacement_neg = reg->ogatoms.getIDByTuple(replacement.tuple);
    if( idreplacement_neg == ID_FAIL )
    {
      // this is ok, the negative replacement does not exist so it cannot be true
      DBGLOG(DBG,"neg eatom replacement " << replacement << " not found -> not required");
    }
    else
    {
      DBGLOG(DBG,"neg eatom replacement ID = " << idreplacement_neg);

      // verify if it is true or not
      if( guess_neg->getFact(idreplacement_neg.address) == true )
      {
        // this is bad, the guess was "false" but the eatom output says it is "true"
        // -> abort
        DBGLOG(DBG,"neg eatom replacement is true in guess -> wrong guess!");

        // (we now that we won't reuse replacement.tuple,
        //  so we do not care about resizing it here)
        return false;
      }
      else
      {
        // this is ok, the negative replacement exists but is not true
        DBGLOG(DBG,"neg eatom replacement found but not set -> ok");
      }
    }
  }
  else
  {
    // remove this bit, so later we can check if all bits were cleared
    // (i.e., if all positive guesses were confirmed)
    guess_pos->clearFact(idreplacement_pos.address);
    DBGLOG(DBG,"clearing replacement fact -> positive guess interpretation is now " << *guess_pos);
  }

  // shorten it, s.t. we can add the next one
  replacement.tuple.resize(size);

  // do not abort if we reach here
  return true;
}

std::set<ID> BaseModelGenerator::getPredicates(const RegistryPtr reg, InterpretationConstPtr edb, const std::vector<ID>& idb){

	std::set<ID> preds;

	// collects edb predicates
	bm::bvector<>::enumerator en = edb->getStorage().first();
	bm::bvector<>::enumerator en_end = edb->getStorage().end();
	while (en < en_end){
		// check if the predicate is relevant
		const OrdinaryAtom& ogatom = reg->ogatoms.getByID(ID(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, *en));
		preds.insert(ogatom.tuple.front());
		en++;
	}

	// collects idb predicates
	BOOST_FOREACH (ID ruleID, idb){
		assert(ruleID.isRule());
		const Rule& rule = reg->rules.getByID(ruleID);

		// head
		BOOST_FOREACH (ID atomID, rule.head){
			const OrdinaryAtom& atom = atomID.isOrdinaryGroundAtom() ? reg->ogatoms.getByID(atomID) : reg->onatoms.getByID(atomID);
			preds.insert(atom.tuple.front());
		}

		// body
		BOOST_FOREACH (ID atomID, rule.body){
			if (atomID.isOrdinaryAtom()){
				const OrdinaryAtom& atom = atomID.isOrdinaryGroundAtom() ? reg->ogatoms.getByID(atomID) : reg->onatoms.getByID(atomID);
				preds.insert(atom.tuple.front());
			}
			if (atomID.isExternalAtom()){
				const ExternalAtom& atom = reg->eatoms.getByID(atomID);
				// go through predicate input parameters
				int i = 0;
				BOOST_FOREACH (ID param, atom.tuple){
					if (atom.pluginAtom->getInputType(i++) == PluginAtom::PREDICATE){
						preds.insert(param);
					}
				}
			}
		}
	}

#ifndef NDEBUG
	std::stringstream ss;
	ss << "Relevant predicates: ";
	bool first = true;
	BOOST_FOREACH (ID p, preds){
		if (!first) ss << ", ";
		first = false;
		ss << p;
	}
	DBGLOG(DBG, ss.str());
#endif

	return preds;
}

InterpretationPtr BaseModelGenerator::restrictInterpretationToPredicates(const RegistryPtr reg, InterpretationConstPtr intr, const std::set<ID>& predicates){

	InterpretationPtr ointr = InterpretationPtr(new Interpretation(reg));

	// go through ground atoms in interpretation
	bm::bvector<>::enumerator en = intr->getStorage().first();
	bm::bvector<>::enumerator en_end = intr->getStorage().end();
	while (en < en_end){
		// check if the predicate is relevant
		const OrdinaryAtom& atom = reg->ogatoms.getByID(ID(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, *en));
		if (std::find(predicates.begin(), predicates.end(), atom.tuple.front()) != predicates.end()){
			// yes, set the atom
			ointr->setFact(*en);
		}
		en++;
	}
	return ointr;
}

Nogood BaseModelGenerator::interpretationToNogood(InterpretationConstPtr intr, NogoodContainer& ngContainer){

	Nogood ng;

	// go through ground atoms in interpretation
	bm::bvector<>::enumerator en = intr->getStorage().first();
	bm::bvector<>::enumerator en_end = intr->getStorage().end();
	while (en < en_end){
		// add the atom to the nogood
		ng.insert(ngContainer.createLiteral(*en));
		en++;
	}

	return ng;
}

void BaseModelGenerator::globalConflictAnalysis(ProgramCtx& ctx, const std::vector<ID>& idb, GenuineSolverPtr solver, bool componentIsMonotonic){

	DBGLOG(DBG, "Global conflict analysis");
	if (solver->getModelCount() == 0 && ctx.config.getOption("GlobalLearning")){
		DBGLOG(DBG, "Contradiction on first model: Component is inconsistent wrt. input");

BOOST_FOREACH (ID id, idb){
DBGLOG(DBG, "Rule " << id);
}

		if (componentIsMonotonic){
			DBGLOG(DBG, "Component is monotonic");
			Nogood gng;
			if (input != InterpretationConstPtr()){
				gng = interpretationToNogood(restrictInterpretationToPredicates(ctx.registry(), input, getPredicates(ctx.registry(), ctx.edb, idb)), ctx.globalNogoods);
			}
			DBGLOG(DBG, "Generating global nogood " << gng);
			ctx.globalNogoods.addNogood(gng);
		}
	}
}

// projects input interpretation
// calls eatom function
// reintegrates output tuples as auxiliary atoms into outputi
// (inputi and outputi may point to the same interpretation)

bool BaseModelGenerator::evaluateExternalAtom(RegistryPtr reg,
  const ExternalAtom& eatom,
  InterpretationConstPtr inputi,
  ExternalAnswerTupleCallback& cb,
  ProgramCtx* ctx,
  NogoodContainerPtr nogoods) const
{
  LOG_SCOPE(PLUGIN,"eEA",false);
  DBGLOG(DBG,"= evaluateExternalAtom for " << eatom <<
      " with input interpretation " << *inputi);
  DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sideea,"evaluate external atom");
	DLVHEX_BENCHMARK_REGISTER(sidier,"integrate external results");

  // build input interpretation
  // for each input tuple (multiple auxiliary inputs possible)
  //   build query
  //   call retrieve
  //   integrate answer into interpretation i as additional facts

  assert(!!eatom.pluginAtom);
  PluginAtom* pluginAtom = eatom.pluginAtom;

  // if this is wrong, we might have mixed up registries between plugin and program
  assert(eatom.predicate == pluginAtom->getPredicateID());

  // project interpretation for predicate inputs
  InterpretationConstPtr eatominp =
    projectEAtomInputInterpretation(reg, eatom, inputi);

  // build input tuples
  std::list<Tuple> inputs;
  buildEAtomInputTuples(reg, eatom, inputi, inputs);

  if( Logger::Instance().shallPrint(Logger::PLUGIN) )
  {
    LOG(PLUGIN,"eatom input tuples = ");
    LOG_INDENT(PLUGIN);
    BOOST_FOREACH(const Tuple& t, inputs)
    {
      std::stringstream s;
      RawPrinter printer(s, reg);
      s << "[";
      printer.printmany(t,",");
      s << "] ";
      LOG(PLUGIN,s.str());
    }
    {
      std::stringstream s;
      RawPrinter printer(s, reg);
      s << "(";
      printer.printmany(eatom.tuple,",");
      s << ")";
      LOG(PLUGIN,"eatom output pattern = " << s.str());
    }
    LOG(PLUGIN,"projected eatom input interpretation = " << *eatominp);
  }

  // call callback and abort if requested
  if( !inputs.empty() )
  {
    if( !cb.eatom(eatom) )
    {
      LOG(DBG,"callback aborted for eatom " << eatom);
      return false;
    }
  }

  // go over all ground input tuples as grounded by auxiliary inputs rule
  BOOST_FOREACH(const Tuple& inputtuple, inputs)
  {
    #ifndef NDEBUG
    std::string sinput;
    {
      std::stringstream s;
      RawPrinter printer(s, reg);
      s << "[";
      printer.printmany(inputtuple,",");
      s << "]";
      sinput = s.str();
    }
    DBGLOG(DBG,"processing input tuple " << sinput << " = " << printrange(inputtuple));
    #endif

    // query
    PluginAtom::Query query(eatominp, inputtuple, eatom.tuple, &eatom);
    PluginAtom::Answer answer;
    pluginAtom->retrieveCached(query, answer, ctx, nogoods);
    LOG(PLUGIN,"got " << answer.get().size() << " answer tuples");

    if( !answer.get().empty() )
    {
      if( !cb.input(inputtuple) )
      {
        LOG(DBG,"callback aborted for input tuple " << printrange(inputtuple));
        return false;
      }
    }

    DLVHEX_BENCHMARK_SCOPE(sidier);

    // integrate result into interpretation
    BOOST_FOREACH(const Tuple& t, answer.get())
    {
      if( Logger::Instance().shallPrint(Logger::PLUGIN) )
      {
        std::stringstream s;
        RawPrinter printer(s, reg);
        s << "(";
        printer.printmany(t,",");
        s << ")";
        LOG(PLUGIN,"got answer tuple " << s.str());
      }
      if( !verifyEAtomAnswerTuple(reg, eatom, t) )
      {
        LOG(WARNING,"external atom " << eatom << " returned tuple " <<
            printrange(t) << " which does not match output pattern (skipping)");
        continue;
      }

      // call callback and abort if requested
      if( !cb.output(t) )
      {
        LOG(DBG,"callback aborted for output tuple " << printrange(t));
        return false;
      }
    }
  } // go over all input tuples of this eatom

  return true;
}

// calls evaluateExternalAtom for each atom in eatoms

bool BaseModelGenerator::evaluateExternalAtoms(RegistryPtr reg,
  const std::vector<ID>& eatoms,
  InterpretationConstPtr inputi,
  ExternalAnswerTupleCallback& cb,
  ProgramCtx* ctx,
  NogoodContainerPtr nogoods) const
{
  BOOST_FOREACH(ID eatomid, eatoms)
  {
    const ExternalAtom& eatom = reg->eatoms.getByID(eatomid);
    if( !evaluateExternalAtom(reg, eatom, inputi, cb, ctx, nogoods) )
    {
      LOG(DBG,"callbacks aborted evaluateExternalAtoms");
      return false;
    }
  }
  return true;
}

// returns false iff tuple does not unify with eatom output pattern
// (the caller must decide whether to throw an exception or ignore the tuple)
bool BaseModelGenerator::verifyEAtomAnswerTuple(RegistryPtr reg,
  const ExternalAtom& eatom, const Tuple& t) const
{
  return true;

    #warning TODO verify answer tuple! (as done in dlvhex trunk using std::mismatch)
    #if 0
    // check answer tuple, if it corresponds to pattern
    this is the respective code


    /**
     * @brief Check the answers returned from the external atom, and
     * remove ill-formed tuples.
     *
     * Check whether the answers in the output list are
     * (1) ground
     * (2) conform to the output pattern, i.e.,
     *     &rdf[uri](S,rdf:subClassOf,O) shall only return tuples of form
     *     <s, rdf:subClassOf, o>, and not for instance <s,
     *     rdf:subPropertyOf, o>, we have to filter them out (do we?)
     */
    struct CheckOutput
      : public std::binary_function<const Term, const Term, bool>
    {
      bool
      operator() (const Term& t1, const Term& t2) const
      {
        // answers must be ground, otw. programming error in the plugin
        assert(t1.isInt() || t1.isString() || t1.isSymbol());

        // pattern tuple values must coincide
        if (t2.isInt() || t2.isString() || t2.isSymbol())
          {
      return t1 == t2;
          }
        else // t2.isVariable() -> t1 is a variable binding for t2
          {
      return true;
          }
      }
    };


    for (std::vector<Tuple>::const_iterator s = answers->begin(); s != answers->end(); ++s)
    {
      if (s->size() != externalAtom->getArguments().size())
        {
          throw PluginError("External atom " + externalAtom->getFunctionName() + " returned tuple of incompatible size.");
        }

      // check if this answer from pluginatom conforms to the external atom's arguments
      std::pair<Tuple::const_iterator,Tuple::const_iterator> mismatched =
        std::mismatch(s->begin(),
          s->end(),
          externalAtom->getArguments().begin(),
          CheckOutput()
          );

      if (mismatched.first == s->end()) // no mismatch found -> add this tuple to the result
        {
          // the replacement atom contains both the input and the output list!
          // (*inputi must be ground here, since it comes from
          // groundInputList(i, inputArguments))
          Tuple resultTuple(*inputi);

          // add output list
          resultTuple.insert(resultTuple.end(), s->begin(), s->end());

          // setup new atom with appropriate replacement name
          AtomPtr ap(new Atom(externalAtom->getReplacementName(), resultTuple));

          result.insert(ap);
        }
      else
        {
          // found a mismatch, ignore this answer tuple
        }
    }
    #endif
}

InterpretationPtr BaseModelGenerator::projectEAtomInputInterpretation(RegistryPtr reg,
  const ExternalAtom& eatom, InterpretationConstPtr full) const
{
	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"BaseModelGen::projectEAII");
  eatom.updatePredicateInputMask();
  InterpretationPtr ret;
  if( full == 0 )
    ret.reset(new Interpretation(reg));
  else
    ret.reset(new Interpretation(*full));
  ret->getStorage() &= eatom.getPredicateInputMask()->getStorage();
  return ret;
}

void BaseModelGenerator::buildEAtomInputTuples(RegistryPtr reg,
  const ExternalAtom& eatom,
  InterpretationConstPtr i,
  std::list<Tuple>& inputs) const
{
	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"BaseModelGen::buildEAIT");
  LOG_SCOPE(PLUGIN,"bEAIT",false);
  DBGLOG(DBG,"= buildEAtomInputTuples " << eatom);

  // if there are no variables, there is no aux input predicate and only one input tuple
  if( eatom.auxInputPredicate == ID_FAIL )
  {
    DBGLOG(DBG,"no auxiliary input predicate -> "
        " returning single unchanged eatom.inputs " <<
        printrange(eatom.inputs));
    inputs.push_back(eatom.inputs);
    return;
  }

  // otherwise we have to calculate a bit, using the aux input predicate
  DBGLOG(DBG,"matching aux input predicate " << eatom.auxInputPredicate <<
      ", original eatom.inputs = " << printrange(eatom.inputs));
  dlvhex::OrdinaryAtomTable::PredicateIterator it, it_end;
  assert(reg != 0);
  for(boost::tie(it, it_end) =
      reg->ogatoms.getRangeByPredicateID(eatom.auxInputPredicate);
      it != it_end; ++it)
  {
    const dlvhex::OrdinaryAtom& oatom = *it;
    #warning perhaps this could be made more efficient by storing back the id into oatom or by creating ogatoms.getIDRangeByPredicateID with some projecting adapter to PredicateIterator
    ID idoatom = reg->ogatoms.getIDByStorage(oatom);
    if( i->getFact(idoatom.address) )
    {
      // add copy of original input tuple
      inputs.push_back(eatom.inputs);

      // modify this copy
      Tuple& inp = inputs.back();

      // replace all occurances of variables with the corresponding predicates in auxinput
      for(unsigned idx = 0; idx < eatom.auxInputMapping.size(); ++idx)
      {
        // idx is the index of the argument to the auxiliary predicate
        // at 0 there is the auxiliary predicate
        ID replaceBy = oatom.tuple[idx+1];
        // replaceBy is the ground term we will use instead of the input constant variable
        for(std::list<unsigned>::const_iterator it = eatom.auxInputMapping[idx].begin();
            it != eatom.auxInputMapping[idx].end(); ++it)
        {
          // *it is the index of the input term that is a variable
          assert(inp[*it].isTerm() && inp[*it].isVariableTerm());
          inp[*it] = replaceBy;
        }
      }
      DBGLOG(DBG,"after inserting auxiliary predicate inputs: input = " << printrange(inp));
    }
  }
}

bool BaseModelGenerator::isCompatibleSet(
		std::vector<ID>& innerEatoms,
		InterpretationConstPtr candidateCompatibleSet,
		InterpretationConstPtr postprocessedInput,
		PredicateMask& gpMask,
		PredicateMask& gnMask,
		ProgramCtx& ctx,
		NogoodContainerPtr nc){

	RegistryPtr reg = ctx.registry();

	// project to pos and neg eatom replacements for validation
	InterpretationPtr projint(new Interpretation(reg));
	projint->getStorage() = candidateCompatibleSet->getStorage() - postprocessedInput->getStorage();

	gpMask.updateMask();
	InterpretationPtr projectedModelCandidate_pos(new Interpretation(reg));
	projectedModelCandidate_pos->getStorage() = projint->getStorage() & gpMask.mask()->getStorage();
	InterpretationPtr projectedModelCandidate_pos_val(new Interpretation(reg));
	projectedModelCandidate_pos_val->getStorage() = projectedModelCandidate_pos->getStorage();
	DBGLOG(DBG,"projected positive guess: " << *projectedModelCandidate_pos);

	gnMask.updateMask();
	InterpretationPtr projectedModelCandidate_neg(new Interpretation(reg));
	projectedModelCandidate_neg->getStorage() = projint->getStorage() & gnMask.mask()->getStorage();
	DBGLOG(DBG,"projected negative guess: " << *projectedModelCandidate_neg);

	// verify whether correct eatoms where guessed true
	// this callback checks if a positive eatom result was guessed as negative
	// -> in this case it aborts
	// this callback resets all positive bits it encounters
	// -> if the positive interpretation is all-zeroes at the end,
	//    the guess was correct
	VerifyExternalAnswerAgainstPosNegGuessInterpretationCB cb(
	  projectedModelCandidate_pos_val, projectedModelCandidate_neg);

	// we might need edb facts here
	// (dependencies to edb are not modelled in the dependency graph)
	// therefore we did not mask the guess program before
	if (!evaluateExternalAtoms(reg, innerEatoms, candidateCompatibleSet, cb, &ctx, ctx.config.getOption("ExternalLearning") ? nc : GenuineSolverPtr())){
		return false;
	}

	// check if we guessed too many true atoms
	if (projectedModelCandidate_pos_val->getStorage().count() > 0){
		return false;
	}
	return true;
}

bool BaseModelGenerator::isSubsetMinimalFLPModel(
		std::vector<ID>& innerEatoms,
		InterpretationConstPtr compatibleSet,
		InterpretationConstPtr postprocessedInput,
		PredicateMask& gpMask,
		PredicateMask& gnMask,
		PredicateMask& fMask,
		std::vector<ID>& xidbflphead,
		std::vector<ID>& xidbflpbody,
		std::vector<ID>& gidb,
		ProgramCtx& ctx){

	RegistryPtr reg = ctx.registry();

	/*
	* FLP check:
	* Check if the flp reduct of the program has a model which is a proper subset of modelCandidate
	* 
	* This check is done as follows:
	* 1. evaluate edb + xidbflphead + M
	*    -> yields singleton answer set containing flp heads F for non-blocked rules
	* 2. evaluate edb + xidbflpbody + gidb + F
	*    -> yields candidate compatible models Cand[1], ..., Cand[n] of the reduct
	* 3. check each Cand[i] for compatibility (just as the check above for modelCandidate)
	*    -> yields compatible reduct models Comp[1], ...,, Comp[m], m <= n
	* 4. for all i: project modelCandidate and Comp[i] to ordinary atoms (remove flp and replacement atoms)
	* 5. if for some i, projected Comp[i] is a proper subset of projected modelCandidate, modelCandidate is rejected,
	*    otherwise it is a subset-minimal model of the flp reduct
	*/
	InterpretationPtr flpas;
	{
		DBGLOG(DBG,"evaluating flp head program");

		// here we can mask, we won't lose FLP heads
		OrdinaryASPProgram flpheadprogram(reg, xidbflphead, compatibleSet, ctx.maxint);
		GenuineSolverPtr flpheadsolver = GenuineSolver::getInstance(ctx, flpheadprogram);

		flpas = flpheadsolver->projectToOrdinaryAtoms(flpheadsolver->getNextModel());
		if( flpas == InterpretationPtr() )
		{
			DBGLOG(DBG, "FLP head program yielded no answer set");
			assert(false);
		}else{
			DBGLOG(DBG, "FLP head program yielded at least one answer set");
		}
	}
	DBGLOG(DBG,"got FLP head model " << *flpas);

	// evaluate xidbflpbody+gidb+edb+flp
	std::stringstream ss;
	RawPrinter printer(ss, ctx.registry());
	ASPSolverManager::ResultsPtr flpbodyres;
	int flpm = 0;
	{
		DBGLOG(DBG, "evaluating flp body program");

		// build edb+flp
		Interpretation::Ptr reductEDB(new Interpretation(reg));
		fMask.updateMask();
		reductEDB->getStorage() |= flpas->getStorage() & fMask.mask()->getStorage();
		reductEDB->add(*postprocessedInput);

		std::vector<ID> simulatedReduct = xidbflpbody;
		// add guessing program to flpbody program
		BOOST_FOREACH (ID rid, gidb){
			simulatedReduct.push_back(rid);
		}

		static const bool encodeMinimalityCheckIntoReduct = true;

		if (encodeMinimalityCheckIntoReduct){
			// add minimality rules to flpbody program
			std::map<ID, std::pair<int, ID> > shadowPredicates;
			computeShadowPredicates(reg, postprocessedInput, simulatedReduct, shadowPredicates);
			Interpretation::Ptr shadowInterpretation(new Interpretation(reg));
			addShadowInterpretation(reg, shadowPredicates, compatibleSet, shadowInterpretation);
			createMinimalityRules(reg, shadowPredicates, simulatedReduct);
			reductEDB->add(*shadowInterpretation);
		}

		ss << "simulatedReduct: IDB={";
		printer.printmany(simulatedReduct, "\n");
		ss << "}\nEDB=" << *reductEDB;
		DBGLOG(DBG, "Evaluating simulated reduct: " << ss.str());

		OrdinaryASPProgram flpbodyprogram(reg, simulatedReduct, reductEDB, ctx.maxint);
		GenuineSolverPtr flpbodysolver = GenuineSolver::getInstance(ctx, flpbodyprogram);

		InterpretationPtr flpbodyas = flpbodysolver->projectToOrdinaryAtoms(flpbodysolver->getNextModel());
		while(flpbodyas != InterpretationPtr())
		{
			// compatibility check
			DBGLOG(DBG, "doing compatibility check for reduct model candidate " << *flpbodyas);
			bool compatible = isCompatibleSet(innerEatoms, flpbodyas, postprocessedInput, gpMask, gnMask, ctx, flpbodysolver);
			DBGLOG(DBG, "Compatibility: " << compatible);

			// remove input and shadow input (because it not contained in modelCandidate neither)
			flpbodyas->getStorage() -= postprocessedInput->getStorage();
			DBGLOG(DBG, "Removed input facts: " << *flpbodyas);

			if (compatible){
				// check if the reduct model is smaller than modelCandidate
				if (encodeMinimalityCheckIntoReduct){
					// reduct model is a proper subset (this was already ensured by the program encoding)
					DBGLOG(DBG, "Model candidate " << *compatibleSet << " failed FLP check (checked agains " << flpm << " compatible reduct models before smaller one was found) because " << *flpbodyas << " is a subset");
					return false;
				}else{
					// project both the model candidate and the reduct model to ordinary atoms
					InterpretationPtr candidate(new Interpretation(*compatibleSet));
					candidate->getStorage() -= (compatibleSet->getStorage() & gpMask.mask()->getStorage());
					candidate->getStorage() -= (compatibleSet->getStorage() & gnMask.mask()->getStorage());
					candidate->getStorage() -= postprocessedInput->getStorage();

					flpbodyas->getStorage() -= (flpbodyas->getStorage() & gpMask.mask()->getStorage());
					flpbodyas->getStorage() -= (flpbodyas->getStorage() & gnMask.mask()->getStorage());
					flpbodyas->getStorage() -= (flpbodyas->getStorage() & fMask.mask()->getStorage());

					DBGLOG(DBG, "Checking if reduct model " << *flpbodyas << " is a subset of model candidate " << *candidate);

					if ((candidate->getStorage() & flpbodyas->getStorage()).count() == flpbodyas->getStorage().count() &&	// subset property
					     candidate->getStorage().count() > flpbodyas->getStorage().count()){				// proper subset property
						// found a smaller model of the reduct
						flpm++;
						DBGLOG(DBG, "Model candidate " << *compatibleSet << " failed FLP check (checked agains " << flpm << " compatible reduct models before smaller one was found) because " << *flpbodyas << " is a subset");
						return false;
					}else{
						DBGLOG(DBG, "Reduct model is no proper subset");
						flpm++;
					}
				}
			}

			DBGLOG(DBG, "Go to next model of reduct");
			flpbodyas = flpbodysolver->projectToOrdinaryAtoms(flpbodysolver->getNextModel());
		}
	}
	DBGLOG(DBG, "Model candidate " << *compatibleSet << " passed FLP check (against " << flpm << " compatible reduct models)");			
	return true;
}

// rewrite all eatoms in body tuple to auxiliary replacement atoms
// store new body into convbody
// (works recursively for aggregate atoms,
// will create additional "auxiliary" aggregate atoms in registry)
void BaseModelGeneratorFactory::convertRuleBody(
    RegistryPtr reg, const Tuple& body, Tuple& convbody)
{
  assert(convbody.empty());
  for(Tuple::const_iterator itlit = body.begin();
      itlit != body.end(); ++itlit)
  {
    if( itlit->isAggregateAtom() )
    {
      // recursively treat aggregates
      
      // findout if aggregate contains external atoms
      const AggregateAtom& aatom = reg->aatoms.getByID(*itlit);
      AggregateAtom convaatom(aatom);
      convaatom.literals.clear();
      convertRuleBody(reg, aatom.literals, convaatom.literals);
      if( convaatom.literals != aatom.literals )
      {
        // really create new aggregate atom
        convaatom.kind |= ID::PROPERTY_AUX;
        ID newaatomid = reg->aatoms.storeAndGetID(convaatom);
        convbody.push_back(newaatomid);
      }
      else
      {
        // use original aggregate atom
        convbody.push_back(*itlit);
      }
    }
    else if( itlit->isExternalAtom() )
    {
      bool naf = itlit->isNaf();
      const ExternalAtom& eatom = reg->eatoms.getByID(
          ID::atomFromLiteral(*itlit));
      DBGLOG(DBG,"rewriting external atom " << eatom <<
          " literal with id " << *itlit);

      assert(!!eatom.pluginAtom);
      PluginAtom* pluginAtom = eatom.pluginAtom;

      // create replacement atom
      OrdinaryAtom replacement(ID::MAINKIND_ATOM | ID::PROPERTY_AUX);
      replacement.tuple.push_back(
          reg->getAuxiliaryConstantSymbol('r',
            pluginAtom->getPredicateID()));
      replacement.tuple.insert(replacement.tuple.end(),
          eatom.inputs.begin(), eatom.inputs.end());
      replacement.tuple.insert(replacement.tuple.end(),
          eatom.tuple.begin(), eatom.tuple.end());

      // bit trick: replacement is ground so far, by setting one bit we make it nonground
      bool ground = true;
      BOOST_FOREACH(ID term, replacement.tuple)
      {
        if( term.isVariableTerm() )
          ground = false;
      }
      if( !ground )
        replacement.kind |= ID::SUBKIND_ATOM_ORDINARYN;

      ID idreplacement;
      if( ground )
        idreplacement = reg->storeOrdinaryGAtom(replacement);
      else
        idreplacement = reg->storeOrdinaryNAtom(replacement);
      DBGLOG(DBG,"adding replacement atom " << idreplacement << " as literal");
      convbody.push_back(ID::literalFromAtom(idreplacement, naf));
    }
    else
    {
      DBGLOG(DBG,"adding original literal " << *itlit);
      convbody.push_back(*itlit);
    }
  }
}

// get rule
// rewrite all eatoms in body to auxiliary replacement atoms
// store and return id
ID BaseModelGeneratorFactory::convertRule(RegistryPtr reg, ID ruleid)
{
  if( !ruleid.doesRuleContainExtatoms() )
  {
    DBGLOG(DBG,"not converting rule " << ruleid << " (does not contain extatoms)");
    return ruleid;
  }

  // we need to rewrite
  const Rule& rule = reg->rules.getByID(ruleid);
  #ifndef NDEBUG
  {
    std::stringstream s;
    RawPrinter printer(s, reg);
    printer.print(ruleid);
    DBGLOG(DBG,"rewriting rule " << s.str() << " from " << rule <<
        " with id " << ruleid << " to auxiliary predicates");
  }
  #endif

  // copy it
  Rule newrule(rule);
  newrule.kind |= ID::PROPERTY_AUX;
  newrule.body.clear();

  // convert (recursively in aggregates)
  convertRuleBody(reg, rule.body, newrule.body);

  // store as rule
  ID newruleid = reg->storeRule(newrule);
  #ifndef NDEBUG
  {
    std::stringstream s;
    RawPrinter printer(s, reg);
    printer.print(newruleid);
    DBGLOG(DBG,"rewritten rule " << s.str() << " from " << newrule <<
        " got id " << newruleid);
  }
  #endif
  return newruleid;
}


/**
 * go through all rules with external atoms
 * for each such rule and each inner eatom in the body:
 * * collect all variables in the eatom (input and output)
 * * collect all positive non-external predicates in the rule body containing these variables
 * * build rule <aux_ext_eatompos>(<all variables>) v <aux_ext_eatomneg>(<all variables>) :- <all bodies>
 * * store into gidb
 */
void BaseModelGeneratorFactory::createEatomGuessingRules(
    RegistryPtr reg,
    const std::vector<ID>& idb,
    const std::vector<ID>& innerEatoms,
    std::vector<ID>& gidb,
    PredicateMask& gpmask,
    PredicateMask& gnmask)
{
  std::set<ID> innerEatomsSet(innerEatoms.begin(), innerEatoms.end());
  assert((innerEatomsSet.empty() ||
      (!innerEatomsSet.begin()->isLiteral() && innerEatomsSet.begin()->isExternalAtom())) &&
      "we don't want literals here, we want external atoms");

  DBGLOG_SCOPE(DBG,"cEAGR",false);
  BOOST_FOREACH(ID rid, idb)
  {
    // skip rules without external atoms
    if( !rid.doesRuleContainExtatoms() )
      continue;

    const Rule& r = reg->rules.getByID(rid);
    DBGLOG(DBG,"processing rule with external atoms: " << rid << " " << r);

    BOOST_FOREACH(ID lit, r.body)
    {
      // skip atoms that are not external atoms
      if( !lit.isExternalAtom() )
        continue;

      if( innerEatomsSet.count(ID::atomFromLiteral(lit)) == 0 )
        continue;

      const ExternalAtom& eatom = reg->eatoms.getByID(lit);
      DBGLOG(DBG,"processing external atom " << lit << " " << eatom);
      DBGLOG_INDENT(DBG);

      // prepare replacement atom
      OrdinaryAtom replacement(
          ID::MAINKIND_ATOM | ID::PROPERTY_AUX);

      // tuple: (replacement_predicate, inputs_as_in_inputtuple*, outputs*)
      // (build up incrementally)
      ID pospredicate = reg->getAuxiliaryConstantSymbol('r', eatom.predicate);
      ID negpredicate = reg->getAuxiliaryConstantSymbol('n', eatom.predicate);
      replacement.tuple.push_back(pospredicate);
      gpmask.addPredicate(pospredicate);
      gnmask.addPredicate(negpredicate);

      // build (nonground) replacement and harvest all variables
      std::set<ID> variables;
      BOOST_FOREACH(ID inp, eatom.inputs)
      {
        replacement.tuple.push_back(inp);
        if( inp.isVariableTerm() )
          variables.insert(inp);
      }
      BOOST_FOREACH(ID outp, eatom.tuple)
      {
        replacement.tuple.push_back(outp);
        if( outp.isVariableTerm() )
          variables.insert(outp);
      }
      DBGLOG(DBG,"found set of variables: " << printset(variables));

      // groundness of replacement predicate
      ID posreplacement;
      ID negreplacement;
      if( variables.empty() )
      {
        replacement.kind |= ID::SUBKIND_ATOM_ORDINARYG;
        posreplacement = reg->storeOrdinaryGAtom(replacement);
        replacement.tuple[0] = negpredicate;
        negreplacement = reg->storeOrdinaryGAtom(replacement);
      }
      else
      {
        replacement.kind |= ID::SUBKIND_ATOM_ORDINARYN;
        posreplacement = reg->storeOrdinaryNAtom(replacement);
        replacement.tuple[0] = negpredicate;
        negreplacement = reg->storeOrdinaryNAtom(replacement);
      }
      DBGLOG(DBG,"registered posreplacement " << posreplacement <<
          " and negreplacement " << negreplacement);

      // create rule head
      Rule guessingrule(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR |
          ID::PROPERTY_AUX | ID::PROPERTY_RULE_DISJ);
      guessingrule.head.push_back(posreplacement);
      guessingrule.head.push_back(negreplacement);

      // create rule body (if there are variables that need to be grounded)
      if( !variables.empty() )
      {
        // harvest all positive ordinary nonground atoms
        // "grounding the variables" (i.e., those that contain them)
        BOOST_FOREACH(ID lit, r.body)
        {
          if( lit.isNaf() ||
              lit.isExternalAtom() )
            continue;

          bool use = false;
          if( lit.isOrdinaryNongroundAtom() )
          {
            const OrdinaryAtom& oatom = reg->onatoms.getByID(lit);
            // look if this atom grounds any variables we need
            BOOST_FOREACH(ID term, oatom.tuple)
            {
              if( term.isVariableTerm() &&
                  (variables.find(term) != variables.end()) )
              {
                use = true;
                break;
              }
            }
          }
          else
          {
            LOG(WARNING,"TODO think about whether we need to consider "
                "builtin or aggregate atoms here");
          }

          if( use )
          {
            guessingrule.body.push_back(lit);
          }
        }
      }

      // store rule
      ID gid = reg->rules.storeAndGetID(guessingrule);
      DBGLOG(DBG,"stored guessingrule " << guessingrule << " which got id " << gid);
      #ifndef NDEBUG
      {
        std::stringstream s;
        RawPrinter p(s, reg);
        p.print(gid);
        DBGLOG(DBG,"  " << s.str());
      }
      #endif
      gidb.push_back(gid);
    }
  }
}

/**
 * for each rule in xidb
 * * keep constraints: copy ID to xidbflphead and xidbflpbody
 * * keep disjunctive facts: copy ID to xidbflphead and xidbflpbody
 * * for all others:
 * * collect all variables in the body (which means also all variables in the head)
 * * create ground or nonground flp replacement atom containing all variables
 * * create rule <flpreplacement>(<allvariables>) :- <body> and store in xidbflphead
 * * create rule <head> :- <flpreplacement>(<allvariables>), <body> and store in xidbflpbody
 */
void BaseModelGeneratorFactory::createFLPRules(
    RegistryPtr reg,
    const std::vector<ID>& xidb,
    std::vector<ID>& xidbflphead,
    std::vector<ID>& xidbflpbody,
    PredicateMask& fmask)
{
  DBGLOG_SCOPE(DBG,"cFLPR",false);
  BOOST_FOREACH(ID rid, xidb)
  {
    const Rule& r = reg->rules.getByID(rid);
    DBGLOG(DBG,"processing rule " << rid << " " << r);
    if( r.body.empty() )
    {
      // keep disjunctive facts as they are
      xidbflphead.push_back(rid);
      xidbflpbody.push_back(rid);
    }
    else if( rid.isConstraint() ||
        rid.isRegularRule() )
    {
      // collect all variables
      std::set<ID> variables;
      BOOST_FOREACH(ID lit, r.body)
      {
        assert(!lit.isExternalAtom() && "in xidb there must not be external atoms left");
        #warning TODO factorize "get all (free) variables from entity"
        // from ground literals we don't need variables
        if( lit.isOrdinaryGroundAtom() )
          continue;

        if( lit.isOrdinaryNongroundAtom() )
        {
          const OrdinaryAtom& onatom = reg->onatoms.getByID(lit);
          BOOST_FOREACH(ID idt, onatom.tuple)
          {
            if( idt.isVariableTerm() )
              variables.insert(idt);
          }
        }
        else if( lit.isBuiltinAtom() )
        {
          const BuiltinAtom& batom = reg->batoms.getByID(lit);
          BOOST_FOREACH(ID idt, batom.tuple)
          {
            if( idt.isVariableTerm() )
              variables.insert(idt);
          }
        }
        #warning implement aggregates here
        else
        {
          LOG(ERROR,"encountered literal " << lit << " in FLP check, don't know what to do about it");
          throw FatalError("TODO: think about how to treat other types of atoms in FLP check");
        }
      }
      DBGLOG(DBG,"collected variables " << printset(variables));

      // prepare replacement atom
      OrdinaryAtom replacement(
          ID::MAINKIND_ATOM | ID::PROPERTY_AUX);

      // tuple: (replacement_predicate, variables*)
      ID flppredicate = reg->getAuxiliaryConstantSymbol('f', rid);
      replacement.tuple.push_back(flppredicate);
      fmask.addPredicate(flppredicate);

      // groundness of replacement predicate
      ID fid;
      if( variables.empty() )
      {
        replacement.kind |= ID::SUBKIND_ATOM_ORDINARYG;
        fid = reg->storeOrdinaryGAtom(replacement);
      }
      else
      {
        replacement.kind |= ID::SUBKIND_ATOM_ORDINARYN;
        replacement.tuple.insert(replacement.tuple.end(),
            variables.begin(), variables.end());
        fid = reg->storeOrdinaryNAtom(replacement);
      }
      DBGLOG(DBG,"registered flp replacement " << replacement <<
          " with fid " << fid);

      // create rules
      Rule rflphead(
          ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR | ID::PROPERTY_AUX);
      rflphead.head.push_back(fid);
      rflphead.body = r.body;

      IDKind kind = ID::MAINKIND_RULE | ID::PROPERTY_AUX;
      if (r.head.size() == 0){
        kind |= ID::SUBKIND_RULE_CONSTRAINT;
      }else{
        kind |= ID::SUBKIND_RULE_REGULAR;
      }
      Rule rflpbody(kind);
      rflpbody.head = r.head;
      if( rflpbody.head.size() > 1 )
        rflpbody.kind |= ID::PROPERTY_RULE_DISJ;
      rflpbody.body = r.body;
      rflpbody.body.push_back(fid);

      // store rules
      ID fheadrid = reg->rules.storeAndGetID(rflphead);
      xidbflphead.push_back(fheadrid);
      ID fbodyrid = reg->rules.storeAndGetID(rflpbody);
      xidbflpbody.push_back(fbodyrid);

      #ifndef NDEBUG
      {
        std::stringstream s;
        RawPrinter p(s, reg);
        p.print(fheadrid);
        s << " and ";
        p.print(fbodyrid);
        DBGLOG(DBG,"stored flphead rule " << rflphead << " which got id " << fheadrid);
        DBGLOG(DBG,"stored flpbody rule " << rflpbody << " which got id " << fbodyrid);
        DBGLOG(DBG,"rules are " << s.str());
      }
      #endif
    }
    else
    {
      LOG(ERROR,"got weak rule " << r << " in guess and check model generator, don't know what to do about it");
      throw FatalError("TODO: think about weak rules in G&C MG");
    }
  }
}

void BaseModelGenerator::computeShadowPredicates(
	RegistryPtr reg,
	InterpretationConstPtr edb,
	const std::vector<ID>& idb,
	std::map<ID, std::pair<int, ID> >& shadowPredicates){

	// collect predicates
	std::set<std::pair<int, ID> > preds;

	// edb
	bm::bvector<>::enumerator en = edb->getStorage().first();
	bm::bvector<>::enumerator en_end = edb->getStorage().end();
	while (en < en_end){
		const OrdinaryAtom& atom = reg->ogatoms.getByID(ID(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, *en));
		if (!ID(atom.kind, *en).isAuxiliary()){
			preds.insert(std::pair<int, ID>(atom.tuple.size() - 1, atom.tuple.front()));
		}
		en++;
	}

	// idb
	BOOST_FOREACH (ID rid, idb){
		const Rule& r = reg->rules.getByID(rid);
		BOOST_FOREACH (ID h, r.head){
			if (!h.isAuxiliary()){
				const OrdinaryAtom& atom = h.isOrdinaryGroundAtom() ? reg->ogatoms.getByID(h) : reg->onatoms.getByID(h);
				preds.insert(std::pair<int, ID>(atom.tuple.size() - 1, atom.tuple.front()));
			}
		}
		BOOST_FOREACH (ID b, r.body){
			if (b.isOrdinaryAtom() && !b.isAuxiliary()){
				const OrdinaryAtom& atom = b.isOrdinaryGroundAtom() ? reg->ogatoms.getByID(b) : reg->onatoms.getByID(b);
				preds.insert(std::pair<int, ID>(atom.tuple.size() - 1, atom.tuple.front()));
			}
		}
	}

	// create unique predicate suffix for shadow predicates
	shadowpostfix = "_shadow";
	int idx = 0;
	bool clash;
	do{
		clash = false;

		// check if the current postfix clashes with any of the predicates
		typedef std::pair<int, ID> Pair;
		BOOST_FOREACH (Pair p, preds){
			std::string currentPred = reg->terms.getByID(p.second).getUnquotedString();
			if (shadowpostfix.length() <= currentPred.length() &&						// currentPred is at least as long as shadowpostfix
			    currentPred.substr(currentPred.length() - shadowpostfix.length()) == shadowpostfix){	// postfixes coincide
				clash = true;
				break;
			}
		}
		std::stringstream ss;
		ss << "_shadow" << idx++;
	}while(clash);

	// create shadow predicates
	typedef std::pair<int, ID> Pair;
	BOOST_FOREACH (Pair p, preds){
		Term shadowTerm(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, reg->terms.getByID(p.second).getUnquotedString() + shadowpostfix);
		ID shadowID = reg->storeTerm(shadowTerm);
		shadowPredicates[p.second] = Pair(p.first, shadowID);
		DBGLOG(DBG, "Predicate " << reg->terms.getByID(p.second).getUnquotedString() << " [" << p.second << "] has shadow predicate " <<
				reg->terms.getByID(p.second).getUnquotedString() + shadowpostfix << " [" << shadowID << "]");
	}
}

void BaseModelGenerator::addShadowInterpretation(
	RegistryPtr reg,
	std::map<ID, std::pair<int, ID> >& shadowPredicates,
	InterpretationConstPtr input,
	InterpretationPtr output){

	bm::bvector<>::enumerator en = input->getStorage().first();
	bm::bvector<>::enumerator en_end = input->getStorage().end();
	while (en < en_end){
		OrdinaryAtom atom = reg->ogatoms.getByID(ID(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, *en));
		if (shadowPredicates.find(atom.tuple[0]) != shadowPredicates.end()){
			atom.tuple[0] = shadowPredicates[atom.tuple[0]].second;
			output->setFact(reg->storeOrdinaryGAtom(atom).address);
		}
		en++;
	}
}

void BaseModelGenerator::createMinimalityRules(
	RegistryPtr reg,
	std::map<ID, std::pair<int, ID> >& shadowPredicates,
	std::vector<ID>& idb){

	// construct a propositional atom which does neither occur in the input program nor as a shadow predicate
	// for this purpose we use the shadowpostfix alone:
	// - it cannot be used by the input program (otherwise it would not be a postfix)
	// - it cannot be used by the shadow atoms (otherwise an input atom would be the empty string, which is not possible)
	Term smallerTerm(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, shadowpostfix);
	OrdinaryAtom smallerAtom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
	smallerAtom.tuple.push_back(reg->storeTerm(smallerTerm));
	ID smallerAtomID = reg->storeOrdinaryGAtom(smallerAtom);

	typedef std::pair<ID, std::pair<int, ID> > Pair;
	BOOST_FOREACH (Pair p, shadowPredicates){
		OrdinaryAtom atom(ID::MAINKIND_ATOM);
		if (p.second.first == 0) atom.kind |= ID::SUBKIND_ATOM_ORDINARYG; else atom.kind |= ID::SUBKIND_ATOM_ORDINARYN;
		atom.tuple.push_back(p.first);
		for (int i = 0; i < p.second.first; ++i){
			std::stringstream var;
			var << "X" << i;
			atom.tuple.push_back(reg->storeVariableTerm(var.str()));
		}

		// store original atom
		ID origID;
		if (p.second.first == 0){
			origID = reg->storeOrdinaryGAtom(atom);
		}else{
			origID = reg->storeOrdinaryNAtom(atom);
		}

		// store shadow atom
		atom.kind = ID::MAINKIND_ATOM;
		if (p.second.first == 0) atom.kind |= ID::SUBKIND_ATOM_ORDINARYG; else atom.kind |= ID::SUBKIND_ATOM_ORDINARYN;
		atom.tuple[0] = p.second.second;
		ID shadowID;
		if (p.second.first == 0){
			shadowID = reg->storeOrdinaryGAtom(atom);
		}else{
			shadowID = reg->storeOrdinaryNAtom(atom);
		}
		DBGLOG(DBG, "Using shadow predicate for " << p.first << " which is " << p.second.second);

		// an atom must not be true if the shadow atom is false because the computed interpretation must be a subset of the shadow interpretation
		{
			// construct rule   :- a, not a_shadow   to ensure that the models are (not necessarily proper) subsets of the shadow model
			Rule r(ID::MAINKIND_RULE | ID::SUBKIND_RULE_CONSTRAINT);
			ID id = origID;
			r.body.push_back(id);
			id = ID(ID::MAINKIND_LITERAL | ID::NAF_MASK | (shadowID.kind & ID::SUBKIND_MASK), shadowID.address);
			r.body.push_back(id);
			idb.push_back(reg->storeRule(r));
		}

		// but we want a proper subset, so add a rule   smaller :- a_shadow, not a
		// an atom must not be true if the shadow atom is false because the computed interpretation must be a subset of the shadow interpretation
		{
			// construct rule   :- a, not a_shadow   to ensure that the models are (not necessarily proper) subsets of the shadow model
			Rule r(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR);
			ID id = smallerAtomID;
			r.head.push_back(id);
			id = ID(ID::MAINKIND_LITERAL | ID::NAF_MASK | (origID.kind & ID::SUBKIND_MASK), origID.address);
			r.body.push_back(id);
			r.body.push_back(shadowID);
			idb.push_back(reg->storeRule(r));
		}
	}

	// construct a rule   :- not smaller   to restrict the search space to proper submodels of the shadow model
	Rule r(ID::MAINKIND_RULE | ID::SUBKIND_RULE_CONSTRAINT);
	r.body.push_back(ID(ID::MAINKIND_LITERAL | ID::SUBKIND_ATOM_ORDINARYG | ID::NAF_MASK, smallerAtomID.address));
	idb.push_back(reg->storeRule(r));
}


DLVHEX_NAMESPACE_END
