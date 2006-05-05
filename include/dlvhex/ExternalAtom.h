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


    ExternalAtom(const ExternalAtom&);


    /**
     * @brief Constructor.
     */
    ExternalAtom(const std::string name,
                 const Tuple& params,
                 const Tuple& input,
                 const unsigned line);

    /**
     * @brief Throws an Inputerror Exception.
     *
     * If something was wrong with the external atom in the input hex-program,
     * this function can be used to throw an InputError. It takes care of
     * throwing the InputError with proper parameters.
     */
    void
    throwSourceError(std::string) const;

    /**
     * @brief Returns the auxiliary predicate name.
     */
    std::string
    getAuxPredicate() const;

    /**
     * @brief Returns the base predicate name.
     */
    std::string
    getBasePredicate() const;


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
     * @brief Returns the input Type of the input parameter with index idx.
     *
     * (see also PluginAtom::InputType)
     */
    PluginAtom::InputType
    getInputType(unsigned idx) const;


    /**
     * Returns the arguments of the external atom.
     */
    virtual Tuple
    getArguments() const;

    /**
     * @brief Returns the set of all possible output values w.r.t. to i.
     *
     * The interpretation is needed here because it might be necessary for
     * creating the ground input list from auxiliary predicates in i.
     */
//    void
//    getBase(const AtomSet& i,
//            AtomSet&) const;

    /**
     * @brief Evaluates the external atom w.r.t. to an interpretation.
     *
     * The prediate name of the returned ground atoms will be the replacement
     * name of this atom.  What the evaluation function basically does, is to
     * pass the list of ground input parameters and part of the interpretation
     * to the plugin and let it evaluate its external atom function there. The
     * ground input parameters are either the ones originally specified in the
     * hex-program or produced from auxiliary predicates if they were
     * non-ground. The passed part of the interpretation is determined by those
     * input parameters that are of type PREDICATE.
     */
    void
    evaluate(const AtomSet &i,
             AtomSet& result) const;


    /**
     * @brief An External Atom never unifies.
     */
    virtual bool
    unifiesWith(const AtomPtr) const;


    /**
     * @brief Inherited equality operator.
     *
     * the comparison of an external atom with an ordinary atom must always
     * return false.
     */
    virtual bool
    operator== (const Atom& atom2) const
    {
        return false;
    }

    /**
     * @brief Comparions operator testing the equality of two external atoms.
     */
    bool
    operator== (const ExternalAtom& atom2) const;


    /**
     * @brief Serialize the external atom.
     *
     * The higher order switch inherited from class Atom has no meaning here.
     * An external atom is serialized as &name[a,b,...](X,Y,...).
     */
    virtual std::ostream&
    print(std::ostream&, const bool) const;

    
    /**
     * @brief Clone function. see Atom::clone.
     */
    //Atom*
    //clone();

    unsigned
    getLine() const;

    /*
    typedef struct
    {
        std::string predicate;
        unsigned argument;
    } binding;

    std::vector<binding> inputBinding;
    */

private:

    /**
     * @brief Grounds the input arguments w.r.t. a specified interpretation.
     *
     * If the input list of an external ato in a hex-program is not completely
     * ground, auxiliary predicates are generated, grounding the list from the
     * remaining body atoms. This function creates ground tuples from the
     * auciliary atoms in the interpretation or simply returns the original
     * input list, if it was fully ground.
     */
    void
    groundInputList(const AtomSet&, std::vector<Tuple>&) const;

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
     * @brief Auxiliary predicate for adding the base to the guessing program.
     */
    std::string basePredicate;
    
    /**
     * @brief Consecutive number to build a unique replacement name.
     */
    static unsigned uniqueNumber;

    /**
     * @brief Replacement name to be used for creating an ordinary logic
     * program.
     */
    std::string replacementName;

    /**
     * @brief Filename of the source file where this atom occured.
     */
    std::string filename;

    /**
     * @brief Line of the source file where this atom occured (for error
     * output).
     */
    unsigned line;

    /**
     * @brief Pointer to the PluginAtom object that matches the atom's
     * function name
     */
    PluginAtom* pluginAtom;

};


#endif /* EXTERNALATOM_H */
