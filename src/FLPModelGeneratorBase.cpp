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
 * @file   FLPModelGeneratorBase.cpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * @author Chrisoph Redl <redl@kr.tuwien.ac.at>
 * 
 * @brief  Base class for model generators using FLP reduct.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "dlvhex2/FLPModelGeneratorBase.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/ProgramCtx.h"

#include "dlvhex2/CDNLSolver.h"
#include "dlvhex2/ClaspSolver.h"
#include "dlvhex2/SATSolver.h"
#include "dlvhex2/Printer.h"

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/visitors.hpp> 

#include <fstream>

DLVHEX_NAMESPACE_BEGIN

FLPModelGeneratorFactoryBase::FLPModelGeneratorFactoryBase(
    RegistryPtr reg):
  reg(reg)
{
  gpMask.setRegistry(reg);
  gnMask.setRegistry(reg);
  fMask.setRegistry(reg);
}

/**
 * go through all rules with external atoms
 * for each such rule and each inner eatom in the body:
 * * collect all variables in the eatom (input and output)
 * * collect all positive non-external predicates in the rule body containing these variables
 * * build rule <aux_ext_eatompos>(<all variables>) v <aux_ext_eatomneg>(<all variables>) :- <all bodies>
 * * store into gidb
 */
void FLPModelGeneratorFactoryBase::createEatomGuessingRules()
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
          ID::MAINKIND_ATOM | ID::PROPERTY_AUX | ID::PROPERTY_EXTERNALAUX);

      // tuple: (replacement_predicate, inputs_as_in_inputtuple*, outputs*)
      // (build up incrementally)
      ID pospredicate = reg->getAuxiliaryConstantSymbol('r', eatom.predicate);
      ID negpredicate = reg->getAuxiliaryConstantSymbol('n', eatom.predicate);

      replacement.tuple.push_back(pospredicate);
      gpMask.addPredicate(pospredicate);
      gnMask.addPredicate(negpredicate);

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
      ID gid = reg->storeRule(guessingrule);
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
 * * keep disjunctive facts: copy ID to xidbflphead and xidbflpbody
 * * (note: nondisjunctive facts are stored in edb)
 * * for all other rules:
 * * collect all variables in the body (which means also all variables in the head)
 * * create ground or nonground flp replacement atom containing all variables
 * * create rule <flpreplacement>(<allvariables>) :- <body> and store in xidbflphead
 * * create rule <head> :- <flpreplacement>(<allvariables>), <body> and store in xidbflpbody
 */
void FLPModelGeneratorFactoryBase::createFLPRules()
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
    else if( rid.isConstraint() || rid.isRegularRule() )
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
      fMask.addPredicate(flppredicate);

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
      ID fheadrid = reg->storeRule(rflphead);
      xidbflphead.push_back(fheadrid);
      ID fbodyrid = reg->storeRule(rflpbody);
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

