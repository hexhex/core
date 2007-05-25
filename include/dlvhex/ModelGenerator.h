/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * 
 * This file is part of dlvhex.
 *
 * dlvhex is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * dlvhex is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with dlvhex; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

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
#include "dlvhex/AtomNode.h"

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
//    virtual void
//    initialize(const Program&) = 0;


    /**
     * @brief Computes all answer sets of a given set of nodes.
     *
     *todo: do we need the program here, too? it's already in the initialize method! 
     */
    virtual void
    compute(//const Program&,
            const std::vector<AtomNodePtr>&,
            const AtomSet& I,
            std::vector<AtomSet>& models) = 0;

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


    //virtual 
        void
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
    compute(//const Program&,
            const std::vector<AtomNodePtr>&,
            const AtomSet& I,
            std::vector<AtomSet>& models);

    /**
     * @brief Computes models of a set of nodes by iteration.
     *
     * @todo make a mediator class between the components and the model
     * generators, that converts a node vector to a program and a list of
     * external atoms
     */
    void
    compute(const Program&,
            const AtomSet& I,
            std::vector<AtomSet>& models);


private:


//    const std::vector<ExternalAtom*>&
//    getExternalAtoms() const;

//    std::vector<ExternalAtom*> externalAtoms;

};



/**
 * @brief Concrete Strategy for computing the model by a single solver call.
 *
 * This strategy can be used for any type of component without external atoms
 * (stratified or unstratified).
 */
class OrdinaryModelGenerator : public ModelGenerator
{
public:

    /// Ctor
    OrdinaryModelGenerator();

    void
    initialize(const Program&);

    /**
     * @brief Computes models of a set of nodes by iteration.
     */
    virtual void
    compute(const std::vector<AtomNodePtr>&,
            const AtomSet& I,
            std::vector<AtomSet>& models);
};



/**
 * @brief Concrete Strategy for computing the model by a guess & checl algorithm.
 *
 * If a component is completely unstratified (neither stratified nor e-stratified,
 * we can only use a guess and check algorithm, which guesses all possible values
 * for the external atoms first and filters out those models which are consistent.
 */
class GuessCheckModelGenerator : public ModelGenerator
{
public:

    /// Ctor
    GuessCheckModelGenerator();

    /**
     * @brief Computes models of a set of nodes by iteration.
     */
    virtual void
    compute(const std::vector<AtomNodePtr>&,
            const AtomSet& I,
            std::vector<AtomSet>& models);
};



#endif /* _MODELGENERATOR_H */
