/* -*- C++ -*- */

/**
 * @file   Error.h
 * @author Roman Schindlauer
 * @date   Thu Nov 24 23:59:33 CET 2005
 * 
 * @brief  Exception classes.
 * 
 * 
 */

#ifndef _ERROR_H
#define _ERROR_H

#include <string>
#include <iostream>

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
      
    /// Dtor.
    virtual ~GeneralError() {}

    /**
     * @brief initialize exception with error string.
     */
    GeneralError(const std::string msg)
        : errorMsg(msg)
    {
        //std::cout << "general error: " << msg << std::endl;
    }
    

    /**
     * Returns error string.
     */
    virtual std::string
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
    
    /// Ctor.
    SyntaxError(const std::string msg,
                const unsigned line = 0,
                const std::string file = "");

    /// Dtor.
    virtual ~SyntaxError() {}

    virtual std::string
    getErrorMsg() const;

    unsigned line;

    std::string file;
};


/**
 * Severe Error, supposed to be followed by program termination.
 */
class FatalError : public GeneralError
{
public:
    
    FatalError(const std::string msg);

    /// Dtor.
    virtual ~FatalError() {}

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

    /// Dtor.
    virtual ~PluginError() {}

    void
    setContext(std::string atomname);
};

#endif /* _ERROR_H */
