/* -*- C++ -*- */

/**
 * @file ModelGenerator.h
 * @author Roman Schindlauer
 * @date Tue Sep 13 17:55:11 CEST 2005
 *
 * @brief Abstract strategy class for computing the model of a program from
 * its graph.
 *
 *
 */


#ifndef _MODELGENERATOR_H
#define _MODELGENERATOR_H

#include "dlvhex/Atom.h"
#include "dlvhex/ProgramBuilder.h"

/**
 * @brief Abstract strategy class for computing the model of a program from
 * it's graph.
 */
class ModelGenerator
{
public:
    virtual
    ~ModelGenerator()
    { }

    /**
     * @brief Initialization of a model generator.
     *
     * An instance of a model generator might be used several times to
     * compute the result of a subprogram. This function initializes everything
     * that will not change on subsequent calls of the compute function.
     */
    virtual void
    initialize(const Program& p) = 0;


    /**
     * @brief Computes all answer sets of a given set of nodes.
     *
     * 
     */
    virtual void
    compute(const Program&,
            const GAtomSet& I,
            std::vector<GAtomSet>& models) = 0;

protected:

    /// Ctor.
    ModelGenerator()
    { }

    /**
     * @brief Subprogram to be evaluated.
     */
//    Program program;

    std::string serializedProgram;
};


/**
 * @brief Concrete Strategy for computing the model by iteration.
 */
class FixpointModelGenerator : public ModelGenerator
{
public:

    /// Ctor
    FixpointModelGenerator();


    virtual void
    initialize(const Program&);


    /**
     * Build the textual representation of the program.
     */
    void
    serializeProgram(const Program&);


    /**
     * Return the text program.
     */
    const std::string&
    getSerializedProgram() const;

    /**
     * @brief Computes models of a set of nodes by iteration.
     */
    virtual void
    compute(const Program&,
            const GAtomSet& I,
            std::vector<GAtomSet>& models);

private:


//    const std::vector<ExternalAtom*>&
//    getExternalAtoms() const;

//    std::vector<ExternalAtom*> externalAtoms;

};


#endif /* _MODELGENERATOR_H */
