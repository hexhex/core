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
#include <assert.h>

#include "dlvhex/Term.h"
#include "dlvhex/Atom.h"
#include "dlvhex/Interpretation.h"
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
public:

    /**
     * @brief Type of input parameter.
     */
    enum InputType { PREDICATE, CONSTANT };


protected:

    /// Ctor
    PluginAtom()
    {
    }


public:

    /// Dtor
    virtual
    ~PluginAtom()
    { }


    /**
     * @brief Adds an input parameter of type PREDICATE.
     */
    void
    addInputPredicate()
    {
        inputType.push_back(PREDICATE);
    }


    /**
     * @brief Adds an input parameter of type CONSTANT.
     */
    void
    addInputConstant()
    {
        inputType.push_back(CONSTANT);
    }


    /**
     * @brief Specifies the output arity of the external Atom.
     */
    void
    setOutputArity(unsigned arity)
    {
        outputSize = arity;
    }


    /**
     * @brief Returns the output arity of the external atom, which was specified by the
     * plugin author.
     */
    unsigned
    getOutputArity() const
    {
        return outputSize;
    }
    

    /**
     * @brief Returns all tuples of this Atom wrt. the given input.
     *
     * Input to an external atom is an interpretation together with the
     * input parameters, which is ground at call time.
     */
    virtual void
    retrieve(const Interpretation&, const Tuple&, std::vector<Tuple>&) throw(PluginError) = 0;


    /**
     * @brief Boolean query for a specific tuple wrt. the given input.
     */
    virtual bool
    query(const Interpretation&, const Tuple&, Tuple&) throw(PluginError) = 0;


    InputType
    getInputType(unsigned index)
    {
        assert(index < inputType.size());

        return inputType[index];
    }


private:

    unsigned inputSize;

    unsigned outputSize;

    std::vector<InputType> inputType;

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