void FLPModelGeneratorFactoryBase::computeCyclicInputPredicates(
			RegistryPtr reg,
			ProgramCtx& ctx,
			const std::vector<ID>& idb){

	std::stringstream dotss;
	dotss << "digraph {";

	// construct predicate dependency graph
	//   nodes are predicates IDs
	//   edges are labeled with booleans (false: ordinary edge, true: external dependency)
	typedef boost::adjacency_list< boost::vecS, boost::vecS, boost::directedS, ID, bool> Graph;
	typedef std::pair<ID, ID> Edge;
	Graph predicateDepGraph;

	std::map<ID, Graph::vertex_descriptor> nodeMapping;
	std::vector<Edge> externalEdges;

	BOOST_FOREACH (ID ruleID, idb){
		const Rule& rule = reg->rules.getByID(ruleID);
		BOOST_FOREACH (ID h, rule.head){
			const OrdinaryAtom& hAtom = h.isOrdinaryGroundAtom() ? reg->ogatoms.getByID(h) : reg->onatoms.getByID(h);

			// make sure that the node exist in the graph
			if (nodeMapping.find(hAtom.tuple[0]) == nodeMapping.end()) nodeMapping[hAtom.tuple[0]] = boost::add_vertex(hAtom.tuple[0], predicateDepGraph);

			BOOST_FOREACH (ID b, rule.body){
				// ordinary edges
				if (b.isOrdinaryAtom()){
					const OrdinaryAtom& bAtom = reg->lookupOrdinaryAtom(b);

					// make sure that the node exist in the graph
					if (nodeMapping.find(bAtom.tuple[0]) == nodeMapping.end()) nodeMapping[bAtom.tuple[0]] = boost::add_vertex(bAtom.tuple[0], predicateDepGraph);

					boost::add_edge(nodeMapping[hAtom.tuple[0]], nodeMapping[bAtom.tuple[0]], false, predicateDepGraph);
					boost::add_edge(nodeMapping[bAtom.tuple[0]], nodeMapping[hAtom.tuple[0]], false, predicateDepGraph);


					if (ctx.config.getOption("DumpCyclicPredicateInputAnalysisGraph")){
						dotss << "\"" << hAtom.tuple[0] << "\" -> \"" << bAtom.tuple[0] << "\";" << std::endl;
					}
				}
				// external edges
				if (b.isExternalAtom()){
					const ExternalAtom& eAtom = reg->eatoms.getByID(b);
					int i = 0;
					BOOST_FOREACH (ID p, eAtom.inputs){
						if (eAtom.pluginAtom->getInputType(i) == PluginAtom::PREDICATE){
							if (nodeMapping.find(p) == nodeMapping.end()) nodeMapping[p] = boost::add_vertex(p, predicateDepGraph);

							boost::add_edge(nodeMapping[hAtom.tuple[0]], nodeMapping[p], true, predicateDepGraph);
							externalEdges.push_back(Edge(hAtom.tuple[0], p));

							if (ctx.config.getOption("DumpCyclicPredicateInputAnalysisGraph")){
								dotss << "\"" << hAtom.tuple[0] << "\" -> \"" << p << "\" [label=\"external\"];" << std::endl;
							}
						}
						i++;
					}
				}
			}
		}
	}

	// check for each e-edge x -> y if there is a path from y to x
	// if yes, then y is a cyclic predicate input
	BOOST_FOREACH (Edge e, externalEdges){
		std::vector<Graph::vertex_descriptor> reachable;
		boost::breadth_first_search(predicateDepGraph, nodeMapping[e.second],
			boost::visitor(
				boost::make_bfs_visitor(
					boost::write_property(
						boost::identity_property_map(),
						std::back_inserter(reachable),
						boost::on_discover_vertex())))); 

		if (std::find(reachable.begin(), reachable.end(), nodeMapping[e.second]) != reachable.end()){
			// yes, there is a cycle
			if (std::find(cyclicInputPredicates.begin(), cyclicInputPredicates.end(), e.second) == cyclicInputPredicates.end()){
				cyclicInputPredicates.push_back(e.second);
			}
		}
	}

#ifndef NDEBUG

	std::stringstream ss;
	bool first = true;
	BOOST_FOREACH (ID p, cyclicInputPredicates){
		if (!first) ss << ", ";
		first = false;
		ss << p;
	}
	DBGLOG(DBG, "Cyclic input predicates: " << ss.str());
#endif

	if (ctx.config.getOption("DumpCyclicPredicateInputAnalysisGraph")){
		dotss << "}";

		std::stringstream fnamev;
		static int cnt = 0;
		fnamev << ctx.config.getStringOption("DebugPrefix") << "_CycInpGraph" << cnt++ << ".dot";
		LOG(INFO,"dumping cyclic predicate input analysis graph " << fnamev.str());
		std::ofstream filev(fnamev.str().c_str());
		filev << dotss.str();
	}

	cyclicInputPredicatesMask.setRegistry(reg);
	BOOST_FOREACH (ID pred, cyclicInputPredicates){
		cyclicInputPredicatesMask.addPredicate(pred);
	}
}

