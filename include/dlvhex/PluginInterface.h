/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * 
 * This file is part of dlvhex.
 *
 * dlvhex is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * dlvhex is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with dlvhex; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA.
 */


/**
 * \file   PluginInterface.h
 * \author Roman Schindlauer
 * \date   Thu Sep 1 15:36:10 2005
 *
 * \brief Declaration of Classes PluginAtom, PluginRewriter,
 * and PluginInterface.
 */

/**
 * \defgroup pluginframework The Plugin Framework
 *
 * \section introduction Introduction
 *
 * dlvhex evaluates Answer Set Programs with external atoms. One important
 * design principle was to provide a mechanism to easily add further external
 * atoms without having to recompile the main application. A \em plugin is a
 * shared library that provides functions to realise custom external atoms.
 * Furthermore, a plugin can supply rewriting facilities, which may alter the
 * input logic program prior to evaluation.
 *
 * This Section introduces the principle of plugins and external atoms and gives
 * a short hands-on example. You can also skip this and jump directly to the
 * respective data structures, starting with PluginInterface.
 *
 * \section extatom The External Atom Function
 *
 * Formally, an external atom is defined to evaluate to \e true or \e
 * false, depending on a number of parameters:
 * - An interpretation (a set of facts)
 * - A list of input constants
 * - A list of output constants
 * .
 * However, it is more intuitive and convenient to think of an external atom not
 * as being boolean, but rather functional: Depending on a given interpretation
 * and a list of input constants, it returns output tuples. For instance, the
 * external atom to import triples from RDF files has this form:
 *
 * \code
 * &rdf[uri](X,Y,Z)
 * \endcode
 *
 * where \c uri stands for a string denoting the RDF-source and \c X, \c Y, and
 * \c Z are variables that represent an RDF-triple. The function associated with
 * this atom simply returns all RDF-triples from the specified source.
 * Obviously, in this case the interpretation is ignored.
 *
 * \subsection informationflow Information Flow
 *
 * The interface that is used by dlvhex to access a plugin follows very closely
 * these semantics. For each atom, a retrieval function has to be implemented,
 * which receives a query-object and has to return an an answer-object. The
 * query-object carries the input interpretation as well as the ground input
 * parameters of the external atom call, while the answer object is a container
 * for the output tuples of the external atom's function.
 *
 * \subsection inputtypes Types of Input Parameters
 *
 * Theoretically, it is completely up to the atom function how to process the
 * interpretation together with the input constants, which are basically only
 * names. In practice however, only parts of the interpretation might be needed
 * (if at all). Considering this as well as for efficiency reasons, we created two
 * categories of input parameters:
 * - the Constant parameter and
 * - the Predicate parameter.
 * .
 * A parameter of type "Constant" is not related to the interpretation at all,
 * like in the previous example of the RDF-atom. A parameter is of type
 * ``Predicate'' means that all facts in the interpretation with this predicate
 * are necessary for the atom. Let's assume, we have an external atom that
 * calculates the overall price of a number of books given by their ISBN number:
 *
 * \code
 * &overallbookprice[isbn](X)
 * \endcode
 *
 * The single input parameter of this atom would be of type "Predicate", meaning
 * that not the constant itself is necessary for the atom's function, but the part
 * of the interpretation with this predicate.  So if we have, e.g.,
 * \f[
 * I=\{{\rm isbn}({\rm 0{-}19{-}824183{-}6}), {\rm isbn}({\rm 0{-}201{-}99954{-}4}), p(a), q(b),\ldots\}
 * \f]
 * the atom's function will be called with a "reduced" interpretation:
 * \f[
 * I=\{{\rm isbn}({\rm 0{-}19{-}824183{-}6}), {\rm isbn}({\rm 0{-}201{-}99954{-}4})\}
 * \f]
 * Specifying the type of input parameters not only helps to single out the
 * relevant part of the interpretation, but also supports dlvhex in calculating
 * the dependencies within a HEX-program.
 *
 * \section writingplugin Writing a Plugin
 * We wanted to keep the interface between dlvhex and the plugins as lean as
 * possible. The necessary interactions are:
 * - Registering the plugin and its atoms
 * - Evaluating an atom
 * .
 * For both tasks, we provide a base class which the plugin author has to
 * subclass.  As a running example, we will use the aforementioned RDF-atom. To
 * begin with, the following header files from dlvhex need to be included:
 *
 * \code
 *     #include "dlvhex/PluginInterface.h"
 *     #include "dlvhex/Error.h"
 * \endcode
 *
 * If dlvhex was installed correctly, these headers should be available.
 * 
 * \subsection extatom The External Atom
 * First, we have to subclass PluginAtom to create our own \b RDFatom class:
 * 
 * \code
 *     class RDFatom : public PluginAtom
 *     {
 * \endcode
 * 
 * The constructor of \b RDFatom will be used to define some properties of the atom:
 *
 * \code
 *     public:
 *         RDFatom()
 *         {
 *             addInputPredicate();
 *             setOutputArity(3);
 *         }
 * \endcode
 * 
 * Here, we need to specify the number and types of input parameters. The RDF-atom
 * as we used it above, has a single input parameter, which is the name of the
 * predicate to hold the uris. In this case, we just need a single call to
 * PluginAtom::addInputPredicate(). Another possibility would have been to define the
 * single input term as a constant, which represents the uri directly (which
 * means, that then there can only be one uri). In this case, the input
 * interpretation would be irrelevant, since we don't need the extension of any
 * predicate. To define a constant input parameter, we use PluginAtom::addInputConstant().
 * If we wanted to have, say, two input parameters which will represent predicate
 * names and a third one which will be a constant, we would put this sequence of
 * calls in the constructor:
 *
 * \code
 *             addInputPredicate();
 *             addInputPredicate();
 *             addInputConstant();
 * \endcode
 * 
 * The call \c setOutputArity(3) ensures that occurences of this atom with other than
 * three output terms will cause an error at parsing time.
 * 
 * The only member function of the atom class that needs to be defined is retrieve:
 *
 * \code
 *         virtual void
 *         retrieve(const Query& query, Answer& answer) throw(PluginError)
 *         {
 * \endcode
 * 
 * retrieve() always takes a query object as input and returns the result tuples in an
 * answer object.
 * The input parameters at call time are accessed by Query::getInputTuple():
 * 
 * \code
 *             Tuple parms = query.getInputTuple();
 * \endcode
 *
 * The interpretation is retrieved by Query::getInterpretation():
 *
 * \code
 *             AtomSet i = query.getInterpretation();
 * \endcode
 *
 * For more information about the datatypes of dlvhex, please refer to the
 * documentation of the classes Term, Atom, AtomSet, etc.
 * 
 * At this point, the plugin author will implement the actual function of the
 * external atom, either within this class or by calling other functions.
 * Furthermore, the author is able to throw an exception of type \b PluginError
 * which will be catched inside dlvhex, e.g.:
 * \code
 *             if (something_went_wrong != 0)
 *                 throw PluginError("rdf plugin error");
 * \endcode
 * 
 * Before returning from the retrieve-function, the answer-object needs to be prepared:
 * 
 * \code
 *             std::vector<Tuple> out;
 * 
 *             // fill the answer object...
 *                 
 *             answer.addTuples(out);
 *         }
 * \endcode
 *
 * \subsection registeratoms Registering the Atoms
 *
 * So far, we described the implementation of a specific external atom. In order
 * to integrate one or more atoms in the interface framework, the plugin author
 * needs to subclass PluginInterface:
 *
 * \code
 *     class RDFplugin : public PluginInterface
 *     {
 *     public:
 * \endcode
 *
 * At the current stage of dlvhex, only the function PluginInterface::getAtoms() needs to be
 * defined inside this class:
 * 
 * \code
 *         virtual void
 *         getAtoms(AtomFunctionMap& a)
 *         {
 *             boost::shared_ptr<PluginAtom> rdf(new RDFatom);
 *             a["rdf"] = rdf;
 *         }
 * \endcode
 * 
 * Here, a Plugin-Atom object is related to a string in a map - as soon as
 * dlvhex encounters an external atom (in this case: \c &rdf) during the
 * evaluation of the program, it finds the corresponding atom object (and hence
 * its PluginAtom::retrieve() function) in this map. Naturally, a plugin can
 * comprise several different Plugin-Atoms, which are all registered here:
 *
 * \code
 *             boost::shared_ptr<PluginAtom> str_cat(new strcat_atom);
 *             boost::shared_ptr<PluginAtom> str_find(new strfind_atom);
 *             boost::shared_ptr<PluginAtom> str_erase(new strerase_atom);
 *             a["str_cat"] = str_cat;
 *             a["str_find"] = str_find;
 *             a["str_erase"] = str_erase;
 * \endcode
 * 
 * \subsection importing Importing the Plugin
 *
 * To make the plugin available, we need to instantiate it. The simplest way to do
 * this is to define a global variable:
 *
 * \code
 *     RDFPlugin theRDFPlugin;
 * \endcode
 * 
 * Eventually, we need to import the plugin to dlvhex. this is achieved by the dynamic
 * linking mechanism of shared libraries. The author needs to define this
 * function,
 * 
 * \code
 *     extern "C" RDFPlugin*
 *     PLUGINIMPORTFUNCTION()
 *     {
 *         theRDFPlugin.setPluginName(PACKAGE_TARNAME);
 *         theRDFPlugin.setVersion(RDFPLUGIN_MAJOR,
 *                                 RDFPLUGIN_MINOR,
 *                                 RDFPLUGIN_MICRO);
 *         return new RDFPlugin();
 *     }
 * \endcode
 * 
 * replacing the type RDFPlugin by her own plugin class. For each found library,
 * dlvhex will call this function and receive a pointer to the plugin object, thus
 * being able to call its PluginInterface::getAtoms() function.
 * 
 * The macros used here for setting the version number have their origin in \c
 * configure.ac, which is needed to produce the configure-script (see
 * Section \ref compiling). Of course they can also be hardcoded here, but the
 * method with the macros from configure.ac is certainly more practical!
 * 
 * \section modifying Modifying the input program
 * 
 * dlvhex provides three interfaces for plugins to modify the program before it
 * will be evaluated. We will present each of them in the following.
 *
 * \subsection converter The Converter
 *
 * The converter can be used to, you guessed it, convert the input program
 * before it is processed by dlvhex itself. Thus, a plugin can provide special
 * syntax that deviates from the syntax of HEX-programs and ensure by the
 * rewriter to pass a well-formed HEX-program on to dlvhex. For instance, the
 * plugin for interfacing description logic knowledge bases allows for
 * dl-programs as input, which permit a more intuitive syntax for this type of
 * external atoms. The converter of this plugin transforms the dl-atoms to
 * external atoms and might add some auxiliary rules to the program. Another
 * example of using this facility is the SPARQL-plugin, which converts a SPARQL
 * query into a HEX-program.
 * 
 * There is no specific order for calling the converters, if more than one is
 * provided by existing plugins. So either the converter of a plugin is tolerant
 * enough to leave things untouched if the input is not recognized, or the
 * plugin uses the option-handling facility to be activated manually when
 * invoking dlvhex, thus leaving the plugin unused if not explicitly desired by
 * the user.
 * 
 * A converter must be subclassed from PluginConverter:
 *
 * \code
 *     class MyConverter : public PluginConverter
 *     {
 *     public:
 *
 *         MyConverter()
 *         {
 *         }
 * \endcode
 * 
 * The following function does the actual rewriting and will be called by dlvhex.
 * The input program is read from i. The output must be passed on to the
 * stream o, either the original input stream or the result of a conversion.
 *
 * \code
 *         virtual void
 *         MyConverter::convert(std::istream& i, std::ostream& o)
 *         { 
 *             // do the rewriting, maybe throw a PluginError
 *         }
 *     }
 * \endcode
 * 
 * For supplying a converter to dlvhex, the following function needs to be defined
 * in the class derived from PluginInterface (like \b RDFPlugin above):
 * 
 * \code
 *         virtual PluginConverter*
 *         createConverter()
 *         {
 *             converter = new MyConverter();
 *             return converter;
 *         }
 * \endcode
 * 
 * Of course, this also requires a pointer \b converter to be defined in your
 * plugin class derived from PluginInterface:
 *
 * \code
 *         MyConverter* converter;
 * \endcode
 * 
 * It might be more elegant to define the converter already in the constructor of
 * the plugin:
 *
 * \code
 *         SomePlugin()
 *             : converter(new MyConverter())
 *         { }
 * \endcode
 * 
 * Then, PluginInterface::createConverter only needs to return the object:
 * 
 * \code
 *         virtual PluginConverter*
 *         createConverter()
 *         {
 *             return converter;
 *         }
 * \endcode
 * 
 * The object would be deleted by the
 * plugin's destructor:
 * 
 * \code
 *         ~SomePlugin()
 *         {
 *             delete converter;
 *         }  
 * \endcode
 * 
 * To learn more about how to write the actual conversion routine, refer to the
 * sources of the DL-plugin, which uses a bison/flex parser.
 *
 * \subsection rewriter The Rewriter
 *
 * The purpose of a rewriter is to give the plugin author the
 * possibility of creating a custom syntax for her external atoms, which will
 * be rewritten to the hex-program syntax by the rewriter. When dlvhex is
 * executed, the rewriter of each found plugin is applied to the input
 * program. The rewriters are called after the converters and receive an already
 * parsed program, represented by a Program object and an AtomSet (which
 * contains the facts of the program).
 *
 * Subclassing from PluginRewriter works just as with PluginConverter. The
 * rewriter is passed to the plugin interface by defining the following function
 * in the subclassed PluginInterface (as above for the converter):
 *
 * \code
 *         virtual PluginRewriter*
 *         createRewriter()
 *         {
 *             rewriter = new MyRewriter();
 *             return rewriter;
 *         }
 * \endcode
 *
 * Again, this presumes that you have a member PluginRewriter* rewriter in your
 * interface class.
 *
 * The rewriting is not carried out on streams, but on a Program object, since
 * at that stage, the program is already parsed into proper datastructures.
 * Also the set of initial
 * facts, the EDB, is passed to the rewriter and can be considered/altered.
 *
 * \code
 *         virtual void
 *         rewrite(Program& idb, AtomSet& edb)
 *         {
 *             // examine and alter the rules (idb) / facts (edb)
 *         }
 * \endcode
 *
 * For more information, see the documentation of the classes Program and
 * AtomSet.
 * 
 * \subsection optimizer The Optimizer
 *
 * TBD.
 *
 * \section compiling Building the Plugin
 * 
 * We provide a toolkit for building plugins based on the GNU
 * Autotools (http://sourceware.org/autobook/)
 * environment. This means that you will need the respective tools installed on
 * your system - which is usually no problem, since they are provided as packages
 * by all major linux distributions. For instance in Debian, you need the packages
 * libtool, automake1.9, and autoconf.
 * 
 * After downloading and extracting the toolkit skeleton, the user can
 * customize the top-level \c configure.ac to her own needs regarding
 * dependencies on other packages. This file contains invocations of
 * the Autoconf macros that test the system features your package needs or can use.
 * The only source-directory in this template is
 * \c src, where \c Makefile.am sets the name of the library and its
 * sourcefiles. When adding further source-subdirectories (like \c include), the
 * user has to take care of referencing them in the top-level \c Makefile.am.
 * Calling \c ./bootstrap in the top-level directory creates the configure file.
 * Subsequently calling \c ./configure and \c make install installs the
 * shared library. If no further arguments are given to \c configure, the plugin
 * will be installed into the system-wide plugin-directory (specified by the
 * package configuration file \c lib/pkgconfig/dlvhex.pc of the devel-package).
 * To install the plugin into \c ~/.dlvhex/plugins, run <tt>./configure
 * --enable-userinstall</tt>. However, dlvhex itself provides a command line switch to
 * provide a further directory at runtime where to search for plugin libraries.
 */


