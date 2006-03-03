/* -*- C++ -*- */

/**
 * @file   GeneralError.cpp
 * @author Roman Schindlauer
 * @date   Fri Mar  3 11:40:24 CET 2006
 * 
 * @brief  Exception classes.
 * 
 * 
 */

#include <sstream>

#include "dlvhex/GeneralError.h"


SyntaxError::SyntaxError(const std::string file,
                         const unsigned line,
                         const std::string msg)
{
    std::ostringstream err;

    err << "Syntax Error in " << file << ", line "
        << line << ": "
        << msg;

    errorMsg = err.str();
}


FatalError::FatalError(const std::string msg)
    : GeneralError("Fatal: " + msg)
{
}


PluginError::PluginError(std::string msg)
    : GeneralError(msg)
{
}


void
PluginError::setContext(std::string atomname)
{
    errorMsg = "Plugin Error at Atom " + atomname + ": " + errorMsg;
}