//
// FLPModelGeneratorBase
//

FLPModelGeneratorBase::FLPModelGeneratorBase(
    FLPModelGeneratorFactoryBase& factory, InterpretationConstPtr input):
  BaseModelGenerator(input),
  factory(factory)
{
}

void FLPModelGeneratorBase::createEAMasks(std::vector<ID> groundIDB){

	RegistryPtr reg = factory.reg;

	eaMasks.resize(factory.innerEatoms.size());
	int eaIndex = 0;
	BOOST_FOREACH (ID eatom, factory.innerEatoms){
		// create an EAMask for each inner external atom
		ExternalAtomMask& eaMask = eaMasks[eaIndex];
		eaMask.setEAtom(reg->eatoms.getByID(eatom), groundIDB);
		eaMask.updateMask();
		//      eaVerified.push_back(false);
		//      eaEvaluated.push_back(false);

		// map external auxiliaries back to their external atoms
		bm::bvector<>::enumerator en = eaMask.mask()->getStorage().first();
		bm::bvector<>::enumerator en_end = eaMask.mask()->getStorage().end();
		while (en < en_end){
			if (reg->ogatoms.getIDByAddress(*en).isExternalAuxiliary()){
				auxToEA[*en].push_back(factory.innerEatoms[eaIndex]);
			}
			en++;
		}
		eaIndex++;
	}
}

FLPModelGeneratorBase::VerifyExternalAnswerAgainstPosNegGuessInterpretationCB::
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
FLPModelGeneratorBase::VerifyExternalAnswerAgainstPosNegGuessInterpretationCB::
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
FLPModelGeneratorBase::VerifyExternalAnswerAgainstPosNegGuessInterpretationCB::
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
FLPModelGeneratorBase::VerifyExternalAnswerAgainstPosNegGuessInterpretationCB::
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


FLPModelGeneratorBase::VerifyExternalAtomCB::VerifyExternalAtomCB(InterpretationConstPtr guess, const ExternalAtom& eatom, const ExternalAtomMask& eaMask) : guess(guess), remainingguess(), verified(true), exatom(eatom), eaMask(eaMask), replacement(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG | ID::PROPERTY_AUX), falsified(ID_FAIL){

	reg = eatom.pluginAtom->getRegistry();

	pospred = reg->getAuxiliaryConstantSymbol('r', exatom.predicate);
	negpred = reg->getAuxiliaryConstantSymbol('n', exatom.predicate);
	replacement.tuple.resize(1);

	remainingguess = InterpretationPtr(new Interpretation(reg));
	remainingguess->add(*guess);
	remainingguess->getStorage() &= eaMask.mask()->getStorage();
}

FLPModelGeneratorBase::VerifyExternalAtomCB::~VerifyExternalAtomCB(){
}

bool FLPModelGeneratorBase::VerifyExternalAtomCB::onlyNegativeAuxiliaries(){

	bm::bvector<>::enumerator en = remainingguess->getStorage().first();
	bm::bvector<>::enumerator en_end = remainingguess->getStorage().end();

	while (en < en_end){
		const OrdinaryAtom& oatom = reg->ogatoms.getByAddress(*en);
		if (oatom.tuple[0] == pospred){
			DBGLOG(DBG, "Unfounded positive auxiliary detected: " << *en);
			falsified = reg->ogatoms.getIDByAddress(*en);
			return false;
		}
		en++;
	}
	return true;
}

bool FLPModelGeneratorBase::VerifyExternalAtomCB::eatom(const ExternalAtom& exatom){

	// this callback must not be used for evaluating multiple external atoms
	assert(&exatom == &this->exatom);

	return true;
}

bool FLPModelGeneratorBase::VerifyExternalAtomCB::input(const Tuple& input){

	assert(replacement.tuple.size() >= 1);

	// shorten
	replacement.tuple.resize(1);

	// add
	replacement.tuple.insert(replacement.tuple.end(), input.begin(), input.end());

	// never abort
	return true;
}