#if !defined(_DLVHEX_PLUGININTERFACE_H)
#define _DLVHEX_PLUGININTERFACE_H

#include "dlvhex/PlatformDefinitions.h"

#include "dlvhex/Term.h"
#include "dlvhex/Atom.h"
#include "dlvhex/AtomSet.h"
#include "dlvhex/Error.h"

#include <map>
#include <string>
#include <iosfwd>

#include <boost/shared_ptr.hpp>


#define PLUGINIMPORTFUNCTION importPlugin
#define PLUGINIMPORTFUNCTIONSTRING "importPlugin"


DLVHEX_NAMESPACE_BEGIN

// forward declarations
class Program;
class NodeGraph;
class OutputBuilder;
class ProgramCtx;


/**
 * \brief Converter class.
 *
 * A converter can be seen as a preprocessor, which receives the raw input
 * program. A converter can either decide to parse the program and return a
 * well-formed HEX-program or simply leave the input untouched.
 */
class DLVHEX_EXPORT PluginConverter
{
 protected:

  /**
   * Constructor.
   */
  PluginConverter()
  { }

 public:

  /**
   * Destructor.
   */
  virtual
  ~PluginConverter()
  { }

  /**
   * Conversion function.
   *
   * The input program is read from i. The output must be passed on to the
   * stream o, either the original input stream or the result of a conversion.
   */
  virtual void
  convert(std::istream& i, std::ostream& o) = 0;
};


