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
 * @file GenuineGuessAndCheckModelGenerator.cpp
 * @author Christoph Redl
 *
 * @brief Implementation of the model generator for "GuessAndCheck" components.
 */

#define DLVHEX_BENCHMARK

#include "dlvhex2/GenuineGuessAndCheckModelGenerator.h"
#include "dlvhex2/Logger.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/ASPSolver.h"
#include "dlvhex2/ASPSolverManager.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/PluginInterface.h"
#include "dlvhex2/Benchmarking.h"
#include "dlvhex2/InternalGroundDASPSolver.h"

#include <bm/bmalgo.h>

#include <boost/foreach.hpp>

DLVHEX_NAMESPACE_BEGIN

/**
 * for one eval unit, we transform the rules (idb) independent of input
 * interpretations as follows:
 * * replace all external atoms with eatom replacements
 *   -> "xidb" (like in other model generators)
 * * create for each inner eatom a guessing rule for grounding and guessing
 *   eatoms
 *   -> "gidb"
 * * create for each rule in xidb a rule with same body and individual
 *   flp auxiliary head containing all variables in the rule
 *   (constraints can stay untouched)
 *   -> "xidbflphead"
 * * create for each rule in xidb a rule with body extended by respective
 *   flp auxiliary predicate containing all variables
 *   -> "xidbflpbody"
 *
 * evaluation works as follows:
 * * evaluate outer eatoms -> yields eedb replacements in interpretation
 * * evaluate edb + eedb + xidb + gidb -> yields guesses M_1,...,M_n
 * * check for each guess M
 *   * whether eatoms have been guessed correctly (remove others)
 *   * whether M is model of FLP reduct of xidb wrt edb, eedb and M
 *     this check is achieved by doing the following
 *     * evaluate edb + eedb + xidbflphead + M
 *       -> yields singleton answer set containing flp heads F for non-blocked rules
 *       (if there is no result answer set, some constraint fired and M can be discarded)
 *     * evaluate edb + eedb + xidbflpbody + (M \cap guess_auxiliaries) + F
 *       -> yields singleton answer set M'
 *       (there must be an answer set, or something went wrong)
 *     * if (M' \setminus F) == M then M is a model of the FLP reduct
 *       -> store as candidate
 * * drop non-subset-minimal candidates
 * * return remaining candidates as minimal models
 *   (this means, that for one input, all models have to be calculated
 *    before the first one can be returned due to the minimality check)
 */

namespace
{

void createEatomGuessingRules(
    RegistryPtr reg,
    const std::vector<ID>& idb,
    const std::vector<ID>& innerEatoms,
    std::vector<ID>& gidb,
    PredicateMask& gpmask,
    PredicateMask& gnmask);

void createFLPRules(
    RegistryPtr reg,
    const std::vector<ID>& xidb,
    std::vector<ID>& xidbflphead,
    std::vector<ID>& xidbflpbody,
    PredicateMask& fmask);

}

GenuineGuessAndCheckModelGeneratorFactory::GenuineGuessAndCheckModelGeneratorFactory(
    ProgramCtx& ctx,
    const ComponentInfo& ci,
    ASPSolverManager::SoftwareConfigurationPtr externalEvalConfig):
  externalEvalConfig(externalEvalConfig),
  ctx(ctx),
  ci(ci)
{
  // this model generator can handle any components
  // (and there is quite some room for more optimization)

  RegistryPtr reg = ctx.registry();
  gpMask.setRegistry(reg);
  gnMask.setRegistry(reg);
  fMask.setRegistry(reg);

  outerEatoms = ci.outerEatoms;

  // copy rules and constraints to idb
  // TODO we do not really need this except for debugging (tiny optimization possibility)
  idb.reserve(ci.innerRules.size() + ci.innerConstraints.size());
  idb.insert(idb.end(), ci.innerRules.begin(), ci.innerRules.end());
  idb.insert(idb.end(), ci.innerConstraints.begin(), ci.innerConstraints.end());

  innerEatoms = ci.innerEatoms;
  // create guessing rules "gidb" for innerEatoms in all inner rules and constraints
  createEatomGuessingRules(reg, idb, innerEatoms, gidb, gpMask, gnMask);

  // transform original innerRules and innerConstraints to xidb with only auxiliaries
  xidb.reserve(ci.innerRules.size() + ci.innerConstraints.size());
  std::back_insert_iterator<std::vector<ID> > inserter(xidb);
  std::transform(ci.innerRules.begin(), ci.innerRules.end(),
      inserter, boost::bind(&GenuineGuessAndCheckModelGeneratorFactory::convertRule, this, reg, _1));
  std::transform(ci.innerConstraints.begin(), ci.innerConstraints.end(),
      inserter, boost::bind(&GenuineGuessAndCheckModelGeneratorFactory::convertRule, this, reg, _1));

  // create cache
  xgidb.insert(xgidb.end(), xidb.begin(), xidb.end());
  xgidb.insert(xgidb.end(), gidb.begin(), gidb.end());

  // transform xidb for flp calculation
  createFLPRules(reg, xidb, xidbflphead, xidbflpbody, fMask);

  DBGLOG(DBG,"GenuineGuessAndCheckModelGeneratorFactory():");
  #ifndef NDEBUG
  {
    DBGLOG_INDENT(DBG);
    // verbose output
    std::stringstream s;
    print(s, true);
    DBGLOG(DBG,s.str());
  }
  #endif
}