bool FLPModelGeneratorBase::VerifyExternalAtomCB::output(const Tuple& output){

	assert(replacement.tuple.size() >= 1);

	// add, but remember size to reset it later
	unsigned size = replacement.tuple.size();
	replacement.tuple.insert(replacement.tuple.end(), output.begin(), output.end());

	// build pos replacement, register, and clear the corresponding bit in guess_pos
	replacement.tuple[0] = pospred;
	ID idreplacement_pos = reg->storeOrdinaryGAtom(replacement);
	replacement.tuple[0] = negpred;
	ID idreplacement_neg = reg->storeOrdinaryGAtom(replacement);

	// shorten it, s.t. we can add the next one
	replacement.tuple.resize(size);

	if(remainingguess->getFact(idreplacement_neg.address)){
		DBGLOG(DBG, "Positive atom was guessed to be false: " << idreplacement_pos.address);
		verified = false;
		falsified = reg->ogatoms.getIDByAddress(idreplacement_neg.address);
		return false;
	}else{
		DBGLOG(DBG, "Positive atom was guessed correctly");
		remainingguess->clearFact(idreplacement_pos.address);
		return true;
	}
}

bool FLPModelGeneratorBase::VerifyExternalAtomCB::verify(){

	if (remainingguess){
		if (!onlyNegativeAuxiliaries()){
			verified = false;
		}
		remainingguess.reset();
	}

	return verified;
}

ID FLPModelGeneratorBase::VerifyExternalAtomCB::getFalsifiedAtom(){
	return falsified;
}


bool FLPModelGeneratorBase::isCompatibleSet(
		InterpretationConstPtr candidateCompatibleSet,
		InterpretationConstPtr postprocessedInput,
    ProgramCtx& ctx,
		NogoodContainerPtr nc){
	RegistryPtr& reg = factory.reg;
  PredicateMask& gpMask = factory.gpMask;
  PredicateMask& gnMask = factory.gnMask;

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
	if( !evaluateExternalAtoms(
        reg, factory.innerEatoms,
        candidateCompatibleSet, cb,
        &ctx, ctx.config.getOption("ExternalLearning") ? nc : GenuineSolverPtr()))
  {
		return false;
	}

	// check if we guessed too many true atoms
	if (projectedModelCandidate_pos_val->getStorage().count() > 0){
		return false;
	}
	return true;
}

Nogood FLPModelGeneratorBase::getFLPNogood(
		ProgramCtx& ctx,
		const OrdinaryASPProgram& groundProgram,
		InterpretationConstPtr compatibleSet,
		InterpretationConstPtr projectedCompatibleSet,
		InterpretationConstPtr smallerFLPModel
		){

	RegistryPtr reg = factory.reg;

	Nogood ng;

	// for each rule with unsatisfied body
	BOOST_FOREACH (ID ruleId, groundProgram.idb){
		const Rule& rule = reg->rules.getByID(ruleId);
		BOOST_FOREACH (ID b, rule.body){
			if (compatibleSet->getFact(b.address) != !b.isNaf()){
				// take an unsatisfied body literal
				ng.insert(NogoodContainer::createLiteral(b.address, compatibleSet->getFact(b.address)));
				break;
			}
		}
	}

	// add the smaller FLP model
	bm::bvector<>::enumerator en = smallerFLPModel->getStorage().first();
	bm::bvector<>::enumerator en_end = smallerFLPModel->getStorage().end();
	while (en < en_end){
		ng.insert(NogoodContainer::createLiteral(*en));
		en++;
	}

	// add one atom which is in the compatible set but not in the flp model
	en = projectedCompatibleSet->getStorage().first();
	en_end = projectedCompatibleSet->getStorage().end();
	while (en < en_end){
		if (!smallerFLPModel->getFact(*en)){
			ng.insert(NogoodContainer::createLiteral(*en));
			break;
		}
		en++;
	}

	DBGLOG(DBG, "Constructed FLP nogood " << ng);

	return ng;
}

