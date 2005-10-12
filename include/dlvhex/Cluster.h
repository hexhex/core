/* -*- C++ -*- */

/**
 * @file Cluster.h
 * @author Roman Schindlauer
 * @date Mon Sep 19 13:03:09 CEST 2005
 *
 * @brief Cluster Class.
 *
 *
 */


#ifndef _CLUSTER_H
#define _CLUSTER_H

#include "dlvhex/ModelGenerator.h"
#include "dlvhex/Component.h"

/**
 * @brief Component Cluster class.
 *
 * A cluster contains a set of components.
 */
class Cluster
{
public:

    /// Ctor
    Cluster();

    /**
     * @brief Returns vector of preceding clusters.
     */
    std::vector<Cluster*>
    getPrecedents() const;

    /**
     * @brief Returns vector of Atom Sets.
     */
    std::vector<GAtomSet>
    getResults() const;

private:

    /**
     * @brief
     */
    std::vector<Component*> components;

    /**
     * @brief Clusters that this one depends on.
     */
    std::vector<Cluster*> precedents;

    /**
     * @brief Clusters that depend on this one.
     *
     * TODO: do I need this?
     */
    std::vector<Cluster*> subsequents;

    /**
     * @brief Results of this cluster
     */
    std::vector<GAtomSet> results;

    /**
     * @brief Suitable model generator for this cluster.
     *
     * A cluster can contain one strongly component or several weakly
     * connected components. For each different type of cluster, a specific
     * model generator exists.
     */
    ModelGenerator* modelGenerator;
};

#endif /* _CLUSTER_H */