namespace
{

/**
 * go through all rules with external atoms
 * for each such rule and each inner eatom in the body:
 * * collect all variables in the eatom (input and output)
 * * collect all positive non-external predicates in the rule body containing these variables
 * * build rule <aux_ext_eatompos>(<all variables>) v <aux_ext_eatomneg>(<all variables>) :- <all bodies>
 * * store into gidb
 */
void createEatomGuessingRules(
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
void createFLPRules(
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
    if( rid.isConstraint() ||
        r.body.empty() )
    {
      // keep constraints and disjunctive facts as they are
      xidbflphead.push_back(rid);
      xidbflpbody.push_back(rid);
    }
    else if( rid.isRegularRule() )
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

      Rule rflpbody(
          ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR | ID::PROPERTY_AUX);
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

}

std::ostream& GenuineGuessAndCheckModelGeneratorFactory::print(
    std::ostream& o) const
{
  return print(o, false);
}

std::ostream& GenuineGuessAndCheckModelGeneratorFactory::print(
    std::ostream& o, bool verbose) const
{
  // item separator
  std::string isep(" ");
  // group separator
  std::string gsep(" ");
  if( verbose )
  {
    isep = "\n";
    gsep = "\n";
  }
  RawPrinter printer(o, ctx.registry());
  if( !outerEatoms.empty() )
  {
    o << "outer Eatoms={" << gsep;
    printer.printmany(outerEatoms,isep);
    o << gsep << "}" << gsep;
  }
  if( !innerEatoms.empty() )
  {
    o << "inner Eatoms={" << gsep;
    printer.printmany(innerEatoms,isep);
    o << gsep << "}" << gsep;
  }
  if( !gidb.empty() )
  {
    o << "gidb={" << gsep;
    printer.printmany(gidb,isep);
    o << gsep << "}" << gsep;
  }
  if( !idb.empty() )
  {
    o << "idb={" << gsep;
    printer.printmany(idb,isep);
    o << gsep << "}" << gsep;
  }
  if( !xidb.empty() )
  {
    o << "xidb={" << gsep;
    printer.printmany(xidb,isep);
    o << gsep << "}" << gsep;
  }
  if( !xidbflphead.empty() )
  {
    o << "xidbflphead={" << gsep;
    printer.printmany(xidbflphead,isep);
    o << gsep << "}" << gsep;
  }
  if( !xidbflpbody.empty() )
  {
    o << "xidbflpbody={" << gsep;
    printer.printmany(xidbflpbody,isep);
    o << gsep << "}" << gsep;
  }
  return o;
}

GenuineGuessAndCheckModelGenerator::GenuineGuessAndCheckModelGenerator(
    Factory& factory,
    InterpretationConstPtr input):
  BaseModelGenerator(input),
  factory(factory)
{
    DBGLOG(DBG, "Genuine GnC-ModelGenerator is instantiated for a " << (factory.ci.disjunctiveHeads ? "" : "non-") << "disjunctive component");

    RegistryPtr reg = factory.ctx.registry();

    // create new interpretation as copy
    if( input == 0 )
    {
      // empty construction
      postprocessedInput.reset(new Interpretation(reg));
    }
    else
    {
      // copy construction
      postprocessedInput.reset(new Interpretation(*input));
    }

    // augment input with edb
    #warning perhaps we can pass multiple partially preprocessed input edb's to the external solver and save a lot of processing here
    postprocessedInput->add(*factory.ctx.edb);

    // remember which facts we must remove
    //InterpretationConstPtr
    mask.reset(new Interpretation(*postprocessedInput));

    // manage outer external atoms
    if( !factory.outerEatoms.empty() )
    {
      // augment input with result of external atom evaluation
      // use newint as input and as output interpretation
      IntegrateExternalAnswerIntoInterpretationCB cb(postprocessedInput);
      evaluateExternalAtoms(reg,
          factory.outerEatoms, postprocessedInput, cb);
      DLVHEX_BENCHMARK_REGISTER(sidcountexternalatomcomps,
          "outer external atom computations");
      DLVHEX_BENCHMARK_COUNT(sidcountexternalatomcomps,1);

      assert(!factory.xidb.empty() &&
          "the guess and check model generator is not required for "
          "non-idb components! (use plain)");
    }

    // evaluate edb+xidb+gidb
    ASPSolverManager::ResultsPtr guessres;
    {
	DBGLOG(DBG,"evaluating guessing program");
	// no mask
	OrdinaryASPProgram program(reg, factory.xgidb, postprocessedInput, factory.ctx.maxint);

//	grounder = InternalGrounderPtr(new InternalGrounder(factory.ctx, program));
//	if (factory.ctx.config.getOption("Instantiate")){
//		std::cout << "% Component " << &(factory.ci) << std::endl;
//		std::cout << grounder->getGroundProgramString();
//	}

//	OrdinaryASPProgram gprogram = grounder->getGroundProgram();
//	igas = InternalGroundDASPSolverPtr(new InternalGroundDASPSolver(factory.ctx, gprogram));
	solver = GenuineSolver::getInstance(factory.ctx, program);
	factory.ctx.globalNogoods.addNogoodListener(solver);
	if (factory.ctx.config.getOption("ExternalLearningPartial")){
		solver->addExternalLearner(this);
	}

//Nogood ng1;
//ng1.insert(solver->createLiteral(11));
//factory.ctx.globalNogoods.addNogood(ng1);

	firstLearnCall = true;
    }
}

GenuineGuessAndCheckModelGenerator::~GenuineGuessAndCheckModelGenerator(){
	factory.ctx.globalNogoods.removeNogoodListener(solver);
	if (factory.ctx.config.getOption("ExternalLearningPartial")){
		solver->removeExternalLearner(this);
	}
	DBGLOG(DBG, "Final Statistics:" << std::endl << solver->getStatistics());
}

namespace
{

// for usual model building where we want to collect all true answers
// as replacement atoms in an interpretation
struct VerifyExternalAnswerAgainstPosNegGuessInterpretationCB:
  public BaseModelGenerator::ExternalAnswerTupleCallback
{
  VerifyExternalAnswerAgainstPosNegGuessInterpretationCB(
      InterpretationPtr guess_pos,
      InterpretationPtr guess_neg);
  virtual ~VerifyExternalAnswerAgainstPosNegGuessInterpretationCB() {}
  // remembers eatom and prepares replacement.tuple[0]
  virtual bool eatom(const ExternalAtom& eatom);
  // remembers input
  virtual bool input(const Tuple& input);
  // creates replacement ogatom and activates respective bit in output interpretation
  virtual bool output(const Tuple& output);
protected:
  RegistryPtr reg;
  InterpretationPtr guess_pos, guess_neg;
  ID pospred, negpred;
  OrdinaryAtom replacement;
};

VerifyExternalAnswerAgainstPosNegGuessInterpretationCB::
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
VerifyExternalAnswerAgainstPosNegGuessInterpretationCB::
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
VerifyExternalAnswerAgainstPosNegGuessInterpretationCB::
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
VerifyExternalAnswerAgainstPosNegGuessInterpretationCB::
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

} // anonymous namespace

// generate and return next model, return null after last model
// see description of algorithm on top of this file
InterpretationPtr GenuineGuessAndCheckModelGenerator::generateNextModel()
{
	if (solver == GenuineSolverPtr()) return InterpretationPtr();

	// for non-disjunctive components, generate one model and return it
	// for disjunctive components, generate all modes, do minimality check, and return one model
	if (factory.ci.disjunctiveHeads){
		DBGLOG(DBG, "Solving disjunctive component by GnC Model Generator");

		if( currentResults == 0 )
		{
			// Generate all compatible models
			InterpretationPtr cm;
			while ((cm = generateNextCompatibleModel()) != InterpretationPtr()){
				candidates.push_back(cm);
			} 

			// minimality check
			DBGLOG(DBG, "Doing minimality check");
			typedef std::list<InterpretationPtr> CandidateList;
			std::set<InterpretationPtr> erase;
			CandidateList::iterator it;
			for(it = candidates.begin(); it != candidates.end(); ++it)
			{
				DBGLOG(DBG,"checking with " << **it);
				for(CandidateList::iterator itv = candidates.begin();
				    itv != candidates.end(); ++itv)
				{
					// do not check against self
					if( itv == it ) continue;

					// (do not check against those already invalidated)
					if( erase.find(*itv) != erase.end() ) continue;

					DBGLOG(DBG,"  does it invalidate " << **itv << "?");

					// any_sub(b1, b2) checks if there is any bit in the bitset obtained by 'b1 - b2'
					// if this is not the case, we know that 'b1 \subseteq b2'
					if( !bm::any_sub( (*it)->getStorage(), (*itv)->getStorage() ) )
					{
						DBGLOG(DBG,"  yes it invalidates!");
						erase.insert(*itv);
					}
				}
			}
			// now all that must be erased are in set 'erase'

			DBGLOG(DBG,"minimal models are:");
			PreparedResults* pr = new PreparedResults;
			currentResults.reset(pr);
			BOOST_FOREACH(InterpretationPtr mdl, candidates)
			{
				if( erase.find(mdl) == erase.end() )
				{
					DBGLOG(DBG,"  " << *mdl);
					pr->add(AnswerSetPtr(new AnswerSet(mdl)));
				}
			}
		}

		assert(currentResults != 0);
		AnswerSet::Ptr ret = currentResults->getNextAnswerSet();
		if( ret == 0 )
		{
			currentResults.reset();
			return InterpretationPtr();
		}
		DLVHEX_BENCHMARK_REGISTER(sidcountgcanswersets,
		"GenuineGuessAndCheckMG answer sets");
		DLVHEX_BENCHMARK_COUNT(sidcountgcanswersets,1);

		return ret->interpretation;
	}else{
		DBGLOG(DBG, "Solving non-disjunctive component by GnC Model Generator");

		return generateNextCompatibleModel();
	}
}

InterpretationPtr GenuineGuessAndCheckModelGenerator::generateNextCompatibleModel()
{
	RegistryPtr reg = factory.ctx.registry();

	// now we have postprocessed input in postprocessedInput
	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidgcsolve, "guess and check loop");

	InterpretationPtr modelCandidate;
	bool aborted;
	do{
		modelCandidate = solver->projectToOrdinaryAtoms(solver->getNextModel());
		DBGLOG(DBG, "Statistics:" << std::endl << solver->getStatistics());
		if (modelCandidate == InterpretationPtr()) return modelCandidate;

		DBGLOG_SCOPE(DBG,"gM", false);
		DBGLOG(DBG,"= got guess model " << *modelCandidate);

		// project to pos and neg eatom replacements for validation
		InterpretationPtr projint(new Interpretation(reg));
		projint->getStorage() = modelCandidate->getStorage() - postprocessedInput->getStorage();

		factory.gpMask.updateMask();
		InterpretationPtr projectedModelCandidate_pos(new Interpretation(reg));
		projectedModelCandidate_pos->getStorage() = projint->getStorage() & factory.gpMask.mask()->getStorage();
		InterpretationPtr projectedModelCandidate_pos_val(new Interpretation(reg));
		projectedModelCandidate_pos_val->getStorage() = projectedModelCandidate_pos->getStorage();
		DBGLOG(DBG,"projected positive guess: " << *projectedModelCandidate_pos);

		factory.gnMask.updateMask();
		InterpretationPtr projectedModelCandidate_neg(new Interpretation(reg));
		projectedModelCandidate_neg->getStorage() = projint->getStorage() & factory.gnMask.mask()->getStorage();
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
		aborted = !evaluateExternalAtoms(reg, factory.innerEatoms, modelCandidate, cb, &factory.ctx, factory.ctx.config.getOption("ExternalLearning") ? solver : GenuineSolverPtr());

		if (projectedModelCandidate_pos_val->getStorage().count() > 0){
			aborted = true;
		}

		// FLP check
		if (factory.ctx.config.getOption("FLPCheck")){
			DBGLOG(DBG, "FLP Check");

			modelCandidate->getStorage() -= projectedModelCandidate_neg->getStorage();
			/*
			* see documentation at top: FLP check
			* * evaluate edb + xidbflphead + M
			*   -> yields singleton answer set containing flp heads F for non-blocked rules
			*   (if there is no result answer set, some constraint fired and M can be discarded)
			* * evaluate edb + xidbflpbody + (M \cap pos_guess_auxiliaries) + F
			*   -> yields singleton answer set M' (we ignore input facts)
			*   (there must be an answer set, or something went wrong)
			* * if M' == M (ignoring input facts "edb \cup pos_guess_auxiliaries \cup flpheads")
			*   then M is a model of the FLP reduct
			*   -> store as candidate
			*/
			InterpretationPtr flpas;
			{
				DBGLOG(DBG,"evaluating flp head program");

				// here we can mask, we won't lose FLP heads
				OrdinaryASPProgram flpheadprogram(reg, factory.xidbflphead, modelCandidate, factory.ctx.maxint, mask);
				//InternalGrounder ig(factory.ctx, flpheadprogram);
				//OrdinaryASPProgram flpheadgprogram = ig.getGroundProgram();
				//InternalGroundDASPSolver flpheadigas(factory.ctx, flpheadgprogram);
				GenuineSolverPtr flpheadsolver = GenuineSolver::getInstance(factory.ctx, flpheadprogram);

				flpas = flpheadsolver->projectToOrdinaryAtoms(flpheadsolver->getNextModel());
				if( flpas == InterpretationPtr() )
				{
					DBGLOG(DBG, "FLP head program yielded no answer set");
					aborted = true;
				}else{
					DBGLOG(DBG, "FLP head program yielded at least one answer set");
				}
			}
			DBGLOG(DBG,"got FLP head model " << *flpas);


			// evaluate xidbflpbody+edb+flp+eatomguess
			ASPSolverManager::ResultsPtr flpbodyres;
			{
				DBGLOG(DBG, "evaluating flp body program");

				// build edb+flp+eatomguess (by keeping of guessint only input and flp bits)
				Interpretation::Ptr edbflpguess(new Interpretation(*modelCandidate));
				edbflpguess->getStorage() &= (flpas->getStorage() | factory.gpMask.mask()->getStorage());

				// here we can also mask, this eliminates many equal bits for comparisons later
				OrdinaryASPProgram flpbodyprogram(reg, factory.xidbflpbody, edbflpguess, factory.ctx.maxint, mask);
				//InternalGrounder ig(factory.ctx, flpbodyprogram);
				//OrdinaryASPProgram flpbodygprogram = ig.getGroundProgram();
				//InternalGroundDASPSolver flpbodyigas(factory.ctx, flpbodygprogram);
				GenuineSolverPtr flpbodysolver = GenuineSolver::getInstance(factory.ctx, flpbodyprogram);

				// for comparing the flpbody answer sets we mask the guess interpretation
				modelCandidate->getStorage() -= mask->getStorage();

				DBGLOG(DBG, "comparing xidbflpbody answer sets with guess interpretation " << *modelCandidate);

				InterpretationPtr flpbodyas = flpbodysolver->projectToOrdinaryAtoms(flpbodysolver->getNextModel());
				while(flpbodyas != InterpretationPtr())
				{
					// now we make sure nothing from EDB is left in flpbodyint
					// (if an edb fact is derived in a rule, it will be returned as "derived fact" and we will get a mismatch here)
					// (e.g., "foo(x). bar(x). foo(X) :- bar(X)." will give "foo(x)" as "answer set without EDB facts"
					//flpbodyint->getStorage() -= factory.ctx.edb->getStorage();

					DBGLOG(DBG,"checking xidbflpbody answer set " << *flpbodyas);
					DBGLOG(DBG,"model candidate is " << *modelCandidate);
					if( flpbodyas->getStorage() == modelCandidate->getStorage() )
					{
					  DBGLOG(DBG, "found (not yet minimalitychecked) FLP model " << *modelCandidate);
					  // we found one -> no need to check for more
					  break;
					}
					else
					{
					  DBGLOG(DBG, "this FLP body model is not equal to guess interpretation -> continuing check");
					}
					flpbodyas = flpbodysolver->projectToOrdinaryAtoms(flpbodysolver->getNextModel());
				}
				if (flpbodyas == InterpretationPtr()){
					aborted = true;
				}
			}
		}else{
			DBGLOG(DBG, "Skipping FLP Check");
		}

		if (!aborted){

			// remove positive and negative guess
			modelCandidate->getStorage() -= projectedModelCandidate_pos->getStorage();
			modelCandidate->getStorage() -= projectedModelCandidate_neg->getStorage();

			// remove edb
			modelCandidate->getStorage() -= mask->getStorage();

			DBGLOG(DBG,"= final model candidate " << *modelCandidate);
		}
	}while(aborted);

	return modelCandidate;
}

bool GenuineGuessAndCheckModelGenerator::learn(Interpretation::Ptr partialInterpretation, const bm::bvector<>& factWasSet, const bm::bvector<>& changed){

	RegistryPtr reg = factory.ctx.registry();

	// go through all external atoms
	bool learned = false;
	BOOST_FOREACH(ID eatomid, factory.innerEatoms)
	{
		const ExternalAtom& eatom = reg->eatoms.getByID(eatomid);
		eatom.updatePredicateInputMask();

		// check if input for external atom is complete
		DBGLOG(DBG, "Checking if input for " << eatom << " is complete");

#ifndef NDEBUG
		bm::bvector<>::enumerator en = factWasSet.first();
		bm::bvector<>::enumerator en_end = factWasSet.end();
		std::stringstream ss;
		ss << "Available input: { ";
		bool first = true;
		while (en < en_end){
			if (!first) ss << ", ";
			ss << *en;
			first = false;
			en++;
		}
		ss << " }";

		en = eatom.getPredicateInputMask()->getStorage().first();
		eatom.getPredicateInputMask()->getStorage().end();
		ss << std::endl << "Needed input: { ";
		first = true;
		while (en < en_end){
			if (!first) ss << ", ";
			ss << *en;
			first = false;
			en++;
		}
		ss << " }";
		DBGLOG(DBG, ss.str());
#endif




/*

// TODO: The if block below checks if the predicate input to the external atom changed, but not the aux input.
//       If the predicate input changed, the external atom is reevaluated, otherwise it is not.
//       As the aux input is not considered, we might miss some changes in the input and do not evaluate the external atom for learning, even though we could learn something new.
//       This loop detects whether the aux input changed. However, the computational overhead is big. This needs to be considered when the heuristic is developed.

bool auxChanged = false;

  dlvhex::OrdinaryAtomTable::PredicateIterator it, it_end;
  for(boost::tie(it, it_end) = reg->ogatoms.getRangeByPredicateID(eatom.auxInputPredicate); it != it_end; ++it)
  {
    const dlvhex::OrdinaryAtom& oatom = *it;
    ID idoatom = reg->ogatoms.getIDByStorage(oatom);
    if (changed.get_bit(idoatom.address) != 0) auxChanged = true;
  }

*/






		if ((eatom.getPredicateInputMask()->getStorage() & factWasSet).count() == eatom.getPredicateInputMask()->getStorage().count()){
			DBGLOG(DBG, "Input is complete");

			// check if at least one input fact changed (otherwise a reevaluation is pointless)
			if (/* auxChanged || */ firstLearnCall || (eatom.getPredicateInputMask()->getStorage() & changed).count() > 0){
// eatom.getPredicateInputMask()->getStorage().count() == 0
				DBGLOG(DBG, "Evaluating external atom");
				InterpretationPtr eaResult(new Interpretation(reg));
				IntegrateExternalAnswerIntoInterpretationCB intcb(eaResult);
				int i = solver->getNogoodCount();
				evaluateExternalAtom(reg, eatom, partialInterpretation, intcb, &factory.ctx, factory.ctx.config.getOption("ExternalLearning") ? solver : NogoodContainerPtr());
				DBGLOG(DBG, "Output has size " << eaResult->getStorage().count());
				if (solver->getNogoodCount() != i) learned = true;
			}else{
				DBGLOG(DBG, "Do not evaluate external atom because input did not change");
			}
		}else{
			DBGLOG(DBG, "Input is not complete");
		}
	}

	firstLearnCall = false;
	return learned;
}

DLVHEX_NAMESPACE_END