#warning TODO could we move shadow predicates and mappings and rules to factory?
void FLPModelGeneratorBase::computeShadowAndUnfoundedPredicates(
	RegistryPtr reg,
	InterpretationConstPtr edb,
	const std::vector<ID>& idb,
	std::map<ID, std::pair<int, ID> >& shadowPredicates,
	std::map<ID, std::pair<int, ID> >& unfoundedPredicates,
	std::string& shadowPostfix,
	std::string& unfoundedPostfix){

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
  // (must not start with _ as it will be used by itself and
  // constants starting with _ are forbidden in dlv as they are not c-identifiers)
	shadowPostfix = "shadow";
	int idx = 0;
	bool clash;
	do{
		clash = false;

		// check if the current postfix clashes with any of the predicates
		typedef std::pair<int, ID> Pair;
		BOOST_FOREACH (Pair p, preds){
			std::string currentPred = reg->terms.getByID(p.second).getUnquotedString();
			if (shadowPostfix.length() <= currentPred.length() &&						// currentPred is at least as long as shadowPostfix
			    currentPred.substr(currentPred.length() - shadowPostfix.length()) == shadowPostfix){	// postfixes coincide
				clash = true;
				break;
			}
		}
		std::stringstream ss;
		ss << "shadow" << idx++;
	}while(clash);

	// create shadow predicates
	typedef std::pair<int, ID> Pair;
	BOOST_FOREACH (Pair p, preds){
		Term shadowTerm(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, reg->terms.getByID(p.second).getUnquotedString() + shadowPostfix);
		ID shadowID = reg->storeTerm(shadowTerm);
		shadowPredicates[p.second] = Pair(p.first, shadowID);
		DBGLOG(DBG, "Predicate " << reg->terms.getByID(p.second).getUnquotedString() << " [" << p.second << "] has shadow predicate " <<
				reg->terms.getByID(p.second).getUnquotedString() + shadowPostfix << " [" << shadowID << "]");
	}

	// create unfounded predicate suffix for shadow predicates
	unfoundedPostfix = "_unfounded";
	idx = 0;
	do{
		clash = false;

		// check if the current postfix clashes with any of the predicates
		typedef std::pair<int, ID> Pair;
		BOOST_FOREACH (Pair p, preds){
			std::string currentPred = reg->terms.getByID(p.second).getUnquotedString();
			if (unfoundedPostfix.length() <= currentPred.length() &&						// currentPred is at least as long as shadowPostfix
			    currentPred.substr(currentPred.length() - unfoundedPostfix.length()) == unfoundedPostfix){		// postfixes coincide
				clash = true;
				break;
			}
		}
		std::stringstream ss;
		ss << "unfounded" << idx++;
	}while(clash);

	// create unfounded predicates
	BOOST_FOREACH (Pair p, preds){
		Term unfoundedTerm(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, reg->terms.getByID(p.second).getUnquotedString() + unfoundedPostfix);
		ID unfoundedID = reg->storeTerm(unfoundedTerm);
		unfoundedPredicates[p.second] = Pair(p.first, unfoundedID);
		DBGLOG(DBG, "Predicate " << reg->terms.getByID(p.second).getUnquotedString() << " [" << p.second << "] has unfounded predicate " <<
				reg->terms.getByID(p.second).getUnquotedString() + unfoundedPostfix << " [" << unfoundedID << "]");
	}
}

