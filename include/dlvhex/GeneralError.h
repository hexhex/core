/* -*- C++ -*- */

/**
 * @file   GeneralError.h
 * @author Roman Schindlauer
 * @date   Thu Nov 24 23:59:33 CET 2005
 * 
 * @brief  Exception classes.
 * 
 * 
 */

#ifndef _GENERALERROR_H
#define _GENERALERROR_H

#include <string>

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
class SyntaxError : public GeneralError
{
public:
    
    SyntaxError(std::string file,
                unsigned line,
                std::string msg);
};


/**
 * Severe Error, supposed to be followed by program termination.
 */
class FatalError : public GeneralError
{
public:
    
    FatalError(const std::string msg);
};


/**
 * A problem is an error that does not necessarily cause the program
 * to stop. Its message might be dumped as a warning.
 *
class Problem : public GeneralError
{
public:
    
    Problem(const std::string msg)
        : GeneralError(msg)
    {
    }

};
*/


/**
 * A plugin error is thrown by plugins and catched inside dlvhex.
 */
class PluginError : public GeneralError
{
public:

    PluginError(std::string msg);

    void
    setContext(std::string atomname);
};

#endif /* _GENERALERROR_H */
