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


namespace global
{
    extern bool optionNoPredicate;

    extern bool optionSilent;

    extern std::vector<std::string> Messages;
}

#endif // _GLOBALS_H
