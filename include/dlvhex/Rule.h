/* -*- C++ -*- */

/**
 * @file Rule.h
 * @author Roman Schindlauer
 * @date Thu Jun 30 12:39:40 2005
 *
 * @brief Rule and Program class.
 *
 * Momentarily , these rules are 'real' rules, i.e., they have
 * to have a body and a head. Facts are stored somewhere else and
 * disjunctive heads as well as constraints don't exist yet.
 *
 */


#ifndef _RULE_H
#define _RULE_H

#include "dlvhex/Atom.h"
#include "dlvhex/Literal.h"


/**
 * @brief Class for representing a rule object.
 */
class Rule
{
public:
    
    /**
     * @brief Constructs a rule from a head and a body.
     */
    Rule(const std::vector<Atom> &h, const std::vector<Literal> &b);

    /**
     * Constructs a rule without a body (fact).
     */
//    Rule(const Atom &head);

    /**
     * @brief Returns true if the rule has a body.
     */
    //bool
    //hasBody() const;

    const std::vector<Atom>&
    getHead() const;

    const std::vector<Literal>&
    getBody() const;

    /**
     * @brief Test for equality.
     *
     * Two rules are equal, if they contain the same atoms in the body and the head.
     */
    bool
    operator== (const Rule& rule2) const;

private:

    /**
     * @brief The head Atoms are related by disjunction.
     */
    std::vector<Atom> head;
       
    /**
     * @brief The body atoms are related by conjunction.
     */
    std::vector<Literal> body;
};

//
// only for verbose and debugging.
//
std::ostream&
operator<< (std::ostream& out, const Rule& rule);


typedef std::vector<Rule> Rules;


/**
 * @brief Program class.
 *
 * The Program class encapsulates rules and external atoms to represent a subprogram
 */
class Program
{
public:

    Program();

    Program(Rules&);

    void
    setRules(const Rules&);

    void
    addRule(const Rule&);

    void
    setExternalAtoms(std::vector<ExternalAtom>&);

    const Rules&
    getRules() const;

    const std::vector<ExternalAtom>&
    getExternalAtoms() const;

    /**
     * @brief Returns the pointer to an External Atom that matches the specified name
     * and input parameters, or NULL if such an atom does not exist.
     *
     */
    ExternalAtom*
    findExternalAtom(const std::string, const Tuple&);

    /**
     * Only for debugging purposes. The real output functions are implemented
     * by the ProgramBuilder class!
     */
    void
    dump(std::ostream&) const;

private:

    Rules rules;

    std::vector<ExternalAtom> externalAtoms;
};


#endif /* _RULE_H */
