/** @file errorHandling.h
 * Declaration of exception class
 *  
 * @date 2005.07.05
 * @author Roman Schindlauer
 */

#ifndef _ERRORHANDLING_H
#define _ERRORHANDLING_H

#include <string>

class generalError
{
public:

    generalError() {}
        
    generalError(std::string msg) : errorMsg(msg) {}
    
    std::string
    getErrorMsg() const { return errorMsg; }
        
protected:
        
    std::string errorMsg;
};

class fatalError : public generalError
{
public:
    
    fatalError(std::string msg) : generalError(msg) {}

};

class Problem : public generalError
{
public:
    
    Problem(std::string msg) : generalError(msg) {}

};

class PluginError : public generalError
{
public:

    PluginError(std::string msg)
        : generalError(msg)
    {
    }

    void
    setContext(std::string atomname)
    {
        errorMsg = "Plugin Error at Atom " + atomname + ": " + errorMsg;
    }

};

#endif // _ERRORHANDLING_H