/**
 * \brief Rewriter class.
 *
 * A plugin can provide a rewriter object.
 * The purpose of a plugin rewriter is to give the plugin author the
 * possibility of creating a custom syntax for her external atoms, which will
 * be rewritten to the hex-program syntax by the rewriter. When dlvhex is
 * executed, the rewriter of each found plugin is applied to the input
 * program. The rewriters are called after the converters and receive an already
 * parsed program, represented by a Program object and an AtomSet (which
 * contains the facts of the program).
 */
class DLVHEX_EXPORT PluginRewriter
{
 protected:
  
  /**
   * Constructor.
   */
  PluginRewriter()
  { }

 public:

  /**
   * Destructor.
   */
  virtual
  ~PluginRewriter()
  { }

  /**
   * Rewriting funcition.
   *
   * The rewriting is applied to a Program object. Also the set of initial
   * facts, the EDB, is passed to the rewriter and can be considered/altered.
   */
  virtual void
  rewrite(Program&, AtomSet&) = 0;
};


/**
 * Optimizer class.
 *
 * \todo doc
 */
class DLVHEX_EXPORT PluginOptimizer
{
 protected:

  /**
   * Constructor.
   */
  PluginOptimizer()
  { }

 public:

  /**
   * Destructor.
   */
  virtual
  ~PluginOptimizer()
  { }

