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

/**
 * @brief General exception class.
 *
 *todo: derive from anything?
 */
class generalError
{
public:

    /// Ctor.
    generalError() {}
      

    /**
     * With this constructor, the exception instance is initialized with
     * an error string.
     */
    generalError(const std::string msg)
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
 * Severe Error, supposed to be followed by program termination.
 */
class fatalError : public generalError
{
public:
    
    fatalError(const std::string msg)
        : generalError(msg)
    {
    }

};


/**
 * A problem is an error that does not necessarily cause the program
 * to stop. Its message might be dumped as a warning.
 */
class Problem : public generalError
{
public:
    
    Problem(std::string msg) : generalError(msg) {}

};


/**
 * A plugin error is thrown by plugins and catched inside dlvhex.
 */
class PluginError : public generalError
{
public:

    PluginError(std::string msg)
        : generalError(msg)
    {
    }

    void
    setContext(const std::string atomname)
    {
        errorMsg = "Plugin Error at Atom " + atomname + ": " + errorMsg;
    }

};

#endif /* _ERRORHANDLING_H */
