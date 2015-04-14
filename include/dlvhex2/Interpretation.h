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
 * @file   Interpretation.h
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 *
 * @brief  Bitset interpretation using bitmagic library.
 */

#ifndef INTERPRETATION_HPP_INCLUDED__08112010
#define INTERPRETATION_HPP_INCLUDED__08112010

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/fwd.h"
#include "dlvhex2/ModelGenerator.h"
#include "dlvhex2/ID.h"
#include "dlvhex2/Registry.h"
#include <bm/bm.h>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

DLVHEX_NAMESPACE_BEGIN

/**
 * \brief Stores a set of atoms efficiently as a bitset.
 */
class DLVHEX_EXPORT Interpretation:
public InterpretationBase,
public ostream_printable<Interpretation>
{
    // types
    public:
        typedef boost::shared_ptr<Interpretation> Ptr;
        typedef boost::shared_ptr<const Interpretation> ConstPtr;
        typedef bm::bvector<> Storage;
        typedef boost::function<bool (IDAddress)> FilterCallback;
        typedef Storage::enumerator TrueBitIterator;

        // storage
    protected:
        /** \brief Regirstry used to interpret IDs when printing. */
        RegistryPtr registry;
        /** \brief Internal bitset storage. */
        Storage bits;

        // members
    public:
        /** \brief Constructor. */
        inline Interpretation(){};
        /** \brief Constructor.
         * @param registry Registry to use for interpreting IDs.
         */
        Interpretation(RegistryPtr registry);
        /** \brief Destructor. */
        virtual ~Interpretation();
        // TODO: bitset stuff with bitmagic

        /** \brief Go through 1-bits and set to zero if callback returns false.
         * @param callback Callback to use.
         * @return Number of atoms removed from the interpretation.
         */
        virtual unsigned filter(FilterCallback callback);

        /**
         * \brief Prints the interpretation.
         * @param o Stream to print.
         * @param first String to print at the begining of the output.
         * @param sep Atom delimiter.
         * @param last String to print at the end of the output.
         * @return \p o.
         */
        virtual std::ostream& print(std::ostream& o, const char* first, const char* sep, const char* last) const;

        /**
         * \brief Prints the interpretation where atom names are printed without module prefixes (cf. modular HEX).
         * @param o Stream to print.
         * @param first String to print at the begining of the output.
         * @param sep Atom delimiter.
         * @param last String to print at the end of the output.
         * @return \p o.
         */
        virtual std::ostream& printWithoutPrefix(std::ostream& o, const char* first, const char* sep, const char* last) const;

        /**
         * \brief Prints the interpretation where atom ID addresses are printed rather than atom names.
         * @param o Stream to print.
         * @param first String to print at the begining of the output.
         * @param sep Atom delimiter.
         * @param last String to print at the end of the output.
         * @return \p o.
         */
        virtual std::ostream& printAsNumber(std::ostream& o, const char* first, const char* sep, const char* last) const;

        /**
         * \brief Prints the interpretation in curly braces with comma as atom delimiter.
         * @param o Stream to print.
         * @return \p o.
         */
        virtual std::ostream& print(std::ostream& o) const;

        /**
         * \brief Prints the interpretation in curly braces with comma as atom delimiter and with atom names printed without module prefixes (cf. modular HEX).
         * @param o Stream to print.
         * @return \p o.
         */
        virtual std::ostream& printWithoutPrefix(std::ostream& o) const;

        /**
         * \brief Prints the interpretation where atom ID addresses are printed rather than atom names.
         * @param o Stream to print.
         * @return \p o.
         */
        virtual std::ostream& printAsNumber(std::ostream& o) const;

        /**
         * \brief Prints the interpretation as set of facts (each atom follows by a dot).
         * @param o Stream to print.
         * @return \p o.
         */
        virtual std::ostream& printAsFacts(std::ostream& o) const;

        /**
         * \brief Adds another interpretation to this one.
         * @param other Interpretation to add.
         */
        void add(const Interpretation& other);

        /**
         * \brief Bit-ands this interpretation with another interpretation one.
         * @param other Interpretation to bit-add.
         */
        void bit_and(const Interpretation& other);

        /**
         * \brief Removed external atom auxiliaries from the interpretation and returns it as a new interpretation.
         *
         * The original (this) interpretation remains unchanged.
         *
         * @param New interpretation with external auxiliaries removed.
         */
        Ptr getInterpretationWithoutExternalAtomAuxiliaries() const;

        /**
         * \brief Adds an atom to the interpretation.
         * @param id Address of a ground atom ID.
         */
        inline void setFact(IDAddress id)
            { bits.set(id); }

        /**
         * \brief Removes an atom from the interpretation.
         * @param id Address of a ground atom ID.
         */
        inline void clearFact(IDAddress id)
            { bits.clear_bit(id); }

        /**
         * \brief Checks if a ground atom is true in the interpretation.
         * @param id Address of a ground atom ID.
         */
        inline bool getFact(IDAddress id) const
            { return bits.get_bit(id); }

        /**
         * \brief Returns the internal storage of the interpretation.
         * @return Interpretation as bitset (cf. bitmagic).
         */
        const Storage& getStorage() const { return bits; }
        Storage& getStorage() { return bits; }

        /**
         * \brief Returns a pair of a begin and an end operator to iterate through true atoms in the interpretation.
         * @return Pair of a begin and an end operator; dereferencing iterator gives IDAddress.
         */
        std::pair<TrueBitIterator, TrueBitIterator> trueBits() const
            { return std::make_pair(bits.first(), bits.end()); }

        /**
         * \brief Helper function gives ordinary ground atom to true bit.
         * @param addr Address of an ID of an ordinary ground atom.
         * @return Shortcut for registry->ogatoms.getByAddress(addr).
         */
        const OrdinaryAtom& getAtomToBit(IDAddress addr) const
            { return registry->ogatoms.getByAddress(addr); }

        /**
         * \brief Helper function gives ordinary ground atom to true bit.
         * @param it Iterator through addresses of IDs of ordinary ground atoms.
         * @return Shortcut for registry->ogatoms.getByAddress(*it).
         */
        const OrdinaryAtom& getAtomToBit(TrueBitIterator it) const
            { return registry->ogatoms.getByAddress(*it); }

        RegistryPtr getRegistry() const { return registry; }

        // TODO why does this exist? it should not!
        void setRegistry(RegistryPtr registry1) { registry = registry1; }

        /**
         * \brief Checks if the interpretation is empty.
         * @return True if there are no true atoms in the interpretation and false otherwise.
         */
        inline bool isClear() const
            {  return bits.none();  }

        /**
         * \brief Resets the interpretation to the empty one.
         */
        inline void clear()
            {  bits.clear();  }

        /**
         * \brief Compares this interpretation atomwise to another one.
         * @param other Interpretation to compare to.
         * @return True if the interpretations are equal and false otherwise.
         */
        bool operator==(const Interpretation& other) const;

        /**
         * \brief Compares this interpretation atomwise to another one.
         * @param other Interpretation to compare to.
         * @return True if the interpretations are different and false otherwise.
         */
        bool operator!=(const Interpretation& other) const;

        /**
         * \brief Compares this interpretation atomwise to another one and checks if it is a subset of \p other.
         * @param other Interpretation to compare to.
         * @return True if the interpretations are equal and false otherwise.
         */
        bool operator<(const Interpretation& other) const;

};

typedef Interpretation::Ptr InterpretationPtr;
typedef Interpretation::ConstPtr InterpretationConstPtr;

DLVHEX_EXPORT std::size_t hash_value(const Interpretation& intr);

// TODO perhaps we want to have something like this for (manual) joins
// (see https://dlvhex.svn.sourceforge.net/svnroot/dlvhex/dlvhex/branches/dlvhex-depgraph-refactoring@1555)
//void multiplyInterpretations(
//		const std::vector<InterpretationPtr>& i1,
//		const std::vector<InterpretationPtr>& i2,
//		std::vector<InterpretationPtr>& result);

DLVHEX_NAMESPACE_END
#endif                           // INTERPRETATION_HPP_INCLUDED__08112010

// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