  /**
   * Optimizing function.
   */
  virtual void
  optimize(NodeGraph&, AtomSet&) = 0;

};




/**
 * \brief Interface class for external Atoms.
 *
 * \ingroup pluginframework
 *
 * An external atom is represented by a subclassed PluginAtom. The actual
 * evaluation function of the atom is realized in the PluginAtom::retrieve()
 * method. In the constructor, the user must specify the types of input terms
 * and the output arity of the atom.
 */
class DLVHEX_EXPORT PluginAtom
{
public:

    /**
     * \brief Query class for wrapping the input of an external atom call.
     */
    class DLVHEX_EXPORT Query
    {
    public:
        /**
         * \brief Query Constructor.
         *
         * A query has three components:
         * - The input interpretation,
         * - the input arguments, and
         * - the output tuple.
		 * .
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
         * \brief Returns the input interpretation.
		 *
		 * An external atom is evaluated w.r.t. the 
		 * The interpretation is one part of the input to an external atom,
		 * which is evaluated
         */
        const AtomSet&
        getInterpretation() const;

        /**
         * \brief Returns the input parameter tuple.
         */
        const Tuple&
        getInputTuple() const;

        /**
         * \brief Return the input pattern.
         */
        const Tuple&
        getPatternTuple() const;


    private:

        const AtomSet& interpretation;

        Tuple input;

        Tuple pattern;
    };


    /**
     * \brief Answer class for wrapping the output of an external atom call.
     */
    class DLVHEX_EXPORT Answer
    {
    public:
        /// Ctor.
        Answer();

        /**
         * \brief Adds an output tuple to the answer object.
         */
        void
        addTuple(const Tuple&);

        /**
         * \brief Adds a set of tuples to the output of the answer object.
         */
        void
        addTuples(const std::vector<Tuple>&);

        /**
         * \brief Replace the output of the answer object.
         */
        void
        setTuples(const std::vector<Tuple>&);

        /**
         * \brief Returns the output tuples of the answer object.
         */
        boost::shared_ptr<std::vector<Tuple> >
        getTuples() const;

    private:

        boost::shared_ptr<std::vector<Tuple> > output;
    };


    /**
     * \brief Type of input parameter.
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
    typedef enum { PREDICATE, CONSTANT, TUPLE } InputType;


protected:

    /// Ctor.
    PluginAtom()
    { }


public:

    /// Dtor
    virtual
    ~PluginAtom()
    { }


    /**
     * \brief Adds an input parameter of type PREDICATE.
     *
     * See InputType.
     */
    void
    addInputPredicate();

    /**
     * \brief Adds an input parameter of type CONSTANT.
     *
     * See InputType.
     */
    void
    addInputConstant();

