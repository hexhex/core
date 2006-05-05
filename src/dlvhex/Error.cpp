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


GeneralError::GeneralError(const std::string msg)
    : std::runtime_error(msg)
{
}

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
    
    err << ": " << this->what();

    return err.str();
}


void
SyntaxError::setLine(unsigned l)
{
    this->line = l;
}


void
SyntaxError::setFile(const std::string& f)
{
    this->file = f;
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
PluginError::setContext(const std::string& c)
{
    context = c;
}


std::string
PluginError::getErrorMsg() const
{
    std::ostringstream err;

    err << "Plugin Error";

    if (!context.empty())
        err << " in " << context;
    
    err << ": " << this->what();

    return err.str();
}
