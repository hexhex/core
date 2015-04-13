/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005-2007 Roman Schindlauer
 * Copyright (C) 2006-2015 Thomas Krennwallner
 * Copyright (C) Peter Schüller
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
 * @file Interpretation.cpp
 * @author Peter Schller
 *
 * @brief Implementation of the (bitset-)interpretation.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif                           // HAVE_CONFIG_H

#include "dlvhex2/Interpretation.h"
#include "dlvhex2/Logger.h"
#include "dlvhex2/Printer.h"
#include <boost/functional/hash.hpp>

DLVHEX_NAMESPACE_BEGIN

std::size_t hash_value(const Interpretation& intr)
{
    std::size_t seed = 0;
    Interpretation::Storage bits= intr.getStorage();
    Interpretation::Storage::enumerator it = bits.first();
    while ( it != bits.end() ) {
        boost::hash_combine(seed, *it);
        it++;
    }
    return seed;
}


Interpretation::Interpretation(RegistryPtr registry):
registry(registry),
bits()
{
}


Interpretation::~Interpretation()
{
}


unsigned Interpretation::filter(FilterCallback cb)
{
    // simply enumerating and clearing bits does not work,
    // as modifying bits invalidates iterators (even the end iterator)
    // so we collect all things to filter out in a separate bitset

    Storage resetThose;

    // go through one-bits
    for(Storage::enumerator it = bits.first();
    it != bits.end(); ++it) {
        if( !cb(*it) ) {
            resetThose.set(*it);
        }
    }

    for(Storage::enumerator it = resetThose.first();
    it != resetThose.end(); ++it) {
        // now clear bits
        clearFact(*it);
    }

    return resetThose.count();
}


std::ostream& Interpretation::print(std::ostream& o) const
{
    return print(o, "{", ",", "}");
}


std::ostream& Interpretation::printWithoutPrefix(std::ostream& o) const
{
    return printWithoutPrefix(o, "{", ",", "}");
}


std::ostream& Interpretation::printAsNumber(std::ostream& o) const
{
    return printAsNumber(o, "{", ",", "}");
}


std::ostream& Interpretation::printAsFacts(std::ostream& o) const
{
    print(o, "", ".", "");
    // make sure the last fact (if any fact exists) gets a dot
    if( bits.first() != bits.end() )
        o << ".";
    return o;
}


std::ostream& Interpretation::print(
std::ostream& o,
const char* first, const char* sep, const char* last) const
{
    Storage::enumerator it = bits.first();
    o << first;
    RawPrinter printer(o, registry);
    if( it != bits.end() ) {
        printer.print(ID(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, *it));
        it++;
        for(; it != bits.end(); ++it) {
            o << sep;
            printer.print(ID(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, *it));
        }
    }
    return o << last;
}


std::ostream& Interpretation::printWithoutPrefix(
std::ostream& o,
const char* first, const char* sep, const char* last) const
{
    Storage::enumerator it = bits.first();
    o << first;
    RawPrinter printer(o, registry);
    if( it != bits.end() ) {
        printer.printWithoutPrefix(ID(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, *it));
        it++;
        for(; it != bits.end(); ++it) {
            o << sep;
            printer.printWithoutPrefix(ID(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, *it));
        }
    }
    return o << last;
}


std::ostream& Interpretation::printAsNumber(
std::ostream& o,
const char* first, const char* sep, const char* last) const
{
    Storage::enumerator it = bits.first();
    o << first;
    if( it != bits.end() ) {
        o << *it;
        it++;
        for(; it != bits.end(); ++it) {
            o << sep;
            o << *it;
        }
    }
    return o << last;
}


void Interpretation::add(const Interpretation& other)
{
    bits |= other.bits;
}


void Interpretation::bit_and(const Interpretation& other)
{
    bits &= other.bits;
}


InterpretationPtr Interpretation::getInterpretationWithoutExternalAtomAuxiliaries() const
{

    // create interpretation without aux bits, but otherwise equivalent to this one
    InterpretationPtr out = InterpretationPtr(new Interpretation(registry));

    bm::bvector<>::enumerator en = getStorage().first();
    bm::bvector<>::enumerator en_end = getStorage().end();
    while (en < en_end) {
        if (!registry->ogatoms.getIDByAddress(*en).isExternalAuxiliary()) {
            out->setFact(*en);
        }
        en++;
    }
    return out;
}


bool Interpretation::operator==(const Interpretation& other) const
{
    return bits == other.bits;
}


bool Interpretation::operator!=(const Interpretation& other) const
{
    return bits != other.bits;
}


bool Interpretation::operator<(const Interpretation& other) const
{
    return bits < other.bits;
}


DLVHEX_NAMESPACE_END
