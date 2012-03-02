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
 * @file InconsistencyAnalyzer.cpp
 * @author Christoph Redl
 *
 * @brief Analyzes the inconsistency in a program wrt. selected input facts.
 */

#include "dlvhex2/InconsistencyAnalyzer.h"

#include "dlvhex2/HexParser.h"
#include "dlvhex2/Printer.h"

DLVHEX_NAMESPACE_BEGIN

InconsistencyAnalyzer::InconsistencyAnalyzer(ProgramCtx& ctx) : ctx(ctx) {
	reg = ctx.registry();
}

void InconsistencyAnalyzer::registerTerms(){

	// register predicate constants
	true_id = reg->storeConstantTerm("true");
	false_id = reg->storeConstantTerm("false");
	undef_id = reg->storeConstantTerm("undef");

/*
	Term trueTerm(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, "true");
	ID true_id = reg->storeTerm(trueTerm);
	Term falseTerm(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, "false");
	ID false_id = reg->storeTerm(falseTerm);
	Term undefTerm(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, "undef");
	ID undef_id = reg->storeTerm(undefTerm);
*/

	atom_id = reg->storeConstantTerm("atom");
	rule_id = reg->storeConstantTerm("rule");
	head_id = reg->storeConstantTerm("head");
	bodyP_id = reg->storeConstantTerm("bodyP");
	bodyN_id = reg->storeConstantTerm("bodyN");

/*
	Term atomTerm(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, "atom");
	ID atom_id = reg->storeTerm(atomTerm);
	Term ruleTerm(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, "rule");
	ID rule_id = reg->storeTerm(ruleTerm);
	Term headTerm(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, "head");
	ID head_id = reg->storeTerm(headTerm);
	Term bodyPTerm(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, "bodyP");
	ID bodyP_id = reg->storeTerm(bodyPTerm);
	Term bodyNTerm(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, "bodyN");
	ID bodyN_id = reg->storeTerm(bodyNTerm);
*/


#if 0
	// ap
	ID ruleBlocked_id = reg->storeConstantTerm("ruleBlocked");
	ID ruleApplicable_id = reg->storeConstantTerm("ruleApplicable");
	ID rulePossiblyApplicable_id = reg->storeConstantTerm("rulePossiblyApplicable");
	ID ruleHasHead_id = reg->storeConstantTerm("ruleHasHead");
	ID ruleSomeHInI_id = reg->storeConstantTerm("ruleSomeHInI");
	ID ruleSomeHUndef_id = reg->storeConstantTerm("ruleSomeHUndef");
	ID ruleViolated_id = reg->storeConstantTerm("ruleViolated");

	// supp
	ID litOtherHInI_id = reg->storeConstantTerm("litOtherHInI");
	ID litSupported_id = reg->storeConstantTerm("litSupported");
	ID litPossiblySupported_id = reg->storeConstantTerm("litPossiblySupported");
	ID litUnsupported_id = reg->storeConstantTerm("litUnsupported");

	// noas
	ID noAnswerSet_id = reg->storeConstantTerm("noAnswerSet");
#endif
}

void InconsistencyAnalyzer::registerAtom(InterpretationPtr explEDB, std::vector<ID>& explIDB, ID atomId){
	OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
	atom.tuple.push_back(atom_id);
	atom.tuple.push_back(ID::termFromInteger(atomId.address));
	explEDB->setFact(reg->storeOrdinaryGAtom(atom).address);

	allAtoms->setFact(atomId.address);
}

void InconsistencyAnalyzer::registerRule(InterpretationPtr explEDB, std::vector<ID>& explIDB, ID ruleId){

	const Rule& rule = reg->rules.getByID(ruleId);

	// register atoms in the rule
	BOOST_FOREACH (ID h, rule.head){
		registerAtom(explEDB, explIDB, h);
	}
	BOOST_FOREACH (ID b, rule.body){
		registerAtom(explEDB, explIDB, b);
	}

	// register rule
	{
		OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
		atom.tuple.push_back(rule_id);
		atom.tuple.push_back(ID::termFromInteger(ruleId.address));
		explEDB->setFact(reg->storeOrdinaryGAtom(atom).address);
	}

	// add facts to encode which atoms occur in the rule
	BOOST_FOREACH (ID h, rule.head){
		OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
		atom.tuple.push_back(head_id);
		atom.tuple.push_back(ID::termFromInteger(ruleId.address));
		atom.tuple.push_back(ID::termFromInteger(h.address));
		explEDB->setFact(reg->storeOrdinaryGAtom(atom).address);
	}
	BOOST_FOREACH (ID b, rule.body){
		OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
		atom.tuple.push_back(b.isNaf() ? bodyN_id : bodyP_id);
		atom.tuple.push_back(ID::termFromInteger(ruleId.address));
		atom.tuple.push_back(ID::termFromInteger(b.address));
		explEDB->setFact(reg->storeOrdinaryGAtom(atom).address);
	}
}

