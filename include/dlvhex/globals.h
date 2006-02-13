/* -*- C++ -*- */

/**
 * @file   globals.h
 * @author Roman Schindlauer
 * @date   Sat Nov  5 15:26:18 CET 2005
 * 
 * @brief  Global variable declarations.
 * 
 */


#ifndef _GLOBALS_H
#define _GLOBALS_H

#include <string>
#include <vector>


/**
 * @brief Definition of global variables.
 */
namespace global
{
    /**
     * If optionNoPredicate is true, higher-order reasoning is assumed, treating
     * each predicate like an argument and introducing artificial per-arity predicates.
     */
    extern bool optionNoPredicate;

    /**
     * Suppressing any other output than the actual result.
     */
    extern bool optionSilent;

    /**
     * Dumping internal and intermediate computation information.
     */
    extern bool optionVerbose;

    /**
     * Messages returned from external computation sources, which do not necessarily
     * lead to an abortion of the evaluation (i.e., which can be treated as warnings).
     */
    extern std::vector<std::string> Messages;

    /**
     * Filename of the (first, if more than one were specified) logic program
     * dlvhex was called with.
     */
    extern std::string lpfilename;
}

#endif // _GLOBALS_H
