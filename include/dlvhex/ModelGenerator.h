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

#include "dlvhex/Component.h"
#include "dlvhex/Atom.h"

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
     * @brief Computes all answer sets of a given set of components.
     *
     * 
     */
    virtual void
    computeModels(const std::vector<Component> &components,
                  const GAtomSet &I,
                  std::vector<GAtomSet> &models) = 0;

protected:

    /// Ctor
    ModelGenerator()
    { }
};


/**
 * @brief Concrete Strategy for computing the model by iteration.
 */
class FixpointModelGenerator : public ModelGenerator
{
public:

    /// Ctor
    FixpointModelGenerator();

    /**
     * @brief Computes models of a set of components by iteration.
     */
    virtual void
    computeModels(const std::vector<Component> &components,
                  const GAtomSet &I,
                  std::vector<GAtomSet> &models);
};


#endif /* _MODELGENERATOR_H */