void InconsistencyAnalyzer::registerExplanationAtoms(InterpretationPtr explEDB, std::vector<ID>& explIDB, InterpretationConstPtr progEDB, InterpretationConstPtr exAt){

	// explanation atoms either 1. have the same true value as in the EDB of the program or 2. are undefined
	// non-explanation atoms are always undefined

	// go through all non-explanation atoms
	bm::bvector<>::enumerator en = allAtoms->getStorage().first();
	bm::bvector<>::enumerator en_end = allAtoms->getStorage().end();
	while (en < en_end){
		if (!exAt->getFact(*en)){
			OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
			atom.tuple.push_back(undef_id);
			atom.tuple.push_back(ID::termFromInteger(*en));
			explEDB->setFact(reg->storeOrdinaryGAtom(atom).address);
		}
		en++;
	}

	// go through all explanation atoms
	en = exAt->getStorage().first();
	exAt->getStorage().end();
	while (en < en_end){
		// eliminate the wrong truth value
		{
			Rule c(ID::MAINKIND_RULE | ID::SUBKIND_RULE_CONSTRAINT);

			{
				OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
				atom.tuple.push_back(progEDB->getFact(*en) ? false_id : true_id);
				atom.tuple.push_back(ID::termFromInteger(*en));
				c.body.push_back(reg->storeOrdinaryGAtom(atom));
			}

			explIDB.push_back(reg->storeRule(c));
		}
		en++;
	}
}