    /**
     * \brief Adds an input parameter of type TUPLE.
     *
     * See InputType.
     */
    void
    addInputTuple();

    /**
     * \brief Checks the input arity of the external atom against the specified
	 * one.
     *
     * The input arity follows from the number of specified predicate types (see
     * addInputPredicate and addInputConstant).
     */
    bool
    checkInputArity(unsigned arity) const;

    /**
     * \brief Specifies the output arity of the external Atom.
     */
    void
    setOutputArity(unsigned arity);

    /**
	 * \brief Checks whether the output arity of the external atom is compatible
	 * with the specified one.
     */
    bool
    checkOutputArity(unsigned arity) const;

    /**
     * \brief Retrieve answer object according to a query.
     */
    virtual void
    retrieve(const Query&, Answer&) throw (PluginError) = 0;


    /**
     * \brief Returns the type of the input argument specified by position
     * (starting with 0).
     */
    InputType
    getInputType(unsigned index) const;

    /**
     * @return inputType
     */
    const std::vector<InputType>&
    getInputTypes() const;

private:

    /**
     * \brief Number of input arguments.
     */
    unsigned inputSize;

    /**
     * \brief Number of output Terms.
     */
    unsigned outputSize;

    /**
     * \brief Type of each input argument.
     */
    std::vector<InputType> inputType;

};



/**
 * \brief Factory base class for representing plugins and creating necessary objects.
 *
 * \ingroup pluginframework
 *
 * \section intro Overview
 *
 * The PluginInterface class can be seen as a wrapper for all user-defined
 * features of a plugin, which are:
 * - announce external atoms
 * - supply rewriting facilities
 * - provide an optimizer
 * - offer additional command line options
 * .
 *
 * The user has to subclass from PluginInterface in order to implement a plugin:
 *
 * \code
 * class MyShinyPlugin : public PluginInterface
 * {
 *     ...
 * }
 * \endcode
 * 
 * Within this definition, the user might want to override some of the follwing
 * methods:
 * - getAtoms() \n
 *   At least, the plugin will contain some external atoms, which
 *   are implemented by deriving custom classes from PluginAtom and then are
 *   incorporated by registering them in the getAtoms function.
 * - createConverter(), createRewriter() \n
 *   A plugin can provide a \e Converter
 *   (see PluginConverter), which will receive the input program before dlvhex
 *   will start looking at it. With this facility, a plugin can for example
 *   provide a conversion from a different language to a hex-program. After
 *   dlvhex has parsed the input program, it will pass it to those found plugins
 *   that have provided a \e Rewriter (see PluginRewriter), which can alter the
 *   program to their liking, e.g., implement some syntactic sugar regarding
 *   their external atoms. In fact, a plugin could also supply only a converter
 *   or rewriter and no external atoms and thus act as a pure preprocessing
 *   stage for dlvhex.
 * - createOptimizer() \n
 *   Before a hex-program is evaluated, dlvhex builds its dependency graph and
 *   passes this graph to those plugins that have implemented an \e Optimizer
 *   (see PluginOptimizer). Such an optimizer can then modify the graph
 *   directly, which is more intuitive in case of optimization tasks than to
 *   work on the textual program repesentation.
 * - setOptions() \n
 *   A plugin can receive switches from the command line invocation
 *   of dlvhex. All command line parameters that are unknown to dlvhex will be
 *   passed to the plugins, which can then check this list for their own
 *   switches.
 * .
 *
 *
 */
class DLVHEX_EXPORT PluginInterface
{
 protected:
  
  /// Ctor.
  PluginInterface()
    : pluginName(""),
    versionMajor(0),
    versionMinor(0),
    versionMicro(0)
  { }

  std::string pluginName;

  unsigned versionMajor;
  unsigned versionMinor;
  unsigned versionMicro;

public:
    /// Dtor.
    virtual
    ~PluginInterface()
    { }

    /**
     * \brief Associates atom names with function pointers.
     */
    typedef std::map<std::string, boost::shared_ptr<PluginAtom> > AtomFunctionMap;

    /**
     * \brief Converter.
     *
     * By overloading this function, a plugin can implement a custom preparser,
     * which will be called first in the entire dlvhex-processing chain. A
     * converter can expect any kind of input data, and must return either the
     * original input data or a well-formed hex-program. With this facility, a
     * preparser can for instance convert a different rule- or query-language to
     * a hex-program.
     */
    virtual PluginConverter* 
    createConverter()
    {
      return 0;
    }

