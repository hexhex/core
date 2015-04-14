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
 * @file   Nogood.hpp
 * @author Christoph Redl <redl@kr.tuwien.ac.at>
 *
 * @brief  Data structures for CDNLSolver.
 */

#ifndef NOGOOD_HPP_INCLUDED__09122011
#define NOGOOD_HPP_INCLUDED__09122011

#include "dlvhex2/ID.h"
#include <vector>
#include <set>
#include <map>
#include <boost/foreach.hpp>
#include "dlvhex2/Printhelpers.h"
#include <boost/foreach.hpp>
#include "dlvhex2/Set.h"
#include "dlvhex2/Registry.h"
#include <boost/unordered_map.hpp>

DLVHEX_NAMESPACE_BEGIN

/**
 * \brief Stores a set of signed literals which cannot be simultanously true.
 *
 * A nogood is used to restrict the search space. To this end, they contain
 * signed literals (that is, positive or negative atoms) which cannot be simultanously
 * true in a compatible set.
 *
 * In dlvhex, they are mainly used to encode conditions which contradict the semantics
 * of external atoms.
 * For instance, the nogood <em>{ p(a), -q(a), -&diff[p,q](a) }</em> encodes that whenever the atom
 * <em>p(a)</em> is true and the atom <em>q(a)</em> is false, then the atom &diff[p,q](a) must be true
 * (i.e. must not be false) since constant <em>a</em> will be in the output of the set difference of <em>p</em> and <em>q</em>.
 *
 * Note: For constructing <em>external source output atoms</em> (such as <em>&diff[p,q](a)</em>), use
 * the method ExternalLearningHelper::getOutputAtom.
 *
 * Nogoods can be added to the reasoner in the method PluginAtom::retrieve.
 * When adding dlvhex IDs to a nogood, they need to be passed through NogoodContainer::createLiteral to strip off
 * property flags (Nogood::insert performs this step automatically).
 */
class DLVHEX_EXPORT Nogood : public Set<ID>, public ostream_printable<Nogood>
{
    private:
        /** \brief Hash value of the nogood for indexing purposes. */
        std::size_t hashValue;

        /** \brief True if the nogood is ground and false otherwise. */
        bool ground;

    public:
        /**
         * \brief Constructs an empty nogood.
         */
        Nogood();

        /**
         * \brief Recomputes the hash after literals were added.
         */
        void recomputeHash();

        /**
         * \brief Returns the hash of the nogood.
         * @return Hash of the nogood.
         */
        size_t getHash();

        /**
         * \brief Overwrites the contents of the nogood with a new one.
         * @param other Nogood to copy to this one.
         * @return Reference to this object.
         */
        const Nogood& operator=(const Nogood& other);

        /**
         * \brief Compares the nogood to another one.
         * @param Nogood to compare to.
         * @return Comparison result (equivalence).
         */
        bool operator==(const Nogood& ng2) const;

        /**
         * \brief Compares the nogood to another one.
         * @param Nogood to compare to.
         * @return Comparison result (antivalence).
         */
        bool operator!=(const Nogood& ng2) const;

        /**
         * \brief Prints the nogood in numeric format.
         * @param o Stream to write to.
         * @return \p o.
         */
        std::ostream& print(std::ostream& o) const;

        /**
         * \brief Prints the nogood in string format.
         * @param reg Registry used to resolve atom IDs.
         * @return String representation of the nogood.
         */
        std::string getStringRepresentation(RegistryPtr reg) const;

        /**
         * \brief Performs resolution on this nogood with another one using a given ground literal.
         * @param ng2 Second nogood.
         * @param groundlitadr Address of a literal which occurs positively in one and negativly in the other nogood.
         * @return Nogood with \p groundlitadr being resolved from this nogood union \p ng2.
         */
        Nogood resolve(const Nogood& ng2, IDAddress groundlitadr);

