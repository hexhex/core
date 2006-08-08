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
#include <map>


/**
 * @brief Definition of global variables.
 */
class Globals
{
protected:
    Globals();

public:
    /**
     * Singleton instance.
     */
    static Globals*
    Instance();

    /**
     * Return the value of the specified identifier.
     */
    unsigned
    getOption(std::string);

    /**
     * Set an option with specified identifier to a value.
     */
    void 
    setOption(std::string, unsigned);

    /**
     * temporary hack
     */
    std::string maxint;
    
    /**
     * Filename of the (first, if more than one were specified) logic program
     * dlvhex was called with.
     */
    std::string lpfilename;

private:

    /**
     * Singleton instance.
     */
    static Globals* _instance;

    /**
     * @brief Associates option names with values.
     */
    std::map<std::string, unsigned> optionMap;

    /**
     * Messages returned from external computation sources, which do not necessarily
     * lead to an abortion of the evaluation (i.e., which can be treated as warnings).
     */
    //std::vector<std::string> Messages;

};

#endif // _GLOBALS_H
