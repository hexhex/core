/* -*- C++ -*- */

/**
 * @file AggregateAtom.cpp
 * @author Roman Schindlauer
 * @date Wed Oct 18 16:04:54 CEST 2006
 *
 * @brief Aggregate Atom class.
 *
 *
 */


#include "dlvhex/AggregateAtom.h"
#include "dlvhex/globals.h"


AggregateAtom::AggregateAtom(const std::string& aggtype,
                             const Tuple& vars,
                             const RuleBody_t& conj)
    : type(aggtype),
      aggVars(vars),
      body(conj),
      cmpLeft(""),
      cmpRight("")
{
    //
    // in higher-rder mode we cannot have aggregates, because then they would
    // almost certainly be recursive, because of our atom-rewriting!
    //
    if (Globals::Instance()->getOption("NoPredicate"))
        throw SyntaxError("Aggregates only allowed in FO-mode (use --firstorder)");

    this->arguments.push_back(Term(""));

//    for (Tuple::const_iterator t = vars.begin(); t != vars.end(); ++t)
//        aggVars.push_back(*t);
}


void
AggregateAtom::setComp(const std::string compLeft,
                       const std::string compRight)
{
    this->cmpLeft = compLeft;
    this->cmpRight = compRight;
}


void
AggregateAtom::setLeftTerm(const Term& left)
{
    this->left = left;

    //
    // we add the comparees as arguments - this is without effect except when we
    // check for safety - then we treat them like normal arguments, such that a
    // rule like:
    //   p(W) :- #min{S : c(S)} = W.
    // is safe. If the W in the body aggregate wouldn't be treated like an
    // atom's argument, this rule would of course be unsafe.
    //
    this->arguments.push_back(left);
}


void
AggregateAtom::setRightTerm(const Term& right)
{
    this->right = right;

    //
    // see comment above
    //
    this->arguments.push_back(right);
}


bool
AggregateAtom::unifiesWith(const AtomPtr atom) const
{
    //
    // an aggregate depends on the atoms in its conjunction
    //
    for (RuleBody_t::const_iterator l = this->body.begin();
            l != this->body.end();
            ++l)
    {
        if (atom->unifiesWith((*l)->getAtom()))
            return true;
    }

    return false;
}


std::ostream&
AggregateAtom::print(std::ostream& stream, const bool ho) const
{
    if (this->cmpLeft != "")
        stream << this->left << " " << this->cmpLeft << " ";

    stream << this->type << "{"
           << this->aggVars << " : ";
    
    if (!this->body.empty())
    {
        for (RuleBody_t::const_iterator l = this->body.begin();
             l != this->body.end() - 1;
             ++l)
        {
            (*l)->print(stream, ho);
            stream << ", ";
        }

        this->body.back()->print(stream, ho);
    }

    stream << "}";
    
    if (this->cmpRight != "")
        stream << " " << this->cmpRight << " " << this->right;

    return stream;
}