        /**
         * \brief Performs resolution on this nogood with another one using a given literal ID.
         * @param ng2 Second nogood.
         * @param lit ID of a literal which occurs positively in one and negativly in the other nogood.
         * @return Nogood with \p lit being resolved from this nogood union \p ng2.
         */
        Nogood resolve(const Nogood& ng2, ID lit);

        /**
         * \brief Substitutes variables in this (nonground) nogood.
         * @param reg Registry used to resolve literal IDs.
         * @param subst Variable substitution of pairs (X,Y) to replace variable X by Y.
         */
        void applyVariableSubstitution(RegistryPtr reg, const std::map<ID, ID>& subst);

        /**
         * Renames variables in this nogoods.
         * @param reg Registry used to resolve literal IDs.
         */
        void heuristicNormalization(RegistryPtr reg);

        /**
         * \brief Adds a literal to this nogood.
         *
         * Note: Before adding, the literal is passed through NogoodContainer::createLiteral to translate it into a uniform form (strip off property flags from the ID).
         *
         * @param lit ID of the literal to add.
         */
        void insert(ID lit);
        template <class InputIterator> void insert(InputIterator begin, InputIterator end) {
            for (InputIterator it = begin; it != end; ++it) {
                insert(*it);
            }
        }

        /**
         * \brief Checks groundness of this nogood.
         * @return True if all literals in this nogood are ground and false otherwise.
         */
        bool isGround() const;

        /**
         * \brief Checks if there is a substitution of variables in this nogood such that \p atomID occurs in the substitution and computes the instance of the nogood in that case.
         *
         * @param atomID An atom to be contained in the substitution.
         * @param instance A nogood to contain the substitution in the success case.
         * @return True if \p atomID could be matched to this nogood.
         */
        bool match(RegistryPtr reg, ID atomID, Nogood& instance) const;

    #ifndef NDEBUG
        // saves the nogood as string or loads it back (for debug purposes)
        std::string dbgsave() const;
        void dbgload(std::string str);
    #endif
};

/**
 * \brief Stores a set of nogoods.
 */
class DLVHEX_EXPORT NogoodSet : private ostream_printable<NogoodSet>
{
    private:
        /** \brief Internal nogood vector. */
        std::vector<Nogood> nogoods;
        /** \brief Stores for each nogood how often it was added although it is actually stored only once since this is a set (used for deletion strategies). */
        std::vector<int> addCount;
        /** \brief Indices between 0 and nogoods.size() which are currently unused. */
        Set<int> freeIndices;
        /** Stores for each hash the indices of nogoods with this hash (used in the unlikely case that there is a clash of hashes). */
        boost::unordered_map<size_t, Set<int> > nogoodsWithHash;

    public:
        /** \brief Reorders the nogoods such that there are no free indices in the range 0-(getNogoodCount()-1). */
        void defragment();

        /**
         * \brief Sets this NogoodSet to another one (overwrites the contents).
         * @param other The new NogoodSet.
         * @param Reference to this object.
         */
        const NogoodSet& operator=(const NogoodSet& other);

        /**
         * \brief Adds a new nogood to the set.
         * @param ng The nogood to add.
         * @return Index of the new nogood.
         */
        int addNogood(Nogood ng);

        /**
         * \brief Removes a nogood from the set if contained.
         * @param nogoodIndex The index of the nogood to remove.
         */
        void removeNogood(int nogoodIndex);

        /**
         * \brief Removes a nogood if it is contained.
         * @param ng Nogood to remove.
         */
        void removeNogood(Nogood ng);

        /**
         * \brief Returns a nogood from the set.
         * @param index Index of the nogood to retrieve.
         * @return Reference to the requested nogood.
         */
        Nogood& getNogood(int index);

        /**
         * \brief Returns a nogood from the set.
         * @param index Index of the nogood to retrieve.
         * @return Constant reference to the requested nogood.
         */
        const Nogood& getNogood(int index) const;

        /**
         * \brief Returns the current number of nogoods in the set.
         * @param Number of nogoods in the set.
         */
        int getNogoodCount() const;

