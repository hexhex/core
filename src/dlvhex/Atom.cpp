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


Atom::Atom()
    : type(INTERNAL)
{
}


Atom::~Atom()
{
}


Atom::Atom(const Atom& atom2)
    : type(INTERNAL),
      arguments(atom2.arguments)//, isHigherOrder(atom2.isHigherOrder)
{
}


Atom::Atom(const std::string atom)
    : type(INTERNAL)//, isHigherOrder(false)
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
    }
    else
    {
        //
        // can only be propositional
        //
        arguments.push_back(Term(atom));
    }

    //
    // we only accept first-order input!
    //
    assert(!arguments.front().isVariable());
}
	

Atom::Atom(const std::string pred, const Tuple& arg)
    : type(INTERNAL)
{
    arguments.push_back(Term(pred));
    
    for (Tuple::const_iterator t = arg.begin(); t != arg.end(); t++)
    {
        arguments.push_back(*t);
    }
}
	

Atom::Atom(const Tuple& arg)
    : type(INTERNAL)
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
    assert(index <= arguments.size());

    return arguments[index];
}


unsigned
Atom::getArity() const
{
    return arguments.size();
}


bool
Atom::unifiesWith(const Atom& atom2) const
{
    if (getArity() != atom2.getArity())
        return false;
    
    bool ret = true;
    
    for (unsigned i = 0; i < getArity(); i++)
    {
        if (!getArgument(i).unifiesWith(atom2.getArgument(i)))
            ret = false;
    }
    
    return ret;
}



bool
Atom::operator== (const Atom& atom2) const
{
    if (getArity() != atom2.getArity())
        return false;
    
    if (getType() != atom2.getType())
        return false;
    
    bool ret = true;
    
    for (unsigned i = 0; i < getArity(); i++)
    {
        if (getArgument(i) != atom2.getArgument(i))
            ret = false;
    }
    
    return ret;
}



std::ostream&
Atom::print(std::ostream& stream, const bool ho) const
{
    if (ho)
    {
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


Atom*
Atom::clone()
{
    return new Atom(*this);
}


Atom::Type
Atom::getType() const
{
    return type;
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





GAtom::GAtom()
{
}


GAtom::GAtom(const Atom& atom)
    : Atom(atom)
{
    for (Tuple::const_iterator t = arguments.begin(); t != arguments.end(); t++)
    {
        assert(!t->isVariable());
    }
}


GAtom::GAtom(const std::string atom) : Atom(atom)
{
    for (Tuple::const_iterator t = arguments.begin(); t != arguments.end(); t++)
    {
        assert(!t->isVariable());
    }
}


GAtom::GAtom(const std::string pred, const Tuple& arg)
    : Atom(pred, arg)
{
    //std::cout << arg << std::endl;
    for (Tuple::const_iterator t = arguments.begin(); t != arguments.end(); t++)
    {
      //  std::cout << *t << std::endl;
        assert(!t->isVariable());
    }
}


GAtom::GAtom(const Tuple& arg)
    : Atom(arg)
{
    for (Tuple::const_iterator t = arguments.begin(); t != arguments.end(); t++)
        assert(!t->isVariable());
}



bool
GAtom::operator== (const GAtom& gatom2) const
{
    if (getArity() != gatom2.getArity())
        return false;
    
    bool ret = true;
    
    for (unsigned i = 0; i < getArity(); i++)
    {
        if (getArgument(i) != gatom2.getArgument(i))
            ret = false;
    }
    
    return ret;
}



std::ostream&
operator<< (std::ostream& out, const GAtom& groundatom)
{
    return groundatom.print(out, false);
}


std::ostream&
operator<< (std::ostream& out, const GAtomSet& gatomset)
{
    out << "{";

    for (GAtomSet::const_iterator a = gatomset.begin(); a != gatomset.end(); a++)
    {
        if (a != gatomset.begin())
            out << ", ";
        
        out << *a;
    }
        
    return out << "}";    
}


int
GAtom::operator< (const GAtom& gatom2) const
{
    if (getPredicate() < gatom2.getPredicate())
    {
        return true;
    }
    else if (getPredicate() > gatom2.getPredicate())
    {
        return false;
    }

    //
    // predicate symbols are equal, now distinguish between arguments
    //

    if (getArity() < gatom2.getArity())
    {
        return true;
    }

    if (getArity() == gatom2.getArity()) // find first mismatch in the arguments
    {
        Tuple aa1 = getArguments();
        Tuple aa2 = gatom2.getArguments();

        std::pair<Tuple::const_iterator, Tuple::const_iterator> m =
            std::mismatch(aa2.begin(), aa2.end(), aa1.begin());

        if ((unsigned int)(m.first - aa2.begin()) >= aa2.size()) // aa2 = aa1
        {
            return false;
        }

        if (*m.first > *m.second) // first mismatch in aa2 is > aa1
        {
            return true;
        }
    }

    // getArity() > gatom2.getArity()
    // or
    // first mismatch in aa2 is <= aa1
    return false;
}


//
// temp solution:
// implementing GAtomSet functions globally here instead of
// a dedicated class like interpretation
// we will see what turns out to be more practical
//

void
printGAtomSet(const GAtomSet& g,
              std::ostream& stream,
              const bool ho)
{
    stream << "{";

    for (GAtomSet::const_iterator a = g.begin(); a != g.end(); a++)
    {
        if (a != g.begin())
            stream << ", ";

        (*a).print(stream, ho);
    }

    stream << "}";
}
