/* -*- C++ -*- */

/**
 * @file   Error.cpp
 * @author Roman Schindlauer
 * @date   Fri Mar  3 11:40:24 CET 2006
 * 
 * @brief  Exception classes.
 * 
 * 
 */

#include <sstream>

#include "dlvhex/Error.h"


SyntaxError::SyntaxError(const std::string msg,
                         const unsigned l,
                         const std::string f)
    : GeneralError(msg),
      line(l),
      file(f)
{
}


std::string
SyntaxError::getErrorMsg() const
{
    std::ostringstream err;

    err << "Syntax Error";

    if (!file.empty())
        err << " in " << file;
    
    if (line != 0)
        err << ", line " << line;
    
    err << ": " << errorMsg;

    return err.str();
}


FatalError::FatalError(const std::string msg)
    : GeneralError("Fatal: " + msg)
{
//    std::cout << "general error: " << msg << std::endl;
//    std::cout << "general error: " << errorMsg << std::endl;
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

