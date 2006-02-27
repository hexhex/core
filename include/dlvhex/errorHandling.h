/* -*- C++ -*- */

/**
 * @file   errorHandling.h
 * @author Roman Schindlauer
 * @date   Thu Nov 24 23:59:33 CET 2005
 * 
 * @brief  Exception classes.
 * 
 * 
 */

#ifndef _ERRORHANDLING_H
#define _ERRORHANDLING_H

#include <string>
#include <sstream>

/**
 * @brief General exception class.
 *
 *todo: derive from anything?
 */
class GeneralError
{
public:

    /// Ctor.
    GeneralError() {}
      

    /**
     * With this constructor, the exception instance is initialized with
     * an error string.
     */
    GeneralError(const std::string msg)
        : errorMsg(msg)
    {
    }
    

    /**
     * Returns error string.
     */
    std::string
    getErrorMsg() const
    {
        return errorMsg;
    }
        
protected:
        
    std::string errorMsg;
};



/**
 * Error caused by malformed input program.
 */
class InputError : public GeneralError
{
public:
    
    InputError(const std::string file,
               const unsigned line,
               const std::string msg)
    {
        std::ostringstream err;

        err << "Input Error in " << file << ": "
            << msg
            << " in line " << line;

        errorMsg = err.str();
    }

};


/**
 * Severe Error, supposed to be followed by program termination.
 */
class FatalError : public GeneralError
{
public:
    
    FatalError(const std::string msg)
        : GeneralError("Fatal: " + msg)
    {
    }

};


/**
 * A problem is an error that does not necessarily cause the program
 * to stop. Its message might be dumped as a warning.
 */
class Problem : public GeneralError
{
public:
    
    Problem(std::string msg)
        : GeneralError(msg)
    {
    }

};


/**
 * A plugin error is thrown by plugins and catched inside dlvhex.
 */
class PluginError : public GeneralError
{
public:

    PluginError(std::string msg)
        : GeneralError(msg)
    {
    }

    void
    setContext(const std::string atomname)
    {
        errorMsg = "Plugin Error at Atom " + atomname + ": " + errorMsg;
    }

};

#endif /* _ERRORHANDLING_H */