void InconsistencyAnalyzer::writeStaticProgramParts(InterpretationPtr explEDB, std::vector<ID>& explIDB){

	DBGLOG(DBG, "Parsing static program");
	InputProviderPtr ip(new InputProvider());
	ip->addStringInput(
		"% int\n"
		"true(A) :- atom(A), not false(A), not undef(A).\n"
		"false(A) :- atom(A), not true(A), not undef(A).\n"
		"undef(A) :- atom(A), not true(A), not false(A).\n"
		"\n"
		"% ap\n"
		"blocked(R) :- bodyP(R, A), false(A).\n"
		"blocked(R) :- bodyN(R, A), true(A).\n"
		"unknown(R) :- bodyP(R, A), undef(A).\n"
		"unknown(R) :- bodyN(R, A), undef(A).\n"
		"applicable(R) :- rule(R), not blocked(R), not unknown(R).\n"
		"\n"
		"% sat\n"
		"hasHead(R) :- head(R, _).\n"
		"someHInI(R) :- head(R, A), true(A).\n"
		"someHUndef(R) :- head(R, A), undef(A).\n"
		"violated(C) :- applicable(C), not hasHead(C).\n"
		"unsatisfied(C) :- applicable(C), hasHead(C), not someHInI(C), not someHUndef(C).\n"
		"\n"
		"% supp\n"
		"otherHInI(R, A1) :- head(R, A2), true(A2), head(R, A1), A1 != A2.\n"
		"supported(A) :- head(R, A), applicable(R), not otherHInI(R, A).\n"
		"possiblySupported(A) :- head(R, A), unknown(R), not otherHInI(R, A).\n"
		"unsupported(A) :- true(A), not supported(A), not possiblySupported(A).\n"
		"\n"
		"% noas\n"
		"noAnswerSet :- violated(_).\n"
		"noAnswerSet :- unsupported(_).\n"
		"noAnswerSet :- unsatisfied(_).\n"
		":- not noAnswerSet."
		"\n"
		"defined(X) :- true(X).\n"
		"defined(X) :- false(X).\n"
		":- defined(X1), defined(X2), X1 != X2."	// learn only "short" explanations
//		":- defined(X1), defined(X2), defined(X3), X1 != X2, X1 != X3, X2 != X3."	// learn only "short" explanations
//		":- defined(X1), defined(X2), defined(X3), defined(X4), X1 != X2, X1 != X3, X1 != X4, X2 != X3, X2 != X4, X3 != X4."	// learn only "short" explanations
//		":~ true(A)."
//		":~ false(A)."
		, "static");
	ProgramCtx pc;
	pc.changeRegistry(ctx.registry());
	ModuleHexParser hp;
	hp.parse(ip, pc);

	explEDB->add(*pc.edb);
	BOOST_FOREACH (ID id, pc.idb){
		explIDB.push_back(id);
	}
#if 0
return;
	// encode blocking of a rule
	{
		Rule blocked(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR);

		{
			OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
			atom.tuple.push_back(ruleBlocked_id);
			atom.tuple.push_back(reg->storeVariableTerm("R"));
			blocked.head.push_back(reg->storeOrdinaryGAtom(atom));
		}
		{
			OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
			atom.tuple.push_back(bodyP_id);
			atom.tuple.push_back(reg->storeVariableTerm("A"));
			atom.tuple.push_back(reg->storeVariableTerm("A"));
			blocked.body.push_back(reg->storeOrdinaryGAtom(atom));
		}
		{
			OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
			atom.tuple.push_back(false_id);
			atom.tuple.push_back(reg->storeVariableTerm("A"));
			blocked.body.push_back(reg->storeOrdinaryGAtom(atom));
		}
		explIDB.push_back(reg->storeRule(blocked));
	}
	{
		Rule blocked(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR);

		{
			OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
			atom.tuple.push_back(ruleBlocked_id);
			atom.tuple.push_back(reg->storeVariableTerm("R"));
			blocked.head.push_back(reg->storeOrdinaryGAtom(atom));
		}
		{
			OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
			atom.tuple.push_back(bodyN_id);
			atom.tuple.push_back(reg->storeVariableTerm("A"));
			atom.tuple.push_back(reg->storeVariableTerm("A"));
			blocked.body.push_back(reg->storeOrdinaryGAtom(atom));
		}
		{
			OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
			atom.tuple.push_back(true_id);
			atom.tuple.push_back(reg->storeVariableTerm("A"));
			blocked.body.push_back(reg->storeOrdinaryGAtom(atom));
		}
		explIDB.push_back(reg->storeRule(blocked));
	}

	// encode that the status of rules may be unknown
	{
		Rule possiblyApplicable(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR);

		{
			OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
			atom.tuple.push_back(rulePossiblyApplicable_id);
			atom.tuple.push_back(reg->storeVariableTerm("R"));
			possiblyApplicable.head.push_back(reg->storeOrdinaryGAtom(atom));
		}
		{
			OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
			atom.tuple.push_back(bodyP_id);
			atom.tuple.push_back(reg->storeVariableTerm("A"));
			atom.tuple.push_back(reg->storeVariableTerm("A"));
			possiblyApplicable.body.push_back(reg->storeOrdinaryGAtom(atom));
		}
		{
			OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
			atom.tuple.push_back(undef_id);
			atom.tuple.push_back(reg->storeVariableTerm("A"));
			possiblyApplicable.body.push_back(reg->storeOrdinaryGAtom(atom));
		}
		explIDB.push_back(reg->storeRule(possiblyApplicable));
	}
	{
		Rule possiblyApplicable(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR);

		{
			OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
			atom.tuple.push_back(rulePossiblyApplicable_id);
			atom.tuple.push_back(reg->storeVariableTerm("R"));
			possiblyApplicable.head.push_back(reg->storeOrdinaryGAtom(atom));
		}
		{
			OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
			atom.tuple.push_back(bodyN_id);
			atom.tuple.push_back(reg->storeVariableTerm("A"));
			atom.tuple.push_back(reg->storeVariableTerm("A"));
			possiblyApplicable.body.push_back(reg->storeOrdinaryGAtom(atom));
		}
		{
			OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
			atom.tuple.push_back(undef_id);
			atom.tuple.push_back(reg->storeVariableTerm("A"));
			possiblyApplicable.body.push_back(reg->storeOrdinaryGAtom(atom));
		}
		explIDB.push_back(reg->storeRule(possiblyApplicable));
	}

	// encode when a rule is applicable for sure
	{
		Rule applicable(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR);

		{
			OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
			atom.tuple.push_back(rule_id);
			atom.tuple.push_back(reg->storeVariableTerm("R"));
			applicable.head.push_back(reg->storeOrdinaryGAtom(atom));
		}
		{
			OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
			atom.tuple.push_back(ruleBlocked_id);
			atom.tuple.push_back(reg->storeVariableTerm("R"));
			applicable.body.push_back(ID(ID::MAINKIND_LITERAL | ID::NAF_MASK, reg->storeOrdinaryGAtom(atom).address));
		}
		{
			OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
			atom.tuple.push_back(rulePossiblyApplicable_id);
			atom.tuple.push_back(reg->storeVariableTerm("R"));
			applicable.body.push_back(ID(ID::MAINKIND_LITERAL | ID::NAF_MASK, reg->storeOrdinaryGAtom(atom).address));
		}
		explIDB.push_back(reg->storeRule(applicable));
	}

	// compute which rules are unsatisfied
	{
		Rule hasHead(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR);

		{
			OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
			atom.tuple.push_back(ruleHasHead_id);
			atom.tuple.push_back(reg->storeVariableTerm("R"));
			hasHead.head.push_back(reg->storeOrdinaryGAtom(atom));
		}
		{
			OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
			atom.tuple.push_back(head_id);
			atom.tuple.push_back(reg->storeVariableTerm("R"));
			atom.tuple.push_back(reg->storeVariableTerm("A"));
			hasHead.body.push_back(reg->storeOrdinaryGAtom(atom));
		}
		explIDB.push_back(reg->storeRule(hasHead));
	}
	{
		Rule someHInI(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR);

		{
			OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
			atom.tuple.push_back(ruleSomeHInI_id);
			atom.tuple.push_back(reg->storeVariableTerm("R"));
			someHInI.head.push_back(reg->storeOrdinaryGAtom(atom));
		}
		{
			OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
			atom.tuple.push_back(head_id);
			atom.tuple.push_back(reg->storeVariableTerm("R"));
			atom.tuple.push_back(reg->storeVariableTerm("A"));
			someHInI.body.push_back(reg->storeOrdinaryGAtom(atom));
		}
		{
			OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
			atom.tuple.push_back(true_id);
			atom.tuple.push_back(reg->storeVariableTerm("A"));
			someHInI.body.push_back(reg->storeOrdinaryGAtom(atom));
		}
		explIDB.push_back(reg->storeRule(someHInI));
	}
	{
		Rule someHUndef(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR);

		{
			OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
			atom.tuple.push_back(ruleSomeHUndef_id);
			atom.tuple.push_back(reg->storeVariableTerm("R"));
			someHUndef.head.push_back(reg->storeOrdinaryGAtom(atom));
		}
		{
			OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
			atom.tuple.push_back(head_id);
			atom.tuple.push_back(reg->storeVariableTerm("R"));
			atom.tuple.push_back(reg->storeVariableTerm("A"));
			someHUndef.body.push_back(reg->storeOrdinaryGAtom(atom));
		}
		{
			OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
			atom.tuple.push_back(undef_id);
			atom.tuple.push_back(reg->storeVariableTerm("A"));
			someHUndef.body.push_back(reg->storeOrdinaryGAtom(atom));
		}
		explIDB.push_back(reg->storeRule(someHUndef));
	}
	{
		Rule unsatisfied(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR);

		{
			OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
			atom.tuple.push_back(ruleViolated_id);
			atom.tuple.push_back(reg->storeVariableTerm("R"));
			unsatisfied.head.push_back(reg->storeOrdinaryGAtom(atom));
		}
		{
			OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
			atom.tuple.push_back(ruleApplicable_id);
			atom.tuple.push_back(reg->storeVariableTerm("R"));
			unsatisfied.body.push_back(reg->storeOrdinaryGAtom(atom));
		}
		{
			OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
			atom.tuple.push_back(ruleHasHead_id);
			atom.tuple.push_back(reg->storeVariableTerm("R"));
			unsatisfied.body.push_back(ID(ID::MAINKIND_LITERAL | ID::NAF_MASK, reg->storeOrdinaryGAtom(atom).address));
		}
		explIDB.push_back(reg->storeRule(unsatisfied));
	}
	{
		Rule unsatisfied(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR);

		{
			OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
			atom.tuple.push_back(ruleViolated_id);
			atom.tuple.push_back(reg->storeVariableTerm("R"));
			unsatisfied.head.push_back(reg->storeOrdinaryGAtom(atom));
		}
		{
			OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
			atom.tuple.push_back(ruleApplicable_id);
			atom.tuple.push_back(reg->storeVariableTerm("R"));
			unsatisfied.body.push_back(reg->storeOrdinaryGAtom(atom));
		}
		{
			OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
			atom.tuple.push_back(ruleHasHead_id);
			atom.tuple.push_back(reg->storeVariableTerm("R"));
			unsatisfied.body.push_back(reg->storeOrdinaryGAtom(atom));
		}
		{
			OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
			atom.tuple.push_back(ruleSomeHInI_id);
			atom.tuple.push_back(reg->storeVariableTerm("R"));
			unsatisfied.body.push_back(ID(ID::MAINKIND_LITERAL | ID::NAF_MASK, reg->storeOrdinaryGAtom(atom).address));
		}
		{
			OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
			atom.tuple.push_back(ruleSomeHUndef_id);
			atom.tuple.push_back(reg->storeVariableTerm("R"));
			unsatisfied.body.push_back(ID(ID::MAINKIND_LITERAL | ID::NAF_MASK, reg->storeOrdinaryGAtom(atom).address));
		}
		explIDB.push_back(reg->storeRule(unsatisfied));
	}

	// we are only interested in interpretations which are no answer sets
	{
		Rule noas(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR);

		{
			OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
			atom.tuple.push_back(noAnswerSet_id);
			noas.head.push_back(reg->storeOrdinaryGAtom(atom));
		}
		{
			OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
			atom.tuple.push_back(ruleViolated_id);
			atom.tuple.push_back(reg->storeVariableTerm("R"));
			noas.body.push_back(reg->storeOrdinaryGAtom(atom));
		}
	
		explIDB.push_back(reg->storeRule(noas));
	}
	{
		Rule noas(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR);

		{
			OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
			atom.tuple.push_back(noAnswerSet_id);
			noas.head.push_back(reg->storeOrdinaryGAtom(atom));
		}
		{
			OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
			atom.tuple.push_back(litUnsupported_id);
			atom.tuple.push_back(reg->storeVariableTerm("R"));
			noas.body.push_back(reg->storeOrdinaryGAtom(atom));
		}
	
		explIDB.push_back(reg->storeRule(noas));
	}
	{
		Rule noas(ID::MAINKIND_RULE | ID::SUBKIND_RULE_CONSTRAINT);

		{
			OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
			atom.tuple.push_back(noAnswerSet_id);
			noas.body.push_back(ID(ID::MAINKIND_LITERAL | ID::NAF_MASK, reg->storeOrdinaryGAtom(atom).address));
		}
	
		explIDB.push_back(reg->storeRule(noas));
	}

	// a rule is not applicable to derive A1, if some other head atom A2 is true
	{
		Rule other(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR);

		{
			OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
			atom.tuple.push_back(litOtherHInI_id);
			atom.tuple.push_back(reg->storeVariableTerm("R"));
			atom.tuple.push_back(reg->storeVariableTerm("A1"));
			other.head.push_back(reg->storeOrdinaryGAtom(atom));
		}
		{
			OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
			atom.tuple.push_back(head_id);
			atom.tuple.push_back(reg->storeVariableTerm("R"));
			atom.tuple.push_back(reg->storeVariableTerm("A2"));
			other.head.push_back(reg->storeOrdinaryGAtom(atom));
		}
		{
			OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
			atom.tuple.push_back(true_id);
			atom.tuple.push_back(reg->storeVariableTerm("A2"));
			other.head.push_back(reg->storeOrdinaryGAtom(atom));
		}
		{
			OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
			atom.tuple.push_back(head_id);
			atom.tuple.push_back(reg->storeVariableTerm("R"));
			atom.tuple.push_back(reg->storeVariableTerm("A1"));
			other.head.push_back(reg->storeOrdinaryGAtom(atom));
		}
		{
			BuiltinAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_BUILTIN);
// @TODO
			atom.tuple.push_back(ID(ID::MAINKIND_TERM | ID::SUBKIND_TERM_BUILTIN, ID::TERM_BUILTIN_NE));
			atom.tuple.push_back(reg->storeVariableTerm("A1"));
			atom.tuple.push_back(reg->storeVariableTerm("A2"));
//!!			other.head.push_back(reg->batoms.getByID(atomreg->storeOrdinaryGAtom(atom));
		}
	
		explIDB.push_back(reg->storeRule(other));
	}

	// a rule R supports a literal A, if R is applicable, A is in R's head and no other atom is in R's head
	{
		Rule supported(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR);

		{
			OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
			atom.tuple.push_back(litSupported_id);
			atom.tuple.push_back(reg->storeVariableTerm("A"));
			supported.head.push_back(reg->storeOrdinaryGAtom(atom));
		}
		{
			OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
			atom.tuple.push_back(head_id);
			atom.tuple.push_back(reg->storeVariableTerm("R"));
			atom.tuple.push_back(reg->storeVariableTerm("A"));
			supported.head.push_back(reg->storeOrdinaryGAtom(atom));
		}
		{
			OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
			atom.tuple.push_back(ruleApplicable_id);
			atom.tuple.push_back(reg->storeVariableTerm("R"));
			supported.head.push_back(reg->storeOrdinaryGAtom(atom));
		}
		{
			OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
			atom.tuple.push_back(litOtherHInI_id);
			atom.tuple.push_back(reg->storeVariableTerm("R"));
			atom.tuple.push_back(reg->storeVariableTerm("A"));
			supported.head.push_back(ID(ID::MAINKIND_LITERAL | ID::NAF_MASK, reg->storeOrdinaryGAtom(atom).address));
		}

		explIDB.push_back(reg->storeRule(supported));
	}

	// a rule R possibly supports a literal A, if R is possibly applicable, A is in R's head and no other atom is in R's head
	{
		Rule possiblySupported(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR);

		{
			OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
			atom.tuple.push_back(litPossiblySupported_id);
			atom.tuple.push_back(reg->storeVariableTerm("A"));
			possiblySupported.head.push_back(reg->storeOrdinaryGAtom(atom));
		}
		{
			OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
			atom.tuple.push_back(head_id);
			atom.tuple.push_back(reg->storeVariableTerm("R"));
			atom.tuple.push_back(reg->storeVariableTerm("A"));
			possiblySupported.head.push_back(reg->storeOrdinaryGAtom(atom));
		}
		{
			OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
			atom.tuple.push_back(rulePossiblyApplicable_id);
			atom.tuple.push_back(reg->storeVariableTerm("R"));
			possiblySupported.head.push_back(reg->storeOrdinaryGAtom(atom));
		}
		{
			OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
			atom.tuple.push_back(litOtherHInI_id);
			atom.tuple.push_back(reg->storeVariableTerm("R"));
			atom.tuple.push_back(reg->storeVariableTerm("A"));
			possiblySupported.head.push_back(ID(ID::MAINKIND_LITERAL | ID::NAF_MASK, reg->storeOrdinaryGAtom(atom).address));
		}

		explIDB.push_back(reg->storeRule(possiblySupported));
	}

	// a literal is unsupported if it is true not neither supported not possibly supported
	{
		Rule unupported(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR);

		{
			OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
			atom.tuple.push_back(litUnsupported_id);
			atom.tuple.push_back(reg->storeVariableTerm("A"));
			unupported.head.push_back(reg->storeOrdinaryGAtom(atom));
		}
		{
			OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
			atom.tuple.push_back(true_id);
			atom.tuple.push_back(reg->storeVariableTerm("A"));
			unupported.head.push_back(reg->storeOrdinaryGAtom(atom));
		}
		{
			OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
			atom.tuple.push_back(litSupported_id);
			atom.tuple.push_back(reg->storeVariableTerm("A"));
			unupported.head.push_back(ID(ID::MAINKIND_LITERAL | ID::NAF_MASK, reg->storeOrdinaryGAtom(atom).address));
		}
		{
			OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
			atom.tuple.push_back(litPossiblySupported_id);
			atom.tuple.push_back(reg->storeVariableTerm("A"));
			unupported.head.push_back(ID(ID::MAINKIND_LITERAL | ID::NAF_MASK, reg->storeOrdinaryGAtom(atom).address));
		}

		explIDB.push_back(reg->storeRule(unupported));
	}
