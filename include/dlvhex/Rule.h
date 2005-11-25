/* -*- C++ -*- */

/**
 * @file Rule.h
 * @author Roman Schindlauer
 * @date Thu Jun 30 12:39:40 2005
 *
 * @brief Rule class.
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
    Rule(const Atom &h, const std::vector<Literal> &b);

    /**
     * Constructs a rule without a body (fact).
     */
//    Rule(const Atom &head);

    /**
     * @brief Returns true if the rule has a body.
     */
    //bool
    //hasBody() const;

    const Atom*
    getHead() const;

    const std::vector<Literal>*
    getBody() const;

private:

    Atom head;
        
    std::vector<Literal> body;
};

//ostream& operator<< (ostream& out, const Rule& rule);

typedef std::vector<Rule> Rules;


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

private:

    Rules rules;

    std::vector<ExternalAtom> externalAtoms;
};


#endif /* _RULE_H */
