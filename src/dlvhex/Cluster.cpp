/* -*- C++ -*- */

/**
 * @file Cluster.cpp
 * @author Roman Schindlauer
 * @date Mon Sep 19 13:03:09 CEST 2005
 *
 * @brief Cluster Class.
 *
 *
 */


#include "dlvhex/Cluster.h"

Cluster::Cluster()
{ }


std::vector<Cluster*>
Cluster::getPrecedents() const
{
    return precedents;
}


std::vector<GAtomSet>
Cluster::getResults() const
{
    return results;
}


