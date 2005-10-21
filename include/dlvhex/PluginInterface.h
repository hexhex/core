/* -*- C++ -*- */

/**
 * @file   PluginInterface.h
 * @author Roman Schindlauer
 * @date   Thu Sep 1 15:36:10 2005
 */


/**
 * @page plugininterface_page The Plugin Interface
 *
 * @section about What is a Plugin?
 *
 * dlvhex evaluates Answer Set Programs with external atoms. The idea was to
 * provide a mechanism to easily add further external atoms without having to
 * recompile the main application. A \a plugin is a shared library that
 * provides functions to realise custom external atoms. Furthermore, a plugin
 * can supply a rewriter, which may alter the input logic program prior to
 * evaluation.
 *
 * @section function The External Atom Function
 *
 * Theoretically, an external atom is defined to evaluate to true or false, depending on a
 * number of parameters:
 * <ul>
 *   <li>An interpretation (a set of facts)</li>
 *   <li>A list of input constants</li>
 *   <li>A list of output constants</li>
 * </ul>
 * However, it is more intuitive and convenient to think of an external atom
 * not as being boolean, but rather functional: Depending on a given
 * interpretation and a list of input constants, it returns output tuples.
 * For instance, an external atom to import triples from RDF files could look like this:
 *
 * @code
 * &rdf[uri](X,Y,Z)
 * @endcode
 *
 * where \a uri is the name of the predicate which holds one or more uris of
 * RDF-files, and \a X, \a Y, \a Z are variables that represent an RDF-triple.
 * The function behind this atom takes all the constants it finds in predicates
 * named "uri" from the given interpretation and treats them as uris, querying
 * their RDF-triples and returning them.
 * So if we have, e.g.,
 * \f[
 * I=\{{\it uri}(``{\it file1.rdf}"), {\it uri}(``{\it file2.rdf}"), p(a), q(b), ... \}
 * \f]
 * and the external atom from above,
 * rather than testing the atom's truth value for each possible assignment of
 * \a X, \a Y, and \a Z, we simply expect all triples in the atom's output. 
 * 
 * @section howto Writing a Plugin
 *
 * We tried to keep the interface between dlvhex and the plugins as lean as
 * possible. The necessary interactions are:
 * <ul>
 *   <li>Evaluating an atom</li>
 *   <li>Registering the plugin and its atoms</li>
 * </ul>
 * We provide several base classes which the plugin author has to subclass.
 * First of all, there is PluginAtom, which provides the actual function of the
 * external atom. In fact, there are two functions to be implemented: one for
 * retrieving all output tuples for a specific interpretation and a list of
 * input arguments, and one for testing if a specific tuple is in the output of
 * the function (again with respect to an interpretation and input list).
 *
 * As a running example, we will use the aforementioned RDF-atom.
 * The the following header files from dlvhex need to be included:
 *
 * @code
 * #include "dlvhex/PluginInterface.h"
 * #include "dlvhex/errorHandling.h"
 * @endcode
 *
 * If dlvhex was installed correctly, these headers should be accessible.
 *
 * @subsection externalatom The External Atom
 *
 * First, we have to subclass PluginAtom to create our own RDFatom class:
 *
 * @code
 * class RDFatom : public PluginAtom
 * {
 * @endcode
 *
 * The constructor of RDFatom will be used to define some properties of the atom:
 *
 * @code
 * public:
 *     RDFatom()
 *     {
 *         addInputPredicate();
 *         setOutputArity(3);
 *     }
 * @endcode
 *
 * Here, we need to specify the number and types of input parameters. The
 * RDF-atom as we used it above, has a single input parameter, which is the
 * name of the predicate to hold the uris. In this case, we just need a single
 * call to addInputPredicate(). Another possibility would have been to define
 * the single input term as a constant, which represents the uri
 * directly (which means, that then there can only be one uri). In this case,
 * the input interpretation would be irrelevant, since
 * we don't need the extension of any predicate. To define a constant input
 * parameter, we use addInputConstant().  If we wanted to have, say, two input
 * parameters which will represent predicate names and a third one which will
 * be a constant, we would put this sequence of calls in the constructor:
 *
 * @code
 * addInputPredicate();
 * addInputPredicate();
 * addInputConstant();
 * @endcode
 *
 * The call setOutputArity(3) ensures that occurences of this atom with other
 * than three output terms will cause an error at parsing time.
 *
 * The next member function to define is retrieve():
 *
 * @code
 * virtual void
 * retrieve(const Interpretation& in,
 *          const Tuple& parms,
 *          std::vector<Tuple>& out) throw(PluginError)
 * {
 *     rdf(parms[0].getUnquotedString(), out);
 * }
 * @endcode
 *
 * retrieve() always takes an Interpretation and a Tuple (which is defined as a
 * std::vector of Term)) as input and returns the result tuples in a
 * vector.  Here, we assume having defined the single input parameter
 * as a constant, which implies that we don't have to care about the
 * interpretation at all, since the uri is directly encoded as the
 * first (and only) Term of the input tuple. Whatever is done inside the
 * retrieve() function is completely up to the plugin author - she just has to
 * fill the result in the output vector. Furthermore, the author is able to
 * throw an exception of type PluginError which will be catched inside dlvhex,
 * e.g.:
 *
 * @code
 * if (something_went_wrong != 0)
 *     throw PluginError("rdf plugin error");
 * @endcode
 *
 * In addition to retrieve(), we also need to define a boolean function:
 *
 * @code
 * virtual bool
 * query(const Interpretation& in,
 *       const Tuple& parms,
 *       Tuple& out) throw(PluginError)
 * {
 *     testrdf(parms[0].getUnquotedString(), out);
 *  }
 * @endcode
 *
 * This function again needs an interpretation and a list of terms as input,
 * but also a single output tuple (vector of terms). If this tuple belongs
 * to the output of the atom's function, query() returns true,
 * otherwise false.
 *
 * @subsection registering Registering the Atoms
 *
 * So far, we described the implementation of a specific external atom. In
 * order to integrate one or more atoms in the interface framework, the plugin
 * author needs to subclass PluginInterface():
 *
 * @code
 * class RDFplugin : public PluginInterface
 * {
 * @endcode
 *
 * At the current stage of dlvhex, only the function getAtoms() needs to be
 * defined inside this class:
 *
 * @code
 * virtual void
 * getAtoms(AtomFunctionMap& a)
 * {
 *     a["rdf"] = new RDFatom;
 * }
 * @endcode
 *
 * Here, a PluginAtom object is related to a string in a map - as soon as
 * dlvhex encounters an external atom during the evaluation of the program, it
 * finds the corresponding atom object (and hence its retrieve() and query()
 * functions) in this map. Naturally, a plugin can comprise several
 * different PluginAtoms, which are all registered here:
 *
 * @code
 * a["str_cat"] = new strcat_atom;
 * a["str_find"] = new strfind_atom;
 * a["str_erase"] = new strerase_atom;
 * @endcode
 *
 * @subsection importing Importing the Plugin
 *
 * Lastly, we need to import the plugin to dlvhex. this is achieved by the
 * dynamic linking mechanism of shared libraries. The author needs to define
 * this function:
 *
 * @code
 * extern "C"
 * RDFPlugin*
 * PLUGINIMPORTFUNCTION()
 * {
 *     return new RDFPlugin();
 * }
 * @endcode
 *
 * replacing the type RDFPlugin by her own plugin class. For each found library,
 * dlvhex will call this function and receive a pointer to the plugin
 * object, thus being able to call its getAtoms() function.
 *
 *
 * @section compiling Compiling the Plugin
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