    /**
     * \brief Converter.
     *
     * By overloading this function, a plugin can implement a list of
     * custom preparser, which will be called first in the entire
     * dlvhex-processing chain. A converter can expect any kind of
     * input data, and must return either the original input data or a
     * well-formed hex-program. With this facility, a preparser can
     * for instance convert a different rule- or query-language to a
     * hex-program.
     *
     * This method is called by dlvhex to get a list of
     * converters. Overload it if you need more than one converter.
     */
    virtual std::vector<PluginConverter*>
    createConverters()
    {
      PluginConverter* pc = this->createConverter();
      return pc 
	? std::vector<PluginConverter*>(1, pc) 
	: std::vector<PluginConverter*>();
    }

    /**
     * \brief Rewriter for hex-programs.
     *
     * The rewriters are called second after the preparsers. Hence, a rewriter
     * can expect a well-formed hex-program as input and must of course also
     * take care of returning a correct program. A rewriter can realize
     * syntactic sugar, e.g., providing a simplified syntax for the user which is
     * then transformed, depending probably on the entire rule body.
     */
    virtual PluginRewriter* 
    createRewriter()
    {
      return 0;
    }

    /**
     * \todo doc.
     */
    virtual PluginOptimizer* 
    createOptimizer()
    {
      return 0;
    }

    /**
     * creates an OutputBuilder
     */
    virtual OutputBuilder*
    createOutputBuilder()
    {
      return 0;
    }

    /**
     * Altering the ProgramCtx is done just before we start our
     * engines and evaluate the Program.
     */
    virtual void
    setupProgramCtx(ProgramCtx&)
    {  }

    /**
     * \brief Fills a mapping from atom names to the plugin's atom objects.
     *
     * This is the central location where the user's atoms are made public.
     * dlvhex will call this function for all found plugins, which write their
     * atoms in the provided map. This map associates strings with pointers to
     * PluginAtom objects. The strings denote the name of the atom as it should
     * be used in the program.
     *
     * Example:
     *
     * \code
     * void
     * getAtoms(AtomFunctionMap& a)
     * {
     *     boost::shared_ptr<PluginAtom> newatom(new MyAtom);
     *     a["newatom"] = newatom;
     * }
     * \endcode
     *
     * Here, we assume to have defined an atom MyAtom derived from PluginAtom.
     * This atom can now be used in a hex-program with the predicate \b
     * &newatom.
     *
     * Naturally, more than one atoms can be registered here:
     *
     * \code
     * boost::shared_ptr<PluginAtom> split(new SplitAtom);
     * boost::shared_ptr<PluginAtom> concat(new ConcatAtom);
     * boost::shared_ptr<PluginAtom> substr(new SubstringAtom);
     * a["split"] = split;
     * a["concat"] = concat;
     * a["substr"] = substr;
     * \endcode
     */
    virtual void
    getAtoms(AtomFunctionMap&)
    { }

    /**
     * \brief Propagates dlvhex program options to the plugin.
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

    /**
     * \brief Set plugin name.
     *
     * The plugin name will be displayed when dlvhex loads the
     * plugin. This method is not supposed to be overridden, but only
     * called in the PLUGINIMPORTFUNCTION() (see Section \ref
     * importing).
     */
    void
    setPluginName(const std::string& name)
    {
      this->pluginName = name;
    }

    /**
     * \brief Set plugin version.
     *
     * The version number will be displayed when dlvhex loads the plugin. It can
     * be used to check whether the right version is loaded. This method is not
     * supposed to be overridden, but only called in the PLUGINIMPORTFUNCTION()
     * (see Section \ref importing).
     */
    void
    setVersion(unsigned major, unsigned minor, unsigned micro)
    {
      this->versionMajor = major;
      this->versionMinor = minor;
      this->versionMicro = micro;
    }

    const std::string&
    getPluginName() const
    {
      return this->pluginName;
    }

    unsigned
    getVersionMajor() const
    {
      return this->versionMajor;
    }

    unsigned
    getVersionMinor() const
    {
      return this->versionMinor;
    }

    unsigned
    getVersionMicro() const
    {
      return this->versionMicro;
    }
};

DLVHEX_NAMESPACE_END

#endif // _DLVHEX_PLUGININTERFACE_H


// Local Variables:
// mode: C++
// End:
