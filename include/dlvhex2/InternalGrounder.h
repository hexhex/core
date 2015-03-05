/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Thomas Krennwallner
 * Copyright (C) 2009, 2010 Peter Schüller
 * Copyright (C) 2011, 2012, 2013 Christoph Redl
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
 * @file   InternalGrounder.hpp
 * @author Christoph Redl <redl@kr.tuwien.ac.at>
 * 
 * @brief  Grounder for disjunctive logic programs.
 */

#ifndef INTERNALGROUNDER_HPP_INCLUDED__09122011
#define INTERNALGROUNDER_HPP_INCLUDED__09122011

#include "dlvhex2/ID.h"
#include "dlvhex2/Interpretation.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/Printhelpers.h"
#include "dlvhex2/OrdinaryASPProgram.h"
#include "dlvhex2/Set.h"

#include <vector>
#include <map>
#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>
#include "dlvhex2/GenuineSolver.h"

DLVHEX_NAMESPACE_BEGIN

/** \brief Implements a grounder without using third-party software. */
class InternalGrounder : public GenuineGrounder{
public:
	/** \brief Defines how much the grounder optimizes. */
	enum OptLevel{
		/** \brief Optimize any atoms when possible. */
		full,
		/** \brief Optimize only builtin atoms. */
		builtin,
		/** \brief No optimization. */
		none
	};

protected:
	typedef boost::unordered_map<ID, ID> Substitution;
	typedef boost::unordered_map<ID, int> Binder;

	/** \brief Nonground input program. */
	OrdinaryASPProgram inputprogram;
	/** \brief Ground output program after the grounder has finished. */
	OrdinaryASPProgram groundProgram;
	/** \brief ProgramCtx. */
	ProgramCtx& ctx;
	/** \brief Registry. */
	RegistryPtr reg;
	/** \brief Level of optimization used. */
	OptLevel optlevel;

	// dependency graph
	typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, ID> DepGraph;
	typedef DepGraph::vertex_descriptor Node;
	/** \brief Nodes of depSCC. */
	boost::unordered_map<ID, Node> depNodes;
	/** \brief Atom dependency graph. */
	DepGraph depGraph;
	/** \brief Strongly connected components of depSCC.
	  *
	  * Store for each component the contained predicates. */
	std::vector<Set<ID> > depSCC;

	// strata dependencies
	typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, int> SCCDepGraph;
	/** \brief Dependencies between program components (=program strata). */
	SCCDepGraph compDependencies;
	/** \brief Stores for each stratum the contained predicates. */
	std::vector<std::set<ID> > predicatesOfStratum;
	/** \brief Stores for each stratum the contained rules. */
	std::vector<std::set<ID> > rulesOfStratum;
	/** \brief Stores for each predicate its stratum index. */
	boost::unordered_map<ID, int> stratumOfPredicate;

	/** \brief An atom which does not occur in the program. */
	ID globallyNewAtom;
	/** \brief Stores for each predicate (=term) the set of ground atoms over this predicate which are currently derivable. */
	boost::unordered_map<ID, std::vector<ID> > derivableAtomsOfPredicate;
	/** \brief Stores for each predicate the set of non-ground rules and body positions where the predicate occurs. */
	boost::unordered_map<ID, std::set<std::pair<int, int> > > positionsOfPredicate;

	/** \brief Atoms which are definitely true (=EDB). */
	InterpretationPtr trueAtoms;

	/** \brief Generated ground rules in the current stratum. */
	std::vector<ID> groundRules;
	/** \brief Input nonground rules from the current stratum. */
	std::vector<ID> nonGroundRules;

	/** \brief Predicates from a lower stratum (all derivable atoms are known). */
	std::set<ID> groundedPredicates;
	/** \brief Completely solved predicates (all ground instances are known to be true or false)
	  * (solvedPredicates is a subset of groundedPredicates). */
	std::set<ID> solvedPredicates;

	// initialization members
	/** \brief Construct atom dependency graph. */
	void computeDepGraph();
	/** \brief Check if rule can be handled and insert variable names for anonymous variables.
	  * @param ruleID Rule to preprocess. */
	ID preprocessRule(ID ruleID);
	/** \brief Partitions the program into strata using the atom dependency graph. */
	void computeStrata();

	/** \brief Extract for all predicates the rules and atoms where it occurs. */
	void buildPredicateIndex();
	/** \brief Load stratum into groundRules and nonGroundRules.
	  * @param index Stratum to load. */
	void loadStratum(int index);


