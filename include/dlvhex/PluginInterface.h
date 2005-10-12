/* -*- C++ -*- */

/**
 * @file   PluginInterface.h
 * @author Roman Schindlauer
 * @date   Thu Sep 1 15:36:10 2005
 * 
 * @brief  Abstract Factory for plugin interface.
 * 
 * 
 */

#ifndef _PLUGININTERFACE_H
#define _PLUGININTERFACE_H

#include <map>
#include <string>
#include <iostream>

#include "dlvhex/Term.h"
#include "dlvhex/Atom.h"
#include "dlvhex/errorHandling.h"

#define PLUGINIMPORTFUNCTION importPlugin
#define PLUGINIMPORTFUNCTIONSTRING "importPlugin"

/**
 * @brief Base class for custom rewriters, which preparse the HEX-program.
 */
class PluginRewriter
{
protected:

    std::istream& input;
    std::ostream& output;

    PluginRewriter(std::istream& i, std::ostream& o)
        : input(i),
          output(o)
    { }

public:
    virtual
    ~PluginRewriter()
    { }

    virtual void
    rewrite() = 0;
};



/**
 * @brief Interface class for external Atoms.
 */
class PluginAtom
{
protected:

    PluginAtom()
    {
    }

public:
    virtual
    ~PluginAtom()
    { }

    /**
    * @brief Type of external atom input.
    */
    typedef std::vector<GAtomSet> FACTSETVECTOR;

    /**
    * @brief Type of external atom boolean query.
    */
    typedef std::vector<Tuple> TUPLEVECTOR;

    /**
     * @brief Returns all tuples of this Atom wrt. the given input.
     */
    virtual void
    retrieve(FACTSETVECTOR&, TUPLEVECTOR&) throw(PluginError) = 0;

    /**
     * @brief Boolean query for a specific tuple wrt. the given input.
     */
    virtual bool
    query(FACTSETVECTOR&, Tuple&) throw(PluginError) = 0;

    /**
     * @brief Specifies the required arities of the atom.
     */
    void
    setArities(unsigned in, unsigned out)
    {
        inputSize = in;
        outputSize = out;
    }

    /**
     * @brief Tests for the required arities.
     */
    bool
    testArities(unsigned in, unsigned out) const
    {
        if ((in != inputSize) || (out != outputSize))
            return false;

        return true;
    }

private:

    unsigned inputSize;

    unsigned outputSize;
};



/**
 * @brief Factory base class for representing plugins and creating necessary objects.
 */
class PluginInterface
{
protected:

    /**
     * Ctor.
     */
    PluginInterface()
    { }

public:
    virtual
    ~PluginInterface()
    { }

    /**
     * @brief Associates atom names with function pointers.
     */
    typedef std::map<std::string, PluginAtom*> AtomFunctionMap;

    /**
     * @brief Rewriting function for custom syntax.
     *
     * By overloading this function, a plugin can implement a custom
     * preparser to rewrite the input logic program.
     */
    virtual PluginRewriter* 
    createRewriter(std::istream&, std::ostream&)
    {
        return 0;
    }

    /**
     * @brief Fills a termlist with the constant universe of the KB
     * specified by a URI.
     */
    virtual void
    getUniverse(std::string&, std::list<Term>&)
    { }

    /**
     * @brief Builds a mapping from atom names to the plugin's atom objects.
     */
    virtual void
    getAtoms(AtomFunctionMap&)
    { }
};


#endif // _PLUGININTERFACE_H