void FLPModelGeneratorBase::addShadowInterpretation(
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

void FLPModelGeneratorBase::createMinimalityRules(
	RegistryPtr reg,
	std::map<ID, std::pair<int, ID> >& shadowPredicates,
	std::string& shadowPostfix,
	std::vector<ID>& idb){

	// construct a propositional atom which does neither occur in the input program nor as a shadow predicate
	// for this purpose we use the shadowPostfix alone:
	// - it cannot be used by the input program (otherwise it would not be a postfix)
	// - it cannot be used by the shadow atoms (otherwise an input atom would be the empty string, which is not possible)
	Term smallerTerm(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, shadowPostfix);
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

void FLPModelGeneratorBase::createFoundingRules(
	RegistryPtr reg,
	std::map<ID, std::pair<int, ID> >& shadowPredicates,
	std::map<ID, std::pair<int, ID> >& unfoundedPredicates,
	std::vector<ID>& idb){

	// We want to compute a _model_ of the reduct rather than an _answer set_,
	// i.e., atoms are allowed to be _not_ founded.
	// For this we introduce for each n-ary shadow predicate
	//	ps(X1, ..., Xn)
	// a rule
	//	p(X1, ..., Xn) v p_unfounded(X1, ..., Xn) :- ps(X1, ..., Xn)
	// which can be used to found an atom.
	// (p_unfounded(X1, ..., Xn) encodes that the atom is not artificially founded)

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

		// store unfounded atom
		atom.kind = ID::MAINKIND_ATOM;
		if (p.second.first == 0) atom.kind |= ID::SUBKIND_ATOM_ORDINARYG; else atom.kind |= ID::SUBKIND_ATOM_ORDINARYN;
		atom.tuple[0] = unfoundedPredicates[p.first].second;
		ID unfoundedID;
		if (p.second.first == 0){
			unfoundedID = reg->storeOrdinaryGAtom(atom);
		}else{
			unfoundedID = reg->storeOrdinaryNAtom(atom);
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
		DBGLOG(DBG, "Using shadow predicate for " << p.first << " which is " << p.second.second << " and unfounded predicate which is " << unfoundedPredicates[p.first].second);

		// for each shadow atom, either the original atom or the notfounded atom is derived
		{
			Rule r(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR | ID::PROPERTY_RULE_DISJ);
			r.head.push_back(origID);
			r.head.push_back(unfoundedID);
			r.body.push_back(shadowID);
			idb.push_back(reg->storeRule(r));
		}
	}
}


InterpretationPtr FLPModelGeneratorBase::getFixpoint(InterpretationConstPtr interpretation, const OrdinaryASPProgram& program){

	RegistryPtr reg = interpretation->getRegistry();

	DBGLOG(DBG, "Well-Justified FLP Semantics: Fixpoint Computation (reference interpretation: " << *interpretation << ")");
	// create a bitset of all ground atoms and prepare the set of all remaining rules (initially all rules)
	std::set<ID> remainingRules;
	InterpretationPtr allAtoms = InterpretationPtr(new Interpretation(*program.edb));
	BOOST_FOREACH (ID ruleID, program.idb){
		const Rule& rule = reg->rules.getByID(ruleID);
		BOOST_FOREACH (ID h, rule.head) allAtoms->setFact(h.address);
		BOOST_FOREACH (ID b, rule.body) allAtoms->setFact(b.address);
		if (rule.head.size() == 2 && rule.head[0].isExternalAuxiliary() && rule.head[1].isExternalAuxiliary()){
			// skip EA guessing rules
		}else{
			remainingRules.insert(ruleID);
		}
	}

	// now construct the fixpoint
	InterpretationPtr fixpoint = InterpretationPtr(new Interpretation(reg));
	InterpretationPtr assigned = InterpretationPtr(new Interpretation(reg));

	// all false atoms and all facts are immediately set
	assigned->getStorage() |= (interpretation->getStorage() ^ allAtoms->getStorage());
	DBGLOG(DBG, "Initially false: " << *assigned);
	assigned->getStorage() |= program.edb->getStorage();
	fixpoint->getStorage() |= program.edb->getStorage();
	DBGLOG(DBG, "Initial interpretation: " << *fixpoint);

	// fixpoint iteration
	bool changed = true;
	std::vector<bool> eaVerified;
	for (int i = 0; i < factory.innerEatoms.size(); ++i) eaVerified.push_back(false);
	while (remainingRules.size() > 0 && changed){
		changed = false;

		// check if an external atom is verified
		int eaIndex = 0;
		BOOST_FOREACH (ID eatomID, factory.innerEatoms){
			if (!eaVerified[eaIndex]){
				DBGLOG(DBG, "Checking if external atom " << eatomID << " is verified");
				const ExternalAtom& eatom = reg->eatoms.getByID(eatomID);
				eatom.updatePredicateInputMask();
				if ((eatom.getPredicateInputMask()->getStorage() & assigned->getStorage()).count() == eatom.getPredicateInputMask()->getStorage().count()){
					DBGLOG(DBG, "external atom " << eatomID << " is verified");
					// set all output atoms as verified
					eaMasks[eaIndex].updateMask();
					bm::bvector<>::enumerator en = eaMasks[eaIndex].mask()->getStorage().first();
					bm::bvector<>::enumerator en_end = eaMasks[eaIndex].mask()->getStorage().end();
					while (en < en_end){
						if (reg->ogatoms.getIDByAddress(*en).isExternalAuxiliary()){
							DBGLOG(DBG, "External atom " << eatomID << " implies " << *en << "=" << interpretation->getFact(*en));
							assigned->setFact(*en);
							if (interpretation->getFact(*en)){
								fixpoint->setFact(*en);
							}
						}
						en++;
					}
				}
				eaVerified[eaIndex] = true;
			}
			eaIndex++;
		}

		// search for a rule with satisfied body
		BOOST_FOREACH (ID ruleID, remainingRules){
			DBGLOG(DBG, "Checking applicability of rule " << ruleID);
			// check if the body is satisfied
			const Rule& rule = reg->rules.getByID(ruleID);
			bool bodySatisfied = true;
			BOOST_FOREACH (ID b, rule.body){
				if (assigned->getFact(b.address)){
					if (fixpoint->getFact(b.address) == b.isNaf()){
						DBGLOG(DBG, "Atom " << b.address << " is " << (fixpoint->getFact(b.address) ? "true" : "false") << " but should be " << (b.isNaf() ? "false" : "true"));
						bodySatisfied = false;
						break;
					}else{
						DBGLOG(DBG, "Satisfied atom " << b.address << " is " << (fixpoint->getFact(b.address) ? "true" : "false"));
					}
				}else{
					DBGLOG(DBG, "Atom " << b.address << " is unassigned");
					bodySatisfied = false;
					break;
				}
			}
			if (bodySatisfied){
				DBGLOG(DBG, "Rule body satisfied: " << ruleID);
				// set head literal, if all other head literals are known to be false
				ID impliedAtom = ID_FAIL;
				BOOST_FOREACH (ID h, rule.head){
					if (!assigned->getFact(h.address)){
						if (impliedAtom != ID_FAIL){
							DBGLOG(DBG, "Skipping choice rule " << ruleID << ": Multiple unassigned head literals");
							impliedAtom = ID_FAIL;
							break;
						}
						impliedAtom = h;
					}else{
						if (fixpoint->getFact(h.address)){
							DBGLOG(DBG, "Skipping choice rule " << ruleID << ": Head already satisfied");
							break;
						}
					}
				}

				// rule was processed: remove it
				assert (std::find(remainingRules.begin(), remainingRules.end(), ruleID) != remainingRules.end());
				remainingRules.erase(std::find(remainingRules.begin(), remainingRules.end(), ruleID));

				// derive head atom if the choice is unique
				if (impliedAtom != ID_FAIL){
					DBGLOG(DBG, "Rule " << ruleID << " implies " << impliedAtom.address);
					fixpoint->setFact(impliedAtom.address);
					assigned->setFact(impliedAtom.address);
					changed = true;
					break;
				}
			}
		}
	}

	// remove external auxiliaries
	bm::bvector<>::enumerator en = fixpoint->getStorage().first();
	bm::bvector<>::enumerator en_end = fixpoint->getStorage().end();
	while (en < en_end){
		if (reg->ogatoms.getIDByAddress(*en).isExternalAuxiliary()){
			fixpoint->clearFact(*en);
		}
		en++;
	}

	DBGLOG(DBG, "Fixpoint is: " << *fixpoint);

	return fixpoint;
}

DLVHEX_NAMESPACE_END