	// grounding members
	/** \brief Grounds a specific stratum.
	  * @param index Stratum to ground. */
	void groundStratum(int index);

	/** \brief Generates all ground instances of a rule.
	  * @param ruleID Rule to ground (ground or nonground).
	  * @param s Set or pairs of variables to be substituted and the values to be inserted; can be incomplete.
	  * @param groundedRules Container to receive the instance.
	  * @param newDerivableAtoms Set of atoms to be extended by those which become newly derivable by the new rule instance. */
	void groundRule(ID ruleID, Substitution& s, std::vector<ID>& groundedRules, Set<ID>& newDerivableAtoms);
	/** \brief Generates a single ground instance of a rule.
	  * @param ruleID Rule to ground (ground or nonground).
	  * @param s Complete set or pairs of variables to be substituted and the values to be inserted.
	  * @param groundedRules Container to receive the instance.
	  * @param newDerivableAtoms Set of atoms to be extended by those which become newly derivable by the new rule instance. */
	void buildGroundInstance(ID ruleID, Substitution s, std::vector<ID>& groundedRules, Set<ID>& newDerivableAtoms);

	/** \brief Checks if a literal matches a given pattern using a substitution.
	  * @param literalID Literal to check.
	  * @param patternLiteral Literal to check against.
	  * @param s Set or pairs of variables to be substituted and the values to be inserted; can be incomplete.
	  * @return True if \p literalID after application of \p s unifies with \p patternLiteral, and false otherwise. */
	bool match(ID literalID, ID patternLiteral, Substitution& s);
	/** \brief Checks if an ordinary literal matches a given pattern using a substitution.
	  * @param literalID Ordinary literal to check.
	  * @param patternLiteral Ordinary literal to check against.
	  * @param s Set or pairs of variables to be substituted and the values to be inserted; can be incomplete.
	  * @return True if \p literalID after application of \p s unifies with \p patternLiteral, and false otherwise. */
	bool matchOrdinary(ID literalID, ID patternAtom, Substitution& s);
	/** \brief Checks if a builtin literal matches a given pattern using a substitution.
	  * @param literalID Builtin literal to check.
	  * @param patternLiteral Builtin literal to check against.
	  * @param s Set or pairs of variables to be substituted and the values to be inserted; can be incomplete.
	  * @return True if \p literalID after application of \p s unifies with \p patternLiteral, and false otherwise. */
	bool matchBuiltin(ID literalID, ID patternAtom, Substitution& s);
	/** \brief Computes the index of the next derivable atom which matches against the given literal using a given substitution.
	  * @param literalID Literal to check.
	  * @param s Set or pairs of variables to be substituted and the values to be inserted; can be incomplete.
	  * @param startSearchIndex Index to start search; start from 0 and pass the index previously returned by this method to iterate.
	  * @return Index of the next derivable atom which matches against \p literalID using substitution \p s. */
	int matchNextFromExtension(ID literalID, Substitution& s, int startSearchIndex);
	/** \brief Computes the index of the next derivable ordinary atom which matches against the given literal using a given substitution.
	  * @param literalID Ordinary literal to check.
	  * @param s Set or pairs of variables to be substituted and the values to be inserted; can be incomplete.
	  * @param startSearchIndex Index to start search; start from 0 and pass the index previously returned by this method to iterate.
	  * @return Index of the next derivable ordinary atom which matches against \p literalID using substitution \p s. */
	int matchNextFromExtensionOrdinary(ID literalID, Substitution& s, int startSearchIndex);
	/** \brief Computes the index of the next derivable builtin atom which matches against the given literal using a given substitution.
	  * @param literalID Builtin literal to check.
	  * @param s Set or pairs of variables to be substituted and the values to be inserted; can be incomplete.
	  * @param startSearchIndex Index to start search; start from 0 and pass the index previously returned by this method to iterate.
	  * @return Index of the next derivable builtin atom which matches against \p literalID using substitution \p s. */
	int matchNextFromExtensionBuiltin(ID literalID, Substitution& s, int startSearchIndex);
	/** \brief Computes the index of the next derivable unary builtin atom which matches against the given literal using a given substitution.
	  * @param literalID Unary builtin literal to check.
	  * @param s Set or pairs of variables to be substituted and the values to be inserted; can be incomplete.
	  * @param startSearchIndex Index to start search; start from 0 and pass the index previously returned by this method to iterate.
	  * @return Index of the next derivable unary builtin atom which matches against \p literalID using substitution \p s. */
	int matchNextFromExtensionBuiltinUnary(ID literalID, Substitution& s, int startSearchIndex);
	/** \brief Computes the index of the next derivable binary builtin atom which matches against the given literal using a given substitution.
	  * @param literalID Binary builtin literal to check.
	  * @param s Set or pairs of variables to be substituted and the values to be inserted; can be incomplete.
	  * @param startSearchIndex Index to start search; start from 0 and pass the index previously returned by this method to iterate.
	  * @return Index of the next derivable binary builtin atom which matches against \p literalID using substitution \p s. */
	int matchNextFromExtensionBuiltinBinary(ID literalID, Substitution& s, int startSearchIndex);
	/** \brief Computes the index of the next derivable ternary builtin atom which matches against the given literal using a given substitution.
	  * @param literalID Ternary builtin literal to check.
	  * @param s Set or pairs of variables to be substituted and the values to be inserted; can be incomplete.
	  * @param startSearchIndex Index to start search; start from 0 and pass the index previously returned by this method to iterate.
	  * @return Index of the next derivable ternary builtin atom which matches against \p literalID using substitution \p s. */
	int matchNextFromExtensionBuiltinTernary(ID literalID, Substitution& s, int startSearchIndex);
	/** \brief Backtracks to the previous substitution where search is to be continued.
	  *
	  * Uses the DLV algorithm.
	  * @param ruleID Rule whos instances are currently enumerated.
	  * @param binders Stores for each variable the body literal which currently binds it.
	  * @param currentIndex Index of the next body literal to be instantiated.
	  * @return Index of the body literal to backtrack to. */
	int backtrack(ID ruleID, Binder& binders, int currentIndex);

