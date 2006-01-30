/* -*- C++ -*- */

/**
 * @file   helper.h
 * @author Roman Schindlauer
 * @date   Thu Nov 24 23:59:33 CET 2005
 * 
 * @brief  Declaration of helper functions.
 * 
 * 
 */

#ifndef _HELPER_H
#define _HELPER_H

#include <string>
#include <vector>


/**
 * The helper functions are kept in a namespace in order to keep global stuff
 * together.
 */
namespace helper
{
    /**
     * Explodes a string by a specified delimiter string.
     */
    std::vector<std::string>
    stringExplode(const std::string&, const std::string&);
    

    /**
     * Escapes quotes in a string (e.g. to use it for shell commands).
     */
    void
    escapeQuotes(std::string &str);

    /**
     * Builds the cartesian product of a number of input sets.
     */
//    void
//    multiplySets(const std::vector<GAtomSet>&
//                 GAtomSet&);
}

#endif /* _HELPER_H */
