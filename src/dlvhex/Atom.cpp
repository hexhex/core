/* -*- C++ -*- */

/**
 * @file Atom.cpp
 * @author Roman Schindlauer
 * @date Mon Sep  5 17:09:33 CEST 2005
 *
 * @brief Atom and GroundAtom class.
 *
 *
 */


#include <assert.h>

#include "dlvhex/Atom.h"
#include "dlvhex/helper.h"
#include "dlvhex/Error.h"



Atom::Atom()
{
}


Atom::~Atom()
{
}


Atom::Atom(const Atom& atom2)
    : ProgramObject(),
      arguments(atom2.arguments),
      isStrongNegated(atom2.isStrongNegated),
      isAlwaysFO(atom2.isAlwaysFO)
{
}


Atom::Atom(const std::string atom, bool neg)
    : isStrongNegated(neg),
      isAlwaysFO(0)
{
    arguments.clear();
    
    std::string::size_type par;
    
    //
    // not propositional?
    //
    if ((par = atom.find("(", 0)) != std::string::npos)
    {
        std::vector<std::string> termlist = helper::stringExplode(atom.substr(par + 1, atom.length() - par - 2), ",");

        arguments.push_back(Term(atom.substr(0, par)));
        
        for (std::vector<std::string>::const_iterator g = termlist.begin();
             g != termlist.end();
             g++)
        {
            arguments.push_back(Term(*g));
        }

        //
        // the predicate itself must be constant (also in ho-mode, then it will be a
        // constant replacement symbol)
        //
        assert(!arguments.front().isVariable());
    }
    else
    {
        //
        // can only be propositional
        //
        arguments.push_back(Term(atom));

        if (arguments.front().isVariable())
            throw SyntaxError("propositional Atom must be ground");
    }
}
	

Atom::Atom(const std::string pred, const Tuple& arg, bool neg)
    : isStrongNegated(neg),
      isAlwaysFO(0)
{
    arguments.push_back(Term(pred));

    for (Tuple::const_iterator t = arg.begin(); t != arg.end(); t++)
    {
        arguments.push_back(*t);
    }
}
	

Atom::Atom(const Tuple& arg, bool neg)
    : isStrongNegated(neg),
      isAlwaysFO(0)
{
    for (Tuple::const_iterator t = arg.begin(); t != arg.end(); t++)
        arguments.push_back(*t);
}
	

Term
Atom::getPredicate() const
{
    return getArgument(0);
}


Tuple
Atom::getArguments() const
{
    Tuple tl;
    
    for (Tuple::const_iterator t = arguments.begin(); t != arguments.end(); t++)
    {
        if (t != arguments.begin())
          tl.push_back(*t);
    }

    return tl;
}


Term
Atom::getArgument(const unsigned index) const
{
    assert(index < arguments.size());

    return arguments[index];
}


unsigned
Atom::getArity() const
{
    return arguments.size();
}


bool
Atom::isStronglyNegated() const
{
    return isStrongNegated;
}


bool
Atom::unifiesWith(const AtomPtr atom2) const
{
    //
    // atoms only unify with atoms
    //
    if (typeid(*atom2) != typeid(Atom))
        return false;

    if (getArity() != atom2->getArity())
        return false;
    
    bool ret = true;
    
    for (unsigned i = 0; i < getArity(); i++)
    {
        if (!getArgument(i).unifiesWith(atom2->getArgument(i)))
            ret = false;
    }
    
    return ret;
}



bool
Atom::operator== (const Atom& atom2) const
{
    if (getArity() != atom2.getArity())
        return false;
    
    if (typeid(*this) != typeid(atom2))
        return false;

    if (isStrongNegated != atom2.isStrongNegated)
        return false;

    for (unsigned i = 0; i < getArity(); i++)
    {
        if (getArgument(i) != atom2.getArgument(i))
            return false;
    }
    
    return true;
}


bool
Atom::operator!= (const Atom& atom2) const
{
    return !(*this == atom2);
}


void
Atom::setAlwaysFO()
{
    isAlwaysFO = 1;
}


std::ostream&
Atom::print(std::ostream& stream, const bool ho) const
{
    if (ho && !isAlwaysFO)
    {
        if (isStrongNegated)
            stream << "-";

        stream << "a_" << getArity() - 1;
        
        stream << "(";
            
        for (unsigned i = 0; i < getArity(); i++)
        {
            stream << getArgument(i);
            
            if (i < getArity() - 1)
                stream << ",";
        }
        
        stream << ")";
    }
    else
    {
        if (isStrongNegated)
            stream << "-";

        stream << getArgument(0);

        if (getArity() > 1)
        {
            stream << "(";
            
            for (unsigned i = 1; i < getArity(); i++)
            {
                stream << getArgument(i);
                
                if (i < getArity() - 1)
                    stream << ",";
            }
            
            stream << ")";
        }
    }
    return stream;
}



bool
Atom::isGround() const
{
    for (Tuple::const_iterator t = arguments.begin(); t != arguments.end(); t++)
    {
        if (t->isVariable())
                return false;
    }

    return true;
}


std::ostream&
operator<< (std::ostream& out, const Atom& atom)
{
    return atom.print(out, false);
}


int
Atom::operator< (const Atom& atom2) const
{
    if (getPredicate() != atom2.getPredicate())
    {
        return (getPredicate() < atom2.getPredicate());
    }
    /*
    if (getPredicate() < atom2.getPredicate())
    {
        return true;
    }
    else if (getPredicate() > atom2.getPredicate())
    {
        return false;
    }
    */

    if (!isStrongNegated && atom2.isStrongNegated)
    {
        return true;
    }
    else if (isStrongNegated && !atom2.isStrongNegated)
    {
        return false;
    }

    //
    // predicate symbols are equal, now distinguish between arguments
    //

    if (getArity() < atom2.getArity())
    {
        //
        // equal predicates, different arities - can happen for variable
        // predicates!
        //
        if (!this->getPredicate().isVariable() && !atom2.getPredicate().isVariable())
            throw SyntaxError("arity mismatch");

        return true;
    }

    // lexicographically compare on the arguments
    if (getArity() == atom2.getArity())
    {
        Tuple aa1 = getArguments();
        Tuple aa2 = atom2.getArguments();
	
	// lexicographical_compare returns true if the range of
	// elements [first1, last1) is lexicographically less than the
	// range of elements [first2, last2), and false otherwise.
	return std::lexicographical_compare(aa1.begin(), aa1.end(),
					    aa2.begin(), aa2.end());
    }

    // getArity() > atom2.getArity()
    return false;
}



BuiltinPredicate::BuiltinPredicate(Term& t1, Term& t2, std::string& b)
{
    arguments.push_back(Term(b));
    arguments.push_back(t1);
    arguments.push_back(t2);
}


std::ostream&
BuiltinPredicate::print(std::ostream& stream, const bool) const
{
    return stream << this->arguments[1] 
                  << this->arguments[0]
                  << this->arguments[2];
}