	/** \brief Makes \p atom permanently true (EDB fact).
	  * @param atom Atom ID. */
	void setToTrue(ID atom);
	/** \brief Is called after one or more atoms became derivable.
	  *
	  * Triggers the instantiation of depending rules.
	  * @param atom Atom to be marked.
	  * @param groundRules Set where new rule instances are to be added.
	  * @param newDerivableAtoms Atoms which recently became derivable. */
	void addDerivableAtom(ID atom, std::vector<ID>& groundRules, Set<ID>& newDerivableAtoms);


	// helper members
	/** \brief Applies a substitution to an atom.
	  * @param s Substitution of variables to terms.
	  * @param atomID Atom to apply \p s to.
	  * @return ID of atom \p atomID after \p s was applied. */
	ID applySubstitutionToAtom(Substitution s, ID atomID);
	/** \brief Applies a substitution to an ordinary atom.
	  * @param s Substitution of variables to terms.
	  * @param atomID Ordinary atom to apply \p s to.
	  * @return ID of atom \p atomID after \p s was applied. */
	ID applySubstitutionToOrdinaryAtom(Substitution s, ID atomID);
	/** \brief Applies a substitution to a bulitin atom.
	  * @param s Substitution of variables to terms.
	  * @param atomID Builtin atom to apply \p s to.
	  * @return ID of atom \p atomID after \p s was applied. */
	ID applySubstitutionToBuiltinAtom(Substitution s, ID atomID);
	/** \brief Returns a string representation of an atom.
	  * @param atomID Atom ID.
	  * @return String representation of \p atomID. */
	std::string atomToString(ID atomID);
	/** \brief Returns a string representation of a rule.
	  * @param ruleID Rule ID.
	  * @return String representation of \p ruleID. */
	std::string ruleToString(ID ruleID);
	/** \brief Extracts the predicate from an atom.
	  * @param atomID Atom ID.
	  * @return Predicate of \p atomID. */
	ID getPredicateOfAtom(ID atomID);
	/** \brief Checks if a rule id ground.
	  * @param ruleID Rule to check.
	  * @return True if \p ruleID is ground and false otherwise. */
	bool isGroundRule(ID ruleID);
	/** \brief Checks if a predicate is fully grounded, i.e., it comes from a lower stratum.
	  * @param pred Predicate ID.
	  * @return True if \p pred is fully grounded. */
	bool isPredicateGrounded(ID pred);
	/** \brief Checks if a predicate is fully solved.
	  *
	  * Completely solved predicates are those such that all ground instances are known to be true or false.
	  * SolvedPredicates is a subset of groundedPredicates.
	  * @param pred Predicate to check.
	  * @return True if \p pred is fully solved. */
	bool isPredicateSolved(ID pred);
	/** \brief Checks if an atom is derivable (in principle, i.e., some rule derives it).
	  * @param atom ID of the atom to check.
	  * @return True if \p atom is derivable and false otherwise. */
	bool isAtomDerivable(ID atom);
	/** \brief Returns the stratum index of a rule.
	  * @param ruleID Rule to check.
	  * @return Stratum of \p ruleID (0-based). */
	int getStratumOfRule(ID ruleID);
	/** \brief Constructs a new atom which does not yet occur in the ground program and stored it in globallyNewAtom. */
	void computeGloballyNewAtom();