        /**
         * \brief Applies a heuristics to remove the lease frequently added nogoods from the set.
         */
        void forgetLeastFrequentlyAdded();

        /**
         * \brief Prints the nogood set in numeric format.
         * @param o The stream to print the output.
         * @return \p o.
         */
        std::ostream& print(std::ostream& o) const;

        /**
         * \brief Prints the nogood set in string format.
         * @param reg The registry used for resolving literal IDs.
         * @return String representation of the nogood set.
         */
        std::string getStringRepresentation(RegistryPtr reg) const;
};

/**
 * \brief Base class for nogood containers.
 */
class DLVHEX_EXPORT NogoodContainer
{
    public:
        /**
         * \brief Adds a nogood to the container.
         * @param ng The nogood to add.
         */
        virtual void addNogood(Nogood ng) = 0;

        /**
         * \brief Transforms a literal into a generic form by stripping off all property flags from the ID (keeping only the NAF flag if present).
         *
         * This method must be called for dlvhex literal IDs before using them in an instance of Nogood (which is automatically done by Nogood::insert).
         *
         * @param lit A positive or negative literal ID.
         * @return Simplified ID.
         */
        static inline ID createLiteral(ID lit) {
            if (lit.isOrdinaryGroundAtom()) {
                return ID(ID::MAINKIND_LITERAL | ID::SUBKIND_ATOM_ORDINARYG | (lit.isNaf() ? ID::NAF_MASK : 0), lit.address);
            }
            else {
                return ID(ID::MAINKIND_LITERAL | ID::SUBKIND_ATOM_ORDINARYN | (lit.isNaf() ? ID::NAF_MASK : 0), lit.address);
            }
        }

        /**
         * \brief Transforms an atom address into a generic ID form by dropping all property flags from the ID (keeping only the NAF flag if present).
         *
         * @param litadr IDAddress of a ground or nonground atom.
         * @param truthValue Defines whether to generate a positive or negative literal from the given IDAddress.
         * @param ground Defines whether the given IDAddress refers to a ground or nonground literal.
         * @param Simplified ID.
         */
        static inline ID createLiteral(IDAddress litadr, bool truthValue = true, bool ground = true) {
            if (ground) {
                return ID(ID::MAINKIND_LITERAL | ID::SUBKIND_ATOM_ORDINARYG | (truthValue ? 0 : ID::NAF_MASK), litadr);
            }
            else {
                return ID(ID::MAINKIND_LITERAL | ID::SUBKIND_ATOM_ORDINARYN | (truthValue ? 0 : ID::NAF_MASK), litadr);
            }
        }

        typedef boost::shared_ptr<NogoodContainer> Ptr;
        typedef boost::shared_ptr<const NogoodContainer> ConstPtr;
};

typedef NogoodContainer::Ptr NogoodContainerPtr;
typedef NogoodContainer::ConstPtr NogoodContainerConstPtr;

/**
 * \brief A NogoodContainer based on NogoodSet.
 */
class DLVHEX_EXPORT SimpleNogoodContainer : public NogoodContainer
{
    private:
        /** \brief Exclusive access to the nogood container. */
        boost::mutex mutex;

        /** \brief Internal nogood storage. */
        NogoodSet ngg;
    public:
        void addNogood(Nogood ng);
        void removeNogood(Nogood ng);
        Nogood& getNogood(int index);
        int getNogoodCount();
        void clear();
        void addAllResolvents(RegistryPtr reg, int maxSize = -1);

        void forgetLeastFrequentlyAdded();
        void defragment();

        typedef boost::shared_ptr<SimpleNogoodContainer> Ptr;
        typedef boost::shared_ptr<const SimpleNogoodContainer> ConstPtr;
};

typedef SimpleNogoodContainer::Ptr SimpleNogoodContainerPtr;
typedef SimpleNogoodContainer::ConstPtr SimpleNogoodContainerConstPtr;

DLVHEX_NAMESPACE_END
#endif

// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
