/* -*- C++ -*- */

/**
 * @file Rule.h
 * @author Roman Schindlauer
 * @date Thu Jun 30 12:39:40 2005
 *
 * @brief Rule class.
 *
 */


#ifndef _RULE_H
#define _RULE_H

#include "dlvhex/Atom.h"
#include "dlvhex/Literal.h"


typedef std::vector<AtomPtr> RuleHead_t;


/**
 * @brief Class for representing a rule object.
 */
class Rule : public ProgramObject
{
public:
    
    /**
     * @brief Constructs a rule from a head and a body.
     *
     * Third argument is the file name and fourth the line number this rule
     * appeared in.
     */
    Rule(const RuleHead_t& h,
         const RuleBody_t& b,
         std::string = "",
         unsigned = 0);

    virtual
    ~Rule();

    /**
     * @brief Returns the rule's head.
     */
    const RuleHead_t&
    getHead() const;

    /**
     * @brief Returns the rule's body.
     */
    const RuleBody_t&
    getBody() const;

    /**
     * @brief Returns the program file of this rule.
     */
    std::string
    getFile() const;

    /**
     * @brief Returns the program line number of this rule.
     */
    unsigned
    getLine() const;

    /**
     * @brief returns the rule's external atoms.
     */
    const std::vector<ExternalAtom*>&
    getExternalAtoms() const;

    /**
     * @brief Test for equality.
     *
     * Two rules are equal, if they contain the same atoms in the body and the head.
     */
    bool
    operator== (const Rule& rule2) const;

    virtual std::ostream&
    output(std::ostream& out) const;

protected:

    RuleHead_t head;

    RuleBody_t body;

    std::string programFile;

    unsigned programLine;

private:

    std::vector<ExternalAtom*> externalAtoms;
};


inline std::ostream&
operator<< (std::ostream& out, const Rule& rule)
{
  return rule.output(out);
}


/**
 * @brief A weak constraint is a rule with empty head and weight/level values.
 */
class WeakConstraint : public Rule
{
public:

    /**
     * @brief See constructor of Rule.
     *
     * The third parameter is the weight, the fourth is the level of the weak
     * constraint.
     */
    WeakConstraint(const RuleBody_t& b,
                   Term,
                   Term,
                   std::string = "",
                   unsigned = 0);

    /*
    void
    setHead(const RuleHead_t& h);
    */

    /**
     * @brief Test for equality.
     */
    bool
    operator== (const WeakConstraint&) const;

    std::ostream&
    output(std::ostream& out) const;

private:

    Term weight, level;
};


#endif /* _RULE_H */