	/** \brief Retrieves for a given rule body its binder.
	  * @param body Rule body.
	  * @return Binder, i.e., for each variable the index of a body literal which binds it. */
	Binder getBinderOfRule(std::vector<ID>& body);
	/** \brief Retrieves for a given rule body the literal up to \p litIndex which first binds a variable from \p variables.
	  * @param body Rule body.
	  * @param litIndex Index of a body literal up to which the binder is computed.
	  * @param variables Variables to search for.
	  * @return Index of the literal (before literal \p litIndex) which first binds a variable from \p variables, or -1 if no such literal exists. */
	int getClosestBinder(std::vector<ID>& body, int litIndex, std::set<ID> variables);
	/** \brief Returns for a given literal in a rule all variables which are not bounded before that literal.
	  * @param body Rule body.
	  * @param litIndex Index of a literal in \p body.
	  * @return Set of all variables in literal \p litIndex in \p body which are not bounded by previous literals. */
	std::set<ID> getFreeVars(std::vector<ID>& body, int litIndex);
	/** \brief Returns for a given rule the output variables.
	  *
	  * This is the set of all variables which occur in literals over unsolved predicates.
	  * @param ruleID Rule to check.
	  * @return Set of output variables in \p ruleID. */
	std::set<ID> getOutputVariables(ID ruleID);
	/** \brief Reorders a rule (mainly for optimization purposes).
	  *
	  * Will place positive atoms first and ordinary atoms before builtin atoms.
	  * @param ruleID Rule to reorder.
	  * @return Reordered rule body. */
	std::vector<ID> reorderRuleBody(ID ruleID);
	/** \brief Checks if two builtin atoms depend on each other.
	  * @param bi1 First builtin atom.
	  * @param bi2 Second builtin atom.
	  * @return True if \p bi1 and \p bi2 depend on each other. */
	bool biDependency(ID bi1, ID bi2);

	/** \brief Used in method InternalGrounder::applyIntFunction to specify the order of application of a builtin function. */
	enum AppDir{
		/** \brief X*Y=Z means to assign Z to the value of X*Y. */
		x_op_y_eq_ret,
		/** \brief X*Z=Y means to assign Z to the value of Y/X. */
		x_op_ret_eq_y,
		/** \brief Z*X=Y means to assign Z to the value of X/Y. */
		ret_op_y_eq_x,
	};
	/** \brief Applies a builtin function to two values.
	  * @param ad See InternalGrounder:AppDir.
	  * @param x Integer value 1.
	  * @param y Integer value 2.
	  * @return Result according to InternalGrounder:AppDir. */
	int applyIntFunction(AppDir ad, ID op, int x, int y);
public:
	/** \brief Constructor; will immediately run the grounder!
	  * @param ctx ProgramCtx.
	  * @program p Program to ground.
	  * @param OptLevel Specifies how much to optimize. */
	InternalGrounder(ProgramCtx& ctx, const OrdinaryASPProgram& p, OptLevel = full);

	/** \brief Extracts the ground program.
	  * @return Ground program. */
	const OrdinaryASPProgram& getGroundProgram();
	/** \brief Extracts the nonground program.
	  * @return Nonground program. */
	const OrdinaryASPProgram& getNongroundProgram();
	/** \brief Extracts the ground program as string.
	  * @return Ground program as string. */
	std::string getGroundProgramString();
	/** \brief Extracts the nonground program as string.
	  * @return Nonground program as string. */
	std::string getNongroundProgramString();

	typedef boost::shared_ptr<InternalGrounder> Ptr;
	typedef boost::shared_ptr<const InternalGrounder> ConstPtr;
};

typedef InternalGrounder::Ptr InternalGrounderPtr;
typedef InternalGrounder::ConstPtr InternalGrounderConstPtr;

DLVHEX_NAMESPACE_END

#endif
