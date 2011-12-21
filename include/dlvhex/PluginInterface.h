/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Thomas Krennwallner
 * Copyright (C) 2009, 2010 Peter Schüller
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
 * \author Peter Schüller
 *
 * \brief Interface that can/should be implemented by a plugin.
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
 * 
 * By definition the same query should return the same result, this is used by the 
 * method retrieveCached() which has the effect that retrieve() is never called twice
 * for the same query (if you do not disable the cache via --nocache).
 * 
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
 *         getAtoms(PluginAtomMap& a)
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
#include "dlvhex/fwd.hpp"
#include "dlvhex/ID.hpp"
#include "dlvhex/Atoms.hpp"
#include "dlvhex/Error.h"
#include "dlvhex/CDNLSolver.hpp"

#include <map>
#include <string>
#include <iosfwd>

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>


#define PLUGINIMPORTFUNCTION importPlugin
#define PLUGINIMPORTFUNCTIONSTRING "importPlugin"


DLVHEX_NAMESPACE_BEGIN

//
// base class for plugin specific data stored in ProgramCtx
// (e.g., whether the plugin is enabled in that ProgramCtx, ...)
//
class PluginData
{
public:
  PluginData() {}
  virtual ~PluginData() {}
};

//
// callback functor for processing models generated by the model generator
// setup these in setupProgramCtx()
//
class ModelCallback:
  public std::unary_function<bool, AnswerSetPtr>
{
public:
  virtual ~ModelCallback() {}
  // returning true continues the model generation process
  // returning false stops the model generation process (afterwards the final
  virtual bool operator()(AnswerSetPtr as) = 0;
};
typedef boost::shared_ptr<ModelCallback> ModelCallbackPtr;

//
// callback functor for processing models generated by the model generator
// setup these in setupProgramCtx()
//
class FinalCallback
{
public:
  virtual ~FinalCallback() {}
  virtual void operator()() = 0;
};
typedef boost::shared_ptr<FinalCallback> FinalCallbackPtr;

/**
 * \brief Converter class.
 *
 * A converter can be seen as a preprocessor, which receives the raw input
 * program. A converter can either decide to parse the program and return a
 * well-formed HEX-program or simply leave the input untouched.
 */
class DLVHEX_EXPORT PluginConverter
{
public:
  virtual ~PluginConverter() { }

  /**
   * Conversion function.
   *
   * The input program is read from i. The output must be passed on to the
   * stream o, either the original input stream or the result of a conversion.
   */
  virtual void convert(std::istream& i, std::ostream& o) = 0;
};
typedef boost::shared_ptr<PluginConverter> PluginConverterPtr;


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
  PluginRewriter() {}

public:
  /**
   * Destructor.
   */
  virtual ~PluginRewriter() {}

  /**
   * Rewriting funcition.
   *
   * The rewriting is applied to a ProgramCtx object.
   *
   * Especially ctx.edb and ctx.idb may be the subject of rewriting.
   */
  virtual void rewrite(ProgramCtx& ctx) = 0;
};
typedef boost::shared_ptr<PluginRewriter> PluginRewriterPtr;

#if 0
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
#endif


/**
 * \brief Interface class for external Atoms.
 *
 * \ingroup pluginframework
 *
 * An external atom is represented by a subclassed PluginAtom. The actual
 * evaluation function of the atom is realized in the PluginAtom::retrieve()
 * method. In the constructor, the user must specify monotonicity, the types of
 * input terms and the output arity of the atom.
 */
class PluginAtom;
typedef boost::shared_ptr<PluginAtom> PluginAtomPtr;
typedef boost::weak_ptr<PluginAtom> PluginAtomWeakPtr;
class DLVHEX_EXPORT PluginAtom
{
public:
  /**
   * \brief Query class for wrapping the input of an external atom call.
   */
  struct DLVHEX_EXPORT Query
  {
    InterpretationConstPtr interpretation;
    Tuple input;
    Tuple pattern;
    const ExternalAtom* eatom;

    /**
     * The input arguments are the ground terms of the input list.
     *
     * The output tuple corresponds to the atom's output list: if it contains
     * variables, the query will be a functional one for those missing values; if
     * it is nullary or completely ground, the query will be a boolean one.
     *
     * The answer shall contain exactly those tuples that match the pattern and are
     * in the output of the atom's function for the interpretation and the input
     * arguments.
     */
    // interpretation is a bitset of ground atoms,
    //   where the predicate is equal to the constant of a predicate input
    // input is a vector of constant/integer term IDs:
    //   for predicate inputs it is the predicate of the ground atoms
    //   for constant inputs it is the input constant
    //     (possibly obtained from an auxiliary input rule+predicate)
    // pattern is a vector of term IDs:
    //   answer tuples must contain the same number of terms as the pattern tuple
    //   constants must be the same
    //   variables must be substituted
    //   TODO verify and specify if the same variable may be substituted differently: may  &dosomething[](X,b,X) return an answer tuple (a,b,c)?
    Query(InterpretationConstPtr interpretation,
          const Tuple& input,
          const Tuple& pattern):
      interpretation(interpretation),
      input(input),
      pattern(pattern),
      eatom(0) {}

    Query(InterpretationConstPtr interpretation,
          const Tuple& input,
          const Tuple& pattern,
          const ExternalAtom* ea):
      interpretation(interpretation),
      input(input),
      pattern(pattern),
      eatom(ea) {}

    // no member encapsulation and no accessors as this will always be supplied to plugins as a const ref

    // strict weak ordering required for using this as index in a map
    // TODO we should use a hash map instead
    //bool operator<(const Query& other) const;

    // equality for hashing
    bool operator==(const Query& other) const;
  };

  /**
   * \brief Answer class for wrapping the output of an external atom call.
   */
  struct DLVHEX_EXPORT Answer
  {
    Answer();

    // simple accessors, controlled by constness of object
    std::vector<Tuple>& get() { used = true; return *output; }
    const std::vector<Tuple>& get() const { return *output; }

    // for efficient hashtable management
    // whether the internal vector<Tuple> it has been written to yet
    bool hasBeenUsed() const { return used; }
    void use() { used = true; }

    Answer& operator=(const Answer& other)
      { output = other.output; used = true; return *this; }

    // do not allow comparison (will produce linker error)
    bool operator==(const Answer& other) const;

  private:
    // use shared ptr to have low-cost-copying of this object while the vector of tuples need not be copied
    boost::shared_ptr<std::vector<Tuple> > output;
    // true if this was default-constructed and never used
    bool used;
  };

  /**
   * \brief Type of input parameter.
   *
   * Three types of input parameters can be specified: PREDICATE, CONSTANT, and TUPLE.
   *
   * * An input argument of type PREDICATE means that the atom needs those facts
   *   of the interpretation whose predicate match the value of this argument.
   * * An input argument of type CONSTANT means that only its value is relevent
   *   to the external atom, regardless of the interpretation.
   * * An input argument of type TUPLE may be specified as last input type only, it
   *   means that an unspecified number of CONSTANT values may be specified after
   *   other inputs.
   *
   * Specifying the input parameters' types is necessary for reducing the
   * interpretation that will be passed to the external atom as well as
   * improving the dependency information used by the internal
   * evaluation strategies of dlvhex.
   */
  typedef enum { PREDICATE, CONSTANT, TUPLE } InputType;

protected:
  // The user must call this constructor when creating custom external
  // atoms.
  // \param predicate
  //   The name of the external atom as it appears in the HEX program
  //   must be specified here. (This is required to create an auxiliary
  //   name and it is useful to have a tight connection of each PluginAtom
  //   with its name, as well as a unified way to get the name of a plugin
  //   atom.)
  // \para monotonic 
  //   This boolean indicates whether the atom is monotonic or not.
  //   Specifying false is always allowed, but might degrade evaluation
  //   performance in cases where a fixed point calculation could be used
  //   instead of guess and check model building.
  //
  // within the derived constructor, you have to define inputs using
  // addInput...() methods and you have to use setOutputArity().
  PluginAtom(const std::string& predicate, bool monotonic):
    predicate(predicate),
    monotonic(monotonic) { }

  // The following functions are to be used in the constructor only:

  /// \brief Adds an input parameter of type PREDICATE.
  void addInputPredicate();
  /// \brief Adds an input parameter of type CONSTANT.
  void addInputConstant();
  /// \brief Adds an input parameter of type TUPLE.
  void addInputTuple();

  /// \brief Specifies the output arity of the external Atom.
  /// (This one always has to be fixed.)
  void setOutputArity(unsigned arity);

public:
  virtual ~PluginAtom() {}

  /**
   * \brief Checks the input arity of the external atom against the
   * specified arity.
   *
   * The input arity follows from the number of specified predicates and might be variable.
   *
   * \returns true iff arity matches. 
   */
  bool checkInputArity(unsigned arity) const;

  /**
   * \brief Checks whether the output arity of the external atom is
   * compatible with the specified arity.
   */
  bool checkOutputArity(unsigned arity) const;

  /**
   * \brief Retrieve answer object according to a query,
   * probably utilizing a cache s.t. the same query is not retrieved twice.
   *
   * This function implements the query cache and will rarely need to be
   * overridden.
   */
  virtual void retrieveCached(const Query&, Answer&);
  virtual void retrieveCached(const Query&, Answer&, CDNLSolverPtr solver);

  /**
   * \brief Retrieve answer object according to a query.
   *
   * This function implements the external atom.
   */
  virtual void retrieve(const Query&, Answer&) = 0;
  virtual void retrieve(const Query&, Answer&, CDNLSolverPtr solver);

  Nogood getInputNogood(CDNLSolverPtr solver, const Query& query);
  Set<ID> getOutputAtoms(CDNLSolverPtr solver, const Query& query, const Answer& answer);
  ID getOutputAtom(bool sign, const Query& query, Tuple t);

  /**
   * \brief Returns the type of the input argument specified by position
   * (starting with 0). Returns TUPLE for TUPLE input arguments (variably many).
   */
  InputType getInputType(unsigned index) const;

  /**
   * @return inputType
   */
  const std::vector<InputType>& getInputTypes() const
    { return inputType; }

  /**
   * @return monotonic
   */
  bool isMonotonic() const
    { return monotonic; }

  // Associate plugin atom with registry pointer.
  // (This implicitly calculates the predicate ID.)
  virtual void setRegistry(RegistryPtr reg);

  /// \brief get associated Registry
  RegistryPtr getRegistry() const
    { return registry; }

  /// \brief get ID of the predicate name (only works with configured registry)
  ID getPredicateID() const
    { return predicateID; }

  /// \brief get predicate name (which was specified in constructor)
  const std::string& getPredicate() const
    { return predicate; }
  
protected:
  // the predicate of the atom as it appears in HEX programs
  // (without leading &)
  //
  // this is not stored as ID, because plugins must be allowed to create
  // atoms without knowing the registry they will be used with
  //
  // setting the registry provides the predicate ID
  std::string predicate;

  // the id of the predicate name, ID_FAIL if no registry is set
  ID predicateID;

  /// \brief whether the function is monotonic or nonmonotonic
  bool monotonic;

  /// \brief Type of each input argument (only last may be TUPLE).
  std::vector<InputType> inputType;

  /// \brief Number of output arguments.
  unsigned outputSize;

  /// \brief Query/Answer cache
  typedef boost::unordered_map<const Query, Answer> QueryAnswerCache;
  QueryAnswerCache queryAnswerCache;

  /// \brief output tuples generated so far (used for learning for
  //         functional sources)
  std::vector<Tuple> otuples;

  /// \brief Registry associated with this atom
  //
  // This association cannot be done by the plugin itself, it is done by
  // the PluginContainer loading or receiving the plugin.
  RegistryPtr registry;
};

// hash function for QueryAnswerCache
std::size_t hash_value(const PluginAtom::Query& q);

/// \brief Associates atom names with PluginAtom instances implementing them.
typedef std::map<std::string, PluginAtomPtr> PluginAtomMap;

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
  PluginInterface():
    pluginName(),
    versionMajor(0),
    versionMinor(0),
    versionMicro(0)
  { }

  std::string pluginName;
  unsigned versionMajor;
  unsigned versionMinor;
  unsigned versionMicro;

  void setNameVersion(const std::string& name, unsigned major, unsigned minor, unsigned micro)
  {
    pluginName = name;
    versionMajor = major;
    versionMinor = minor;
    versionMicro = micro;
  }

