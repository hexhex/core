/* -*- C++ -*- */

/**
 * @file   PluginInterface.h
 * @author Roman Schindlauer
 * @date   Thu Sep 1 15:36:10 2005
 *
 * @brief Declaration of Classes PluginAtom, PluginRewriter,
 * and PluginInterface.
 */


#ifndef _PLUGININTERFACE_H
#define _PLUGININTERFACE_H

#include <map>
#include <string>
#include <iostream>
#include <assert.h>

#include "dlvhex/Term.h"
#include "dlvhex/Atom.h"
#include "dlvhex/AtomSet.h"
#include "dlvhex/Error.h"

#define PLUGINIMPORTFUNCTION importPlugin
#define PLUGINIMPORTFUNCTIONSTRING "importPlugin"


/**
 * @brief Base class for custom rewriters, which preparses the HEX-program.
 *
 * A plugin can provide a number of plugin atoms as well as a rewriter object.
 * The purpose of a plugin rewriter is to give the plugin author the
 * possibility of creating a custom syntax for her external atoms, which will
 * be converted to the hex-program syntax by the rewriter. When dlvhex is
 * executed, the rewriter of each found plugin is applied to the original input
 * program. The specification of a rewriter is very general: it receives the
 * entire program through a stream and sends back the modified program. Thus, a
 * rewriter is a very powerful tool to add any syntactical sugar to
 * hex-programs - not necessarily related only to the syntax of external atoms.
 * A plugin could even provide only a rewriter, but no external atoms.
 */
class PluginRewriter
{
protected:

    std::istream* input;

    std::ostream* output;

    PluginRewriter(std::istream& i, std::ostream& o)
        : input(&i),
          output(&o)
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
     * @brief Query class for wrapping the input of an external atom call.
     */
    class Query
    {
    public:
        /**
         * @brief Query Constructor.
         *
         * A query has three components:
         * * The input interpretation,
         * * the input arguments, and
         * * the output tuple.
         * The input arguments are the ground terms of the input list. The
         * output tuple corresponds to the atom's output list: If it contains
         * variables, the query will be a functional one for those missing
         * values; if it is nullary or completely ground, the query will be a
         * boolean one. Either way, the answer will contain exactly those tuples
         * that are in the output of the atom's function for the interpretation
         * and the input arguments.
         */
        Query(const AtomSet&,
              const Tuple&,
              const Tuple&);

        /**
         * @brief Returns the input interpretation.
         */
        const AtomSet&
        getInterpretation() const;

        /**
         * @brief Returns the input parameter tuple.
         */
        const Tuple&
        getInputTuple() const;

        /**
         * @brief Return the input pattern.
         */
        const Tuple&
        getPatternTuple() const;


    private:

        const AtomSet& interpretation;

        Tuple input;

        Tuple pattern;
    };


    /**
     * @brief Answer class for wrapping the output of an external atom call.
     */
    class Answer
    {
    public:
        /// Ctor.
        Answer();

        /**
         * @brief Adds an output tuple to the answer object.
         */
        void
        addTuple(const Tuple&);

        /**
         * @brief Adds a set of tuples to the output of the answer object.
         */
        void
        addTuples(const std::vector<Tuple>&);

        /**
         * @brief Replace the output of the answer object.
         */
        void
        setTuples(const std::vector<Tuple>&);

        /**
         * @brief Returns the output tuples of the answer object.
         */
        const std::vector<Tuple>*
        getTuples() const;

    private:

        std::vector<Tuple> output;
    };


    /**
     * @brief Type of input parameter.
     *
     * Currently, two types of input parameters can be specified: PREDICATE and
     * CONSTANT.
     * An input argument of type PREDICATE means that the atom needs those facts
     * of the interpretation whose predicate match the value of this argument.
     * An input argument of type CONSTANT means that only its value is relevent
     * to the external atom, regardless of the interpretation.
     * Specifying the input parameters' types is necessary for reducing the
     * interpretation that will be passed to the external atom as well as
     * improving the dependency information used by the internal
     * evaluation strategies of dlvhex.
     */
    typedef enum { PREDICATE, CONSTANT } InputType;


protected:

    /// Ctor.
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
     *
     * See InputType.
     */
    void
    addInputPredicate();

    /**
     * @brief Adds an input parameter of type CONSTANT.
     *
     * See InputType.
     */
    void
    addInputConstant();

    /**
     * @brief Returns the input arity of the external atom.
     *
     * The input arity follows from the number of specified predicate types (see
     * addInputPredicate and addInputConstant).
     */
    unsigned
    getInputArity() const;

    /**
     * @brief Specifies the output arity of the external Atom.
     */
    void
    setOutputArity(const unsigned arity);

    /**
     * @brief Returns the output arity of the external atom, which was specified by the
     * plugin author.
     */
    unsigned
    getOutputArity() const;

    /**
     * @brief Retrieve answer object according to a query.
     */
    virtual void
    retrieve(const Query&, Answer&) throw (PluginError) = 0;


    /**
     * @brief Retrieve the atom's universe.
     *
     * The universe of an atom is the set of all possible output tuples w.r.t. a
     * specific input tuple.
     */
//    virtual void
//    getUniverse(const Tuple&, std::set<Tuple>&) throw (PluginError) = 0;

    /**
     * @brief Boolean query for a specific tuple wrt. the given input.
     */
//    virtual bool
//    query(const Interpretation&, const Tuple&, Tuple&) throw(PluginError) = 0;


    /**
     * @brief Returns the type of the input argument specified by position
     * (starting with 0).
     */
    InputType
    getInputType(const unsigned index) const;


private:

    /**
     * @brief Number of input arguments.
     */
    unsigned inputSize;

    /**
     * @brief Number of output Terms.
     */
    unsigned outputSize;

    /**
     * @brief Type of each input argument.
     */
    std::vector<InputType> inputType;

};



/**
 * @brief Factory base class for representing plugins and creating necessary objects.
 */
class PluginInterface
{
protected:

    /// Ctor.
    PluginInterface()
    { }

public:
    /// Dtor.
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
     * @brief Fills a mapping from atom names to the plugin's atom objects.
     */
    virtual void
    getAtoms(AtomFunctionMap&)
    { }

    /**
     * @brief Propagates dlvhex program options to the plugin.
     *
     * Each option known to the plugin must be deleted from the vector. dlvhex
     * will exit with an error if unknown options are left in the vector after
     * all plugins have been processed.
     * If the first parameter is true, then help was requested. The plugin must
     * write its help output into the given stream;
     */
    virtual void
    setOptions(bool, std::vector<std::string>&, std::ostream&)
    { }

};


#endif // _PLUGININTERFACE_H
