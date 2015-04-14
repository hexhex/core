/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005-2007 Roman Schindlauer
 * Copyright (C) 2006-2015 Thomas Krennwallner
 * Copyright (C) 2009-2015 Peter Schüller
 * Copyright (C) 2011-2015 Christoph Redl
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
 * @file AtomSet.cpp
 * @author Roman Schindlauer
 * @date Tue Feb  7 17:19:18 CET 2006
 *
 * @brief AtomSet class.
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif                           // HAVE_CONFIG_H

#include <vector>
#include <algorithm>

#include "dlvhex2/AtomSet.h"

DLVHEX_NAMESPACE_BEGIN

AtomSet::const_iterator
AtomSet::begin() const
{
    return const_iterator(atoms.begin());
}


AtomSet::const_iterator
AtomSet::end() const
{
    return const_iterator(atoms.end());
}


void
AtomSet::clear()
{
    atoms.clear();
}


bool
AtomSet::empty() const
{
    return atoms.empty();
}


size_t
AtomSet::size() const
{
    return atoms.size();
}


void
AtomSet::insert(const AtomPtr& ap)
{
    /// @todo test if *ap really exists

    atoms.insert(ap);
}


void
AtomSet::insert(const AtomSet& add)
{
    atoms.insert(add.atoms.begin(), add.atoms.end());
}


AtomSet
AtomSet::difference(const AtomSet& as) const
{
    AtomSet res;

    //   std::set_difference(this->atoms.begin(), this->atoms.end(),
    // 		      as.atoms.begin(), as.atoms.end(),
    // 		      std::inserter(res.atoms, res.atoms.begin())
    // 		      );

    for (atomset_t::const_iterator a = atoms.begin();
        a != atoms.end();
    ++a) {
        if (as.atoms.find(*a) == as.atoms.end())
            res.atoms.insert(*a);
    }

    return res;
}


void
AtomSet::matchPredicate(const std::string& pred,
AtomSet& matched) const
{
    /// @todo: stdlib algorithm!
    for (atomset_t::const_iterator a = atoms.begin();
        a != atoms.end();
    a++) {
        if ((*a)->getPredicate() == pred)
            matched.atoms.insert(*a);
    }
}


void
AtomSet::matchAtom(const AtomPtr& atom,
AtomSet& matched) const
{
    /// @todo: stdlib algorithm!
    for (atomset_t::const_iterator a = atoms.begin();
        a != atoms.end();
    a++) {
        if ((*a)->unifiesWith(atom))
            matched.atoms.insert(*a);
    }
}


void
AtomSet::accept(BaseVisitor& v) const
{
    v.visit(this);
}


/**
 * @brief General purpose predicate functor, which returns true iff
 * predicate of g matches pred.
 */
struct PredicateMatches : public std::binary_function<AtomPtr, std::string, bool>
{
    bool
        operator() (const AtomPtr& g, const std::string& pred) const
    {
        return (g->getPredicate() == Term(pred));
    }
};

void
AtomSet::remove(const std::string& pred)
{
    atomset_t::iterator cur = atoms.begin();

    atomset_t::const_iterator last = atoms.end();

    while ((cur = std::find_if(cur, last, std::bind2nd(PredicateMatches(), pred))) != last) {
        atomset_t::iterator tmp = cur++;

        atoms.erase(tmp);
    }
}


void
AtomSet::remove(const std::vector<std::string>& preds)
{
    for (std::vector<std::string>::const_iterator predit = preds.begin();
        predit != preds.end();
    ++predit) {
        remove(*predit);
    }
}


void
AtomSet::keep(const std::vector<std::string>& preds)
{
    atomset_t::iterator cur = atoms.begin();

    atomset_t::const_iterator last = atoms.end();

    //
    // go through all atoms of this set
    //
    while (cur != last) {
        //
        // look if the current atom is in the filter set
        //
        if ((std::find_if(preds.begin(),
            preds.end(),
        std::bind1st(PredicateMatches(), *cur))) == preds.end()) {
            //
            // if not, delete this atom
            //
            atomset_t::iterator tmp = cur++;

            atoms.erase(tmp);
        }
        else {
            cur++;
        }
    }
}


void
AtomSet::keepPos()
{
    atomset_t::iterator cur = atoms.begin();

    atomset_t::const_iterator last = atoms.end();

    //
    // go through all atoms of this set
    //
    while (cur != last) {
        if ((*cur)->isStronglyNegated()) {
            atomset_t::iterator tmp = cur++;

            atoms.erase(tmp);
        }
        else {
            cur++;
        }
    }
}


/**
 * @brief General purpose predicate functor, which returns true iff
 * (*g == a).
 */
struct AtomMatches : public std::binary_function<AtomPtr, Atom, bool>
{
    bool
        operator() (const AtomPtr& g, const Atom& a) const
    {
        return (*g == a);
    }
};

bool
AtomSet::isConsistent() const
{
    atomset_t::iterator cur = atoms.begin();

    atomset_t::const_iterator last = atoms.end();

    //
    // go through all atoms of this set
    //
    while (cur != last) {
        Atom a(**cur);
        a.negate();

        //
        // see if 'cur' occurs negated (i.e., 'a') in the range of 'cur+1' to
        // 'last'
        //
        if (std::find_if(++cur, last, std::bind2nd(AtomMatches(), a)) != last)
            return false;
    }

    return true;
}


bool
AtomSet::operator== (const AtomSet& atomset2) const
{
    return ((this->size() == atomset2.size())
        && std::equal(this->begin(), this->end(), atomset2.begin()));
}


bool
AtomSet::operator!= (const AtomSet& atomset2) const
{
    return !(*this == atomset2);
}


int
AtomSet::operator< (const AtomSet& atomset2) const
{
                                 // <
    if (this->size() < atomset2.size()) {
        return true;
    }
                                 // >
    else if (this->size() > atomset2.size()) {
        return false;
    }
    else {                       // same size, they can still be < or >=
        // find first mismatch
        std::pair<AtomSet::const_iterator, AtomSet::const_iterator> result;
        result = std::mismatch(this->begin(), this->end(), atomset2.begin());

        // no mismatch: ==, otw. check if the found mismatch is < or >=
        return result.first == this->end() ? false : *result.first < *result.second;
    }
}


DLVHEX_NAMESPACE_END



// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