public:
  virtual ~PluginInterface() { }

  /**
   * \brief Converter.
   *
   * By overloading this function, a plugin can implement one custom preparser,
   * which will be called first in the entire dlvhex-processing chain. A
   * converter can expect any kind of input data, and must return either the
   * original input data or a well-formed hex-program. With this facility, a
   * preparser can for instance convert a different rule- or query-language to
   * a hex-program.
   *
   * Do not overload this one if you overload createConverters()
   */
  virtual PluginConverterPtr createConverter(ProgramCtx&)
    { return PluginConverterPtr(); }

  /**
   * \brief Multiple Converters.
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
  virtual std::vector<PluginConverterPtr> createConverters(ProgramCtx& ctx)
  {
    std::vector<PluginConverterPtr> ret;

    PluginConverterPtr pc = this->createConverter(ctx);
    if( pc )
      ret.push_back(pc);
    return ret;
  }

  /**
   * \brief Provide parser modules
   * 
   * This is the preferred way to extend the input language by supplying dlvhex
   * with parser modules that plug into the HEX grammar and extend the syntax for
   * valid program input.
   *
   * See the QueryPlugin and the StrongNegationPlugin for example plugins that
   * use this feature.
   */
  virtual std::vector<HexParserModulePtr> createParserModules(ProgramCtx&)
    { return std::vector<HexParserModulePtr>(); }

  /**
   * \brief Provide alternative parser
   * 
   * This method can be overwritten to provide an alternative HEX parser,
   * e.g., for implementing slightly changed input syntax.
   */
  virtual HexParserPtr createParser(ProgramCtx&)
    { return HexParserPtr(); }

  /**
   * \brief Rewriter for hex-programs.
   *
   * The rewriters are called second after the preparsers. Hence, a rewriter
   * can expect a well-formed hex-program as input and must of course also
   * take care of returning a correct program. A rewriter can realize
   * syntactic sugar, e.g., providing a simplified syntax for the user which is
   * then transformed, depending probably on the entire rule body.
   */
  virtual PluginRewriterPtr createRewriter(ProgramCtx&)
    { return PluginRewriterPtr(); }

  #warning implement optimizer
  #if 0

  /**
   * \todo doc.
   */
  virtual PluginOptimizer* 
  createOptimizer()
  {
    return 0;
  }
  #endif

  /**
   * Altering the ProgramCtx permits plugins to do many things, e.g.,
   * * installing model and finish hooks
   * * removing default model (and finish) hooks
   * * setting maxint
   * * changing and configuring the solver backend to be used
   * * TODO other useful examples?
   */
  virtual void setupProgramCtx(ProgramCtx&) {  }

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
   * getAtoms(PluginAtomMap& a)
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
  virtual std::vector<PluginAtomPtr> createAtoms(ProgramCtx& ctx) const
    { return std::vector<PluginAtomPtr>(); }

  const std::string& getPluginName() const
    { return this->pluginName; }
  unsigned getVersionMajor() const
    { return this->versionMajor; }
  unsigned getVersionMinor() const
    { return this->versionMinor; }
  unsigned getVersionMicro() const
    { return this->versionMicro; }

	// output help message for this plugin
	virtual void printUsage(std::ostream& o) const;

	// processes options for this plugin, and removes recognized options from pluginOptions
  // (do not free the pointers, the const char* directly come from argv)
	virtual void processOptions(std::list<const char*>& pluginOptions, ProgramCtx& ctx);
};
// beware: most of the time this Ptr will have to be created with a "deleter" in the library
typedef boost::shared_ptr<PluginInterface> PluginInterfacePtr;

//
// deleters
//

// this class should be used as a "deleter" for boost::shared_ptr if the library
// returning the pointer wants to return an object allocated with "new"
// this must stay inline! (the correct delete operator must be used by the library)
template<typename AllocatedT>
struct PluginPtrDeleter
{
  PluginPtrDeleter() {}
  inline void operator()(const AllocatedT* ptr) const
  {
    delete ptr;
  }
};

// this class should be used as a "deleter" for boost::shared_ptr if the library
// returning the pointer wants to return a statically linked object (i.e., do not free)
// [this is used for all PluginInterface pointers, as this pointer is returned as a
// POD type via an extern "C" function]
template<typename AllocatedT>
struct PluginPtrNOPDeleter
{
  PluginPtrNOPDeleter() {}
  inline void operator()(const AllocatedT* ptr) const
  {
    // NOP = do not delete
  }
};

DLVHEX_NAMESPACE_END

#endif // _DLVHEX_PLUGININTERFACE_H

// Local Variables:
// mode: C++
// End:
