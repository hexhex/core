/* -*- C++ -*- */

/**
 * @file ExternalAtom.h
 * @author Roman Schindlauer
 * @date Wed Sep 21 19:40:57 CEST 2005
 *
 * @brief External Atom class.
 *
 *
 */

#ifndef _EXTERNALATOM_H
#define _EXTERNALATOM_H

#include "dlvhex/Atom.h"
#include "dlvhex/PluginInterface.h"


/**
 * @brief External atom class.
 */
class ExternalAtom : public Atom
{
public:

    /// Ctor.
    ExternalAtom();

    ExternalAtom(const ExternalAtom& extatom);

    /**
     * @brief Constructor.
     */
    ExternalAtom(std::string name,
                 const Tuple &params,
                 const Tuple &input,
                 unsigned line);

    /**
     * @brief Returns the function name of the external atom.
     *
     * The external atom's function name is equal to its identifier string
     * used in the logic program - without the ampersand-character.
     */
    std::string
    getFunctionName() const;

    /**
     * @brief Returns the atom's replacement name.
     *
     * The replacement name is a unique (w.r.t. the entire logic program) string,
     * used to replace the external-atoms by ordinary atoms for being processed
     * by an external answer set solver.
     */
    std::string
    getReplacementName() const;

    /**
     * @brief Returns the tuple of input arguments as they were specified
     * in the logic program.
     */
    void
    getInputTerms(Tuple &it) const;

    /**
     * @brief Evaluates the external atom w.r.t. to an interpretation and a
     * list of input parameters.
     *
     * The input parameter list here must be the ground version of the
     * input arguments specified in the logic program. The prediate name of the
     * returned ground atoms will be the replacement name of this atom.
     */
    void
    evaluate(const Interpretation &i,
             const Tuple& inputParms,
             GAtomSet& result) const;

    /**
     * @brief Serialize the external atom.
     *
     * The higher order switch inherited from class Atom has no meaning here.
     * An external atom is serialized as &name[a,b,...](X,Y,...).
     */
    virtual std::ostream&
    print(std::ostream &stream, const bool ho) const;

    /**
     * @brief Clone function. see Atom::clone.
     */
    Atom*
    clone();

private:

    Tuple inputList;

    /**
     * Storing the function name here in functionName. Without higher order,
     * it will be accessible through getPredicate from the base class ATOM,
     * but with higher order, the constructor of ATOM throws away the predicate,
     * so we better keep it here, too.
     */
    std::string functionName;

    
    static unsigned uniqueNumber;

    std::string replacementName;

    unsigned line;

    /**
     * @brief Pointer to the PluginAtom object that matches the atom's
     * function name
     */
    PluginAtom* pluginAtom;

};


#endif /* EXTERNALATOM_H */
