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
     * @brief
     */
    std::string
    getFunctionName() const;

    /**
     * @brief
     */
    std::string
    getReplacementName() const;

    /**
     * @brief
     */
    void
    getInputTerms(Tuple &it) const;

    /**
     * @brief
     */
    void
    evaluate(const Interpretation &i, GAtomSet &result) const;

    /**
     * @brief
     */
    virtual std::ostream&
    print(std::ostream &stream, const bool ho) const;

    /**
     * @brief
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
    PluginAtom* externalPlugin;

};


#endif /* EXTERNALATOM_H */