#endif
}

Nogood InconsistencyAnalyzer::extractExplanationFromInterpretation(InterpretationConstPtr intr){

	Nogood ng;

	// go through interpretation
	bm::bvector<>::enumerator en = intr->getStorage().first();
	bm::bvector<>::enumerator en_end = intr->getStorage().end();
	while (en < en_end){
		// check if the atom is over "true" or "false"
		const OrdinaryAtom& ogatom = reg->ogatoms.getByID(ID(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, *en));
		if (ogatom.tuple.front() == true_id){
			ng.insert(NogoodContainer::createLiteral(ogatom.tuple[1].address, true));
		}
		if (ogatom.tuple.front() == false_id){
			ng.insert(NogoodContainer::createLiteral(ogatom.tuple[1].address, false));
		}
		en++;
	}

	return ng;
}

Nogood InconsistencyAnalyzer::explainInconsistency(const OrdinaryASPProgram& groundProgram, InterpretationConstPtr explainationAtoms){

	DBGLOG(DBG, "Analyze inconsistency of program:");

#ifndef NDEBUG
	std::stringstream ss;
	ss << *groundProgram.edb << std::endl;
	RawPrinter p(ss, reg);
	BOOST_FOREACH (ID ruleID, groundProgram.idb){
		p.print(ruleID);
	}
	DBGLOG(DBG, ss.str());
#endif

	DBGLOG(DBG, "With respect to explanation atoms: " << *explainationAtoms);

	DBGLOG(DBG, "Constructing analysis program");
	registerTerms();

	// explanation program
	std::vector<ID> explIDB;
	InterpretationPtr explEDB = InterpretationPtr(new Interpretation(reg));
	OrdinaryASPProgram analysisProgram(reg, explIDB, explEDB);

	// keep a list of all atoms
	allAtoms = InterpretationPtr(new Interpretation(reg));

	// register all rules and atoms
	bm::bvector<>::enumerator en = groundProgram.edb->getStorage().first();
	bm::bvector<>::enumerator en_end = groundProgram.edb->getStorage().end();
	while (en < en_end){
		registerAtom(explEDB, explIDB, ID(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, *en));

		// add the atom as fact
		std::stringstream ss;
		ss << "f" << *en;
		ID rid = reg->storeConstantTerm(ss.str());
		{
			OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
			atom.tuple.push_back(head_id);
			atom.tuple.push_back(rid);
			atom.tuple.push_back(ID::termFromInteger(*en));
			explEDB->setFact(reg->storeOrdinaryGAtom(atom).address);
		}
		{
			OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
			atom.tuple.push_back(rule_id);
			atom.tuple.push_back(rid);
			explEDB->setFact(reg->storeOrdinaryGAtom(atom).address);
		}

		en++;
	}

	BOOST_FOREACH (ID ruleId, groundProgram.idb){
		const Rule& rule = reg->rules.getByID(ruleId);
		registerRule(explEDB, analysisProgram.idb, ruleId);
	}

	// register all explanation atoms
	registerExplanationAtoms(explEDB, analysisProgram.idb, groundProgram.edb, explainationAtoms);

	// we are only interested in interpretations which are no answer sets
	writeStaticProgramParts(explEDB, analysisProgram.idb);

	DBGLOG(DBG, "Analysis program has " << analysisProgram.idb.size() << " rules");
#ifndef NDEBUG
	ss.str("");
	ss << *explEDB << std::endl;
	BOOST_FOREACH (ID ruleID, analysisProgram.idb){
		p.print(ruleID);
	}
	DBGLOG(DBG, "Analysis program: " << ss.str());
#endif

	DBGLOG(DBG, "Evaluating analysis program");
	GenuineSolverPtr solver = GenuineSolver::getInstance(ctx, analysisProgram);
	InterpretationConstPtr explanation;
	Nogood ng;
	while ((explanation = solver->getNextModel()) != InterpretationConstPtr()){
		ng = extractExplanationFromInterpretation(explanation);
		DBGLOG(DBG, "Found an explanation: " << ng);
	}
	return ng;
}

DLVHEX_NAMESPACE_END

