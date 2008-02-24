/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
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
 * @file ExternalAtom.h
 * @author Roman Schindlauer
 * @date Wed Sep 21 19:40:57 CEST 2005
 *
 * @brief External Atom class.
 *
 *
 */

#if !defined(_DLVHEX_EXTERNALATOM_H)
#define _DLVHEX_EXTERNALATOM_H

#include "dlvhex/PlatformDefinitions.h"

#include "dlvhex/Atom.h"

DLVHEX_NAMESPACE_BEGIN

/**
 * @brief External atom class.
 */
class DLVHEX_EXPORT ExternalAtom : public Atom
{
public:

    /// @brief copy ctor
    ExternalAtom(const ExternalAtom&);


    /**
     * @brief Constructor.
     *
     * The constructor does not check the parameters - this is done only in
     * setPluginAtom(), where we actually associate the parsed external atom
     * with the atom-object provided by the plugin.
     */
    ExternalAtom(const std::string& name,
                 const Tuple& params,
                 const Tuple& input,
                 const unsigned line);

    /**
     * @brief Returns the auxiliary predicate name.
     */
    const std::string&
    getAuxPredicate() const;

    /**
     * @brief set new auxiliary predicate name.
     */
    void
    setAuxPredicate(const std::string&);

    /**
     * @brief Returns the function name of the external atom.
     *
     * The external atom's function name is equal to its identifier string
     * used in the logic program - without the ampersand-character.
     */
    const std::string&
    getFunctionName() const;

    /**
     * @brief Setup a new function name (and the corresponding
     * replacement and auxiliary names) for this external atom.
     *
     * The external atom's function name is equal to its identifier string
     * used in the logic program - without the ampersand-character.
     */
    void
    setFunctionName(const std::string& name);


    /**
     * @brief Returns the atom's replacement name.
     *
     * The replacement name is a unique (w.r.t. the entire logic program) string,
     * used to replace the external-atoms by ordinary atoms for being processed
     * by an external answer set solver.
     */
    const std::string&
    getReplacementName() const;


    /**
     * @brief Returns 1 if all input arguments are ground, 0 otherwise.
     */
    bool
    pureGroundInput() const;


    /**
     * @brief Returns the tuple of input arguments as they were specified
     * in the logic program.
     */
    const Tuple&
    getInputTerms() const;


    /**
     * @brief Set the tuple of input arguments.
     */
    void
    setInputTerms(const Tuple& ninput);


    /**
     * @brief An External Atom never unifies.
     */
    virtual bool
    unifiesWith(const AtomPtr&) const;

    /**
     * @brief Tests for equality.
     *
     * Two atoms of different class (e.g., ExternalAtom and Atom) are always inequal.
     */
    virtual bool
    operator== (const Atom& atom2) const;


    /**
     * @brief extatom ordering.
     */
    virtual bool
    operator< (const Atom& atom2) const;

    /**
     * @brief accepts a visitor.
     */
    virtual void
    accept(BaseVisitor&);

    /**
     * @return #line
     */
    unsigned
    getLine() const;


private:

    /// private default ctor.
    ExternalAtom();

    /**
     * @brief initializes replacementName and auxPredicate from functionName
     */
    void
    initReplAux();

    /**
     * @brief the input list of an external atom
     */
    Tuple inputList;

    /**
     * Storing the function name here in functionName. Without higher order,
     * it will be accessible through getPredicate from the base class ATOM,
     * but with higher order, the constructor of ATOM throws away the predicate,
     * so we better keep it here, too.
     */
    std::string functionName;

    /**
     * @brief Auxiliary predicate for grounding the input list.
     */
    std::string auxPredicate;

    /**
     * @brief Replacement name to be used for creating an ordinary logic
     * program.
     */
    std::string replacementName;

    /**
     * @brief Filename of the source file where this atom occured
     * (used in error output).
     */
    std::string filename;

    /**
     * @brief Line of the source file where this atom occured (used in
     * error output).
     */
    unsigned line;
};

DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_EXTERNALATOM_H */


// Local Variables:
// mode: C++
// End:
