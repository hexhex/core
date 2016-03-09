/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005-2007 Roman Schindlauer
 * Copyright (C) 2006-2015 Thomas Krennwallner
 * Copyright (C) 2009-2016 Peter Schüller
 * Copyright (C) 2011-2016 Christoph Redl
 * Copyright (C) 2015-2016 Tobias Kaminski
 * Copyright (C) 2015-2016 Antonius Weinzierl
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
 * \author Peter Schller
 * \author Christoph Redl
 *
 * \brief Interface that can/should be implemented by a plugin.
 */

/**
 * \defgroup pluginframework The C++ Plugin Framework
 *
 * \section introduction Introduction
 *
 * The dlvhex reasoner evaluates Answer %Set Programs with external atoms. One important
 * design principle was to provide a mechanism to easily add further external
 * atoms without having to recompile the main application. A \em plugin is a
 * shared library that provides functions to realise custom external atoms.
 * Furthermore, a plugin can supply rewriting facilities, which may alter the
 * input logic program prior to evaluation.
 * Plugins can be written in C++ as described below or in Python as described
 * \ref pythonpluginframework "here"; the latter does not have all features of the C++
 * version but should be sufficient in almost all cases.
 *
 * This Section gives an overview of dlvhex plugins and external atoms.
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
 *   &rdf[uri](X,Y,Z)
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
 * (if at all). Considering this as well as for efficiency reasons, we created
 * two (in the implementation three, see below) categories of input parameters:
 * - the Constant parameter and
 * - the Predicate parameter.
 * A third category called Tuple exists, which is a meta category standing for
 * an arbitrary mount of Constant input parameters. This is useful, e.g.,
 * for &concat[s1,s2,...](Out).
 *
 * A parameter of type "Constant" is not related to the interpretation at all,
 * like in the previous example of the RDF-atom. A parameter is of type
 * "Predicate" means that all facts in the interpretation with this predicate
 * are necessary for the atom. Let's assume, we have an external atom that
 * calculates the overall price of a number of books given by their ISBN number:
 *
 * \code
 *   &overallbookprice[isbn](X)
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
 * \section writingplugin Writing a C++ Plugin
 * We wanted to keep the interface between dlvhex and the plugins as lean as
 * possible.
 * Necessary tasks are:
 * - Registering the plugin with its interface and its version.
 *   This is easiest done using the define IMPLEMENT_PLUGINABIVERSIONFUNCTION
 *   and PLUGINIMPORTFUNCTION (see testsuite/TestPlugin.cpp)
 * - Implementing the PluginInterface (only the constructor is really mandatory).
 *
 * Usual tasks for implementing a plugin interface are at least one of the
 * following. We give the most usual implementation tasks first:
 * - external atoms (method PluginInterface::createAtoms)
 * - a usage (--help) message (method PluginInterface::printUsage)
 * - commandline option processing (method PluginInterface::processOptions)
 * - input converter (method createConverter or PluginInterface::createConverters)
 * - ProgramCtx modifications before evaluation (method PluginInterface::setupProgramCtx)
 * - input parser modules (method createParserModules or PluginInterface::createParser)
 * - program rewriter (method PluginInterface::createRewriter)
 * - dependency graph optimizer (method PluginInterface::createOptimizer)
 *
 * In particular, the task of defining external atoms which can be used in the HEX-program
 * is done by implementing a class derived from PluginAtom
 * and registering it via PluginInterface::createAtoms.
 * Each such class defines exactly one external predicate (e.g. &concat) by implementing
 * the method PluginAtom::retrieve, which comes in two overloaded versions depending on whether
 * customized learning techniques shall be used or not.
 *
 * For details about each of these tasks see the respective method
 * documentation, the test plugin in testsuite/TestPlugin.cpp,
 * the internal plugins
 * - QueryPlugin.{cpp,hpp},
 * - StrongNegationPlugin.{cpp,hpp}, and
 * - HigherOrderPlugin.{cpp,hpp},
 * as well as other basic plugins like the stringplugin and the scriptplugin.
 *
 * \section implementingeatoms Implementing External Atoms
 *
 * For implementing external computations there are two very different
 * possibilities: ComfortPluginAtom and PluginAtom.
 * - ComfortPluginAtom (in header ComfortPluginInterface.h)
 *   This interface requires no knowledge of the ID mechanism of dlvhex;
 *   every class processed in ComfortPluginAtom is defined in
 *   ComfortPluginInterface.h therefore this interface makes it easy to
 *   start developing with dlvhex. However this comes at the cost of
 *   performance.
 * - PluginAtom (in header PluginInterface.h)
 *   This interface is the native interface to implement external
 *   computations, in fact ComfortPluginAtom is implemented using
 *   PluginAtom. Using PluginAtom requires knowledge of how to deal with
 *   the Registry and ID classes.
 *
 * It is recommended to start prototyping using ComfortPluginAtom and then
 * later reimplement performance-critical external computations in the
 * PluginAtom interface. (The HEX programs do not change at all, just the
 * implementation of the external atom.)
 *
 * In addition to the actual semantics of an external atom, the class PluginAtom
 * allows allows for defining customized learning functions.
 * To this end, the method PluginAtom::retrieve comes with an overloaded
 * version which gets a pointer to a NogoodContainer where customized nogoods
 * can be added to restrict the search space.
 * For instance, the nogood <em>{ p(a), -q(a), -&diff[p,q](a) }</em> encodes that whenever the atom <em>p(a)</em> is true and the atom
 * <em>q(a)</em> is false, then the atom &diff[p,q](a) must be true (i.e. must not be false) since constant <em>a</em>
 * will be in the output of the set difference of <em>p</em> and <em>q</em>.
 * We refer to the classes Nogood and NogoodContainer for details.
 * Moreover, the class ExternalLearningHelper contains some methods which help with the construction of nogoods.
 *
 * In TestPlugin.cpp, some plugin atoms are implemented both using
 * PluginAtom and ComfortPluginAtom in order to provide an example of the
 * differences.
 *
 * \section compiling Building Plugins
 *
 * We provide a toolkit for building plugins based on the GNU
 * Autotools (http://sourceware.org/autobook/)
 * environment. This means that you will need the respective tools installed on
 * your system - which is usually no problem, since they are provided as packages
 * by all major linux distributions.
 * You need packages for libtool, automake, and autoconf.
 * We also provide a dlvhex.m4 script for easier configuration of DLVHEX plugins,
 * see for its usage the dlvhex-stringplugin or the dlvhex-scriptplugin.
 *
 * \section usage Using a C++ Plugin
 *
 * In order to load a Python-implemented plugin stored as shared object in a directory PATH,
 * pass the additional option \code --plugindir=PATH \endcode to dlvhex.
 * Alternatively, you make install the plugin in the global plugin directory by calling <em>make install</em>.
 */
#if !defined(_DLVHEX_PLUGININTERFACE_H)
#define _DLVHEX_PLUGININTERFACE_H

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/fwd.h"
#include "dlvhex2/ID.h"
#include "dlvhex2/Atoms.h"
#include "dlvhex2/Error.h"
#include "dlvhex2/CDNLSolver.h"
#include "dlvhex2/ComponentGraph.h"
#include "dlvhex2/ExtSourceProperties.h"
#include "dlvhex2/ExternalAtomEvaluationHeuristicsInterface.h"

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>
#include <boost/thread/mutex.hpp>

#include <map>
#include <string>
#include <iosfwd>
#include <algorithm>

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>

/**
 * Definitions for C ABI interface for dlvhex plugins.
 * We store the version like boost as an integer.
 * We use the apache APR approach to versioning.
 * - see PluginContainer.cpp for version processing,
 * - see Trac wiki page LibraryVersions on the general approach and rationale,
 * - see TestPlugin.cpp for an example plugin implementation
 *
 * The following code must be used by a plugin to publish their
 * ABI compatibility:
 * - in configure.ac use dlvhex.m4 and the macro
 *
 * \code
 *   DLVHEX_DEFINE_VERSION([DLVHEX_ABI],[X.Y.Z])
 * \endcode
 *
 * - in the library source file use
 *
 * \code
 *   IMPLEMENT_PLUGINABIVERSIONFUNCTION
 * \endcode
 *
 * See the documentation of PluginInterface for how to publish
 * your plugin interface.
 */
#define PLUGINABIVERSIONFUNCTION getDlvhex2ABIVersion
#define PLUGINABIVERSIONFUNCTIONSTRING "getDlvhex2ABIVersion"

#define PLUGINVERSIONFUNCTION getDlvhexPluginVersion
#define PLUGINVERSIONFUNCTIONSTRING "getDlvhexPluginVersion"
#define PLUGINIMPORTFUNCTION importPlugin
#define PLUGINIMPORTFUNCTIONSTRING "importPlugin"
#define IMPLEMENT_PLUGINABIVERSIONFUNCTION \
    extern "C" DLVHEX_PLUGINEXPORT int PLUGINABIVERSIONFUNCTION() \
    { \
        return DLVHEX_ABI_VERSION_MAJOR*10000+\
        DLVHEX_ABI_VERSION_MINOR*100+\
        DLVHEX_ABI_VERSION_MICRO; \
    }

DLVHEX_NAMESPACE_BEGIN

/**
 * \brief Factory where plugins interact with the dlvhex core.
 *
 * \section intro Overview
 *
 * The PluginInterface class can be seen as a wrapper for all user-defined
 * features of a plugin, which are:
 * - announce external atoms
 * - supply convert/rewrite/parse/optimize facilities
 * - supply model and final callbacks
 * - offer help messages and command line options
 *
 * You shall derive from PluginInterface in order to implement a plugin,
 * the constructor shall set name and version of the plugin,
 * preferrably defined in configure.ac using DLVHEX_DEFINE_VERSION.
 * (See dlvhex-stringplugin for an example.)
 *
 * \subsection importing Importing the Plugin
 *
 * To make the plugin available, we need to instantiate it. The simplest way to do
 * this is to define a global variable and the following code:
 * Assume your plugin class is RDFPlugin derived from PluginInterface and you have
 *
 * \code
 *   RDFPlugin theRDFPlugin;
 * \endcode
 *
 * Then you publish theRDFPlugin to dlvhex as follows:
 *
 * \code
 *   extern "C"
 *   void* PLUGINIMPORTFUNCTION()
 *   {
 *     return reinterpret_cast<void*>(&theRDFPlugin);
 *   }
 * \endcode
 */
class DLVHEX_EXPORT PluginInterface
{
    protected:
        /**
         * \brief Constructor
         *
         * Write your own constructor and set the version using setNameVersion().
         */
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

        /**
         * \brief Set plugin name and version (informative, this is not the ABI version).
         *
         * Set the version in your own constructor. It is recommended you
         * add to configure.ac the line
         *
         * \code
         *   DLVHEX_DEFINE_VERSION([<YOURPLUGINNAME>],[$PACKAGE_VERSION])
         * \endcode
         *
         * and use for setNameVersion() the defines in config.h.in as created by
         * configure (see also dlvhex.m4).
         */
        void setNameVersion(
            const std::string& name, unsigned major,
        unsigned minor, unsigned micro) {
            pluginName = name;
            versionMajor = major;
            versionMinor = minor;
            versionMicro = micro;
        }

    public:
        virtual ~PluginInterface() {}

        /**
         * \brief Publish external computation sources to dlvhex.
         *
         * This is the central location where the user's atoms are made public.
         * dlvhex will call this function for all found plugins, which write their
         * atoms in the provided map. This map associates strings with pointers to
         * PluginAtom objects. The strings denote the name of the atom as it should
         * be used in the program.
         *
         * Override this method to publish your atoms.
         *
         * Example:
         *
         * \code
         *   std::vector<PluginAtomPtr>
         *   createAtoms(ProgramCtx& ctx) const
         *   {
         *   	std::vector<PluginAtomPtr> ret;
         *      PluginAtomPtr newatom1(new MyAtom1);
         *      PluginAtomPtr newatom2(new MyAtom2);
         *      ret["newatom1"] = newatom1;
         *      ret["newatom2"] = newatom2;
         *      return ret;
         *   }
         * \endcode
         *
         * Here, we assume to have defined atoms MyAtom1 and MyAtom2
         * derived from PluginAtom. These atom can now be used in a
         * HEX-program with the predicate \b &newatom1[]() and
         * \b &newatom2[]().
         */
        virtual std::vector<PluginAtomPtr> createAtoms(ProgramCtx& ctx) const;

        /**
         * \brief Output help message for this plugin.
         */
        virtual void printUsage(std::ostream& o) const;

        /**
         * \brief Processes options for this plugin.
         *
         * If you override this method, remove all options your plugin recognizes from
         * pluginOptions. (Do not free the pointers, the const char* directly come from
         * argv.) You can store configuration of your plugin using PluginData and
         * ProgramCtx::getPluginData (see there for more information.)
         */
        virtual void processOptions(std::list<const char*>& pluginOptions, ProgramCtx& ctx);

        /**
         * \brief Provide PluginConverter.
         *
         * This method is called by createConverters, so if you overload
         * createConverters, do not overload createConverter.
         *
         * See PluginConverter documentation.
         */
        virtual PluginConverterPtr createConverter(ProgramCtx&);

        /**
         * \brief Provide multiple PluginConverter objects.
         *
         * This method calls createConverter, you can override it to provide
         * multiple converters.
         *
         * See PluginConverter documentation.
         */
        virtual std::vector<PluginConverterPtr> createConverters(ProgramCtx& ctx);

        /**
         * \brief Returns if this plugin provides a custom evaluation heuristics for a certain external atom.
         *
         * @param
         */
        virtual bool providesCustomModelGeneratorFactory(ProgramCtx& ctx) const { return false; }

        /**
         * \brief Must create a model generator factory for the component described by ci.
         * Needs to be implemented only if PluginAtom::providesCustomModelGeneratorFactory return true;
         */
        virtual BaseModelGeneratorFactoryPtr getCustomModelGeneratorFactory(ProgramCtx& ctx, const ComponentGraph::ComponentInfo& ci) const
            { assert(false && "This plugin does not provide a custom model generator factory"); return BaseModelGeneratorFactoryPtr(); }

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
        virtual std::vector<HexParserModulePtr> createParserModules(ProgramCtx&);

        /**
         * \brief Provide alternative parser
         *
         * This method can be overwritten to provide an alternative HEX parser, e.g., for
         * implementing slightly changed input syntax.
         */
        virtual HexParserPtr createParser(ProgramCtx&);

        /**
         * \brief Rewriter for hex-programs.
         *
         * The rewriters are called on the parsed HEX program (which may have been
         * rewritten by a PluginConverter). Hence, a rewriter can expect a well-formed
         * HEX-program represented in ProgramCtx::edb and ProgramCtx::idb as input and must
         * of course also take care of keeping that representation correct program.
         */
        virtual PluginRewriterPtr createRewriter(ProgramCtx&);

        /**
         * \brief Optimizer: may optimize dependency graph.
         */
        virtual PluginOptimizerPtr createOptimizer(ProgramCtx&);

        /**
         * Altering the ProgramCtx permits plugins to do many things, e.g.,
         * * installing model and finish callbacks
         * * removing default model (and final) hooks
         * * setting maxint
         * * changing and configuring the solver backend to be used
         * See internal plugins for example usage.
         */
        virtual void setupProgramCtx(ProgramCtx&) {  }

        const std::string& getPluginName() const
            { return this->pluginName; }
        unsigned getVersionMajor() const
            { return this->versionMajor; }
        unsigned getVersionMinor() const
            { return this->versionMinor; }
        unsigned getVersionMicro() const
            { return this->versionMicro; }

};
// beware: most of the time this Ptr will have to be created with a "deleter" in the library
typedef boost::shared_ptr<PluginInterface> PluginInterfacePtr;

/**
 * \brief Base class for plugin-specific storage in ProgramCtx
 *
 * Concrete usage pattern:
 * * derive from this in YourPluginClass::CtxData
 * * default construct this with ProgramCtx::getPluginData<YourPluginClass>
 * * obtain where needed via ProgramCtx::getPluginData<YourPluginClass>
 *
 * Use this, e.g., to store commandline arguments processed by the plugin.
 * (For an example see QueryPlugin.cpp.)
 */
class PluginData
{
    public:
        PluginData() {}
        virtual ~PluginData() {}
};
typedef boost::shared_ptr<PluginData> PluginDataPtr;

/**
 * \brief Base class for plugin-specific storage in ProgramCtx
 *
 * Concrete usage pattern:
 * * derive from this in YourPluginClass::Environment
 * * default construct this with ProgramCtx::getPluginEnvironment<YourPluginClass>
 * * obtain where needed via ProgramCtx::getPluginEnvironment<YourPluginClass>
 *
 */
class PluginEnvironment
{
    public:
        PluginEnvironment() {}
        virtual ~PluginEnvironment() {}
};
typedef boost::shared_ptr<PluginEnvironment> PluginEnvironmentPtr;

/**
 * \brief Interface class for external Atoms.
 *
 * \ingroup pluginframework
 *
 * An external atom is represented by a subclassed PluginAtom. The actual
 * evaluation function of the atom is realized in the PluginAtom::retrieve()
 * method. In the constructor, the user must specify monotonicity, the types of
 * input terms and the output arity of the atom.
 *
 * See also ComfortPluginInterface.
 *
 * \subsection extatom The External Atom
 *
 * First, we have to subclass PluginAtom to create our own \b RDFatom class:
 *
 * \code
 *   class RDFatom: public PluginAtom
 *   {
 * \endcode
 *
 * The constructor of \b RDFatom will be used to define some properties of the atom:
 *
 * \code
 *   public:
 *     RDFatom():
 *       PluginAtom("rdf", true)
 *     {
 *         addInputPredicate();
 *         setOutputArity(3);
 *     }
 * \endcode
 *
 * We first specify predicate "&rdf" and monotonicity (it is monotonic).
 * Then we specify the number and types of input parameters. The RDF-atom
 * as we used it above, has a single input parameter, which is the name of
 * the predicate to hold the URIs. In this case, we just need a single call
 * to PluginAtom::addInputPredicate(). Another possibility would have been
 * to define the single input term as a constant, which represents the URI
 * directly (which means, that then there can only be one URI). In this
 * case, the input interpretation would be irrelevant, since we don't need
 * the extension of any predicate. To define a constant input parameter, we
 * use PluginAtom::addInputConstant().
 *
 * If we wanted to have, say, two input parameters which will represent
 * predicate names and a third one which will be a constant, we would put
 * this sequence of calls in the constructor:
 *
 * \code
 *   addInputPredicate();
 *   addInputPredicate();
 *   addInputConstant();
 * \endcode
 *
 * The call \c setOutputArity(3) ensures that occurences of this atom with
 * other than three output terms will cause an error at parsing time.
 *
 * The only member function of the atom class that needs to be defined is
 * retrieve. There are two overloaded versions:
 *
 * \code
 *   virtual void
 *   retrieve(const Query& query, Answer& answer)
 *   {
 * \endcode
 *
 * and
 *
 * \code
 *   virtual void
 *   retrieve(const Query& query, Answer& answer, NogoodContainerPtr nogoods)
 *   {
 * \endcode
 *
 * retrieve() always takes a query object as input and returns the result tuples in an
 * answer object. The second version also takes a pointer to a NogoodContainer
 * which can be used for learning additional user-defined nogoods (see NogoodContainer and ExternalLearningHelper).
 * Note that this pointer can be NULL as external sources are sometimes evaluated without the need to learn something.
 * However, whenever it is not NULL, the external source is free to add customized nogoods in order to prune the search space.
 * If both methods are overridden, the second one takes precedence over the first one.
 *
 * By definition, the same query should return the same result, this is used by the
 * method retrieveCached() which has the effect that retrieve() is never called twice
 * for the same query (if you do not disable the cache via --nocache).
 *
 * In the implementation of retrieve, query.input contains the input tuple,
 * query.pattern contains the tuple of constants and variables an output
 * tuple should unify with, and query.interpretation is an interpretation
 * containing all relevant bits of predicate inputs.
 *
 * query.input and query.pattern store Term IDs, you resolve such IDs via
 * the table Registry::terms, except for integers which are directly
 * encoded in the ID::address member and are detected using ID::isInteger().
 *
 * Note that each constant has a unique ID in the Registry, therefore
 * obtaining const Term& from Registry::terms might not be necessary in all
 * cases, it might be sufficient to compare the ID without looking at the
 * actual (printable) content of the symbol.
 *
 * \code
 *   if( query.input[0].isInteger() )
 *   {
 *      std::cerr << "got integer " << query.input[0].address << std::endl;
 *   }
 *   else
 *   {
 *      const Term& t = registry->terms.getByID(query.input[0]);
 *      std::cerr << "got term " << t << std::endl;
 *   }
 * \endcode
 *
 * You obtain all ground atoms in the interpretation by iterating over its
 * bits, or by testing individual bits.
 *
 * \code
 *   Interpretation::TrueBitIterator it, it_end;
 *   for(boost::tie(it, it_end) = query.interpretation.trueBits();
 *       it != it_end; ++it)
 *   {
 *     const OrdinaryAtom& gatom = registry->ogatoms.getByAddress(*it);
 *     std::cerr << "got atom in interpretation: " << gatom << std::endl;
 *   }
 * \endcode
 *
 * Note that each ground atom has a unique ID in the Registry, therefore
 * obtaining const OrdinaryAtom& from Registry::ogatoms might not be
 * necessary in all cases, it might be sufficient to compare the ID without
 * looking at the actual (printable) content of the symbol.
 *
 * For more information about accessing interpretation and IDs, please
 * refer to the documentation and header files of the classes ID, Registry,
 * Term, Atom, Interpretation, ...
 *
 * At this point, the plugin author will implement the actual function of
 * the external atom, either within this class or by calling other
 * functions.
 *
 * Throwing exceptions will abort the model building process and display
 * the exception.
 *
 * Answer tuples of data type Tuple which is std::vector<ID>, are added to
 * the answer object as follows:
 *
 * \code
 *   Tuple tuple;
 *   answer.get().push_back(tuple);
 * \endcode
 *
 * IDs put into tuple must either be integer IDs or Term IDs that have been
 * registered in table Registry::terms.
 *
 * If an external atom specifies that it provides partial answers (see ExtSourceProperties::providesPartialAnswer),
 * it further must add all tuples which can become true if currently unassigned input atoms are defined
 * using \code answer.getUnknown().push_back(tuple) \endcode.
 * As this might be the case for infinitely many tuples,
 * the (finite) set of relevant ones can be retrieved from the output atoms in the set \code replacements \endcode
 * of all positive replacement atoms stemming from this external atom.
 * 
 * \subsection registeratoms Registering the Atoms
 *
 * So far, we described the implementation of a specific external atom. In order
 * to integrate one or more atoms in the interface framework, the plugin author
 * needs to subclass PluginInterface and register all external atoms in
 * PluginInterface::createAtoms (see documentation of PluginInterface).
 */
class DLVHEX_EXPORT PluginAtom
{
    public:
        /**
         * \brief Query class which provides the input of an external atom call.
         *
         * The Query is passed to external computations as a const ref, therefore its data
         * members are not encapsulated.
         *
         * Query::input contains the ground terms of the input list.
         *
         * Query::pattern tuple corresponds to the atom's output list: if it contains
         * variables, the query will be a functional one for those missing values; if
         * it is nullary or completely ground, the query will be a boolean one.
         *
         * The answer shall contain exactly those tuples that match the pattern and are
         * in the output of the atom's function for the interpretation and the input
         * arguments.
         *
         * Query objects are passed to PluginAtom::retrieve or PluginAtom::retrieveCached.
         */
        struct DLVHEX_EXPORT Query
        {
            /**
             * \brief Reference to the active program context.
             */
            const ProgramCtx* ctx;

            /**
             * \brief Bitset of ground atoms representing current (partial) model
             * projected to the input of this external atom.
             *
             * Partial model contains bits about all atoms relevant for the computation,
             * i.e., all atoms with a predicate equal to a constant given for a predicate
             * input in the input tuple.
             */
            InterpretationConstPtr interpretation;

            /**
             * \brief Bitset of ground atoms which are currently assigned.
             *
             * For the atoms in assigned, interpretation defines the truth value.
             * For unassigned atoms, the value in interpretation is false.
             * Note: The assigned atoms just hint relevant atoms for effective learning.
             *       External atoms always have to compute the answer according
             *       to the *values in interpretation*, assuming that all atoms are assigned
             *       (those atoms which are not in interpretation are assigned to false).
             * The assigned atoms might be unknown (NULL-pointer).
             */
            InterpretationConstPtr assigned;

            /**
             * \brief Bitset of ground atoms which potentially changed since last query
             *        to the same external atom (cf. Query::eatomID). Atoms not in this set stayed the same.
             *
             * The method is intended to provide information which might be used for
             * better learning and caching techniques in this external source.
             *
             * The changed atoms might be unknown (NULL-pointer).
             */
            InterpretationConstPtr changed;

            /**
             * \brief Input constant vector.
             *
             * For predicate inputs it is the predicate of the ground atoms.  For constant
             * inputs it is the input constant. (Variables in input tuples in the program are
             * grounded via auxiliary predicates, so input never contains variable terms.)
             *
             * If &extatom[a,b](y,Z) the input tuple is <a,b>.
             */
            Tuple input;

            /**
             * \brief Output term vector.
             *
             * The vector of output terms of the external atom.  This vector might contain
             * variables, if variables occur as output terms in the program.  Several of
             * these variables might even be the same, e.g., if the external atom is
             * &extatom[a,b](X,c,X,d) the pattern is <X,c,X,d>.
             */
            Tuple pattern;

            /** ID of the external atom to be evaluated */
            ID eatomID;

            /** Set of all input atoms to this external atom */
            InterpretationPtr predicateInputMask;

            /**
             * \brief Construct query.
             * @param interpretation Set of all true input atoms to external atom.
             * @param input Input tuple.
             * @param patter Output pattern.
             * @param eatomID The external atom for which the query is to be answered; can be ID_FAIL if unknown.
             * @param predicateInputMask Set of all (true and false) input atoms to the external source.
             * @param assigned Set of atoms which are currently assigned; if it is a NULL-pointer, then the assignment is complete.
             * @param changed Set of atoms which have possibly changed since last call for the same eatomID (can be used to optimize query answering); can be unknown.
             */
            Query(const ProgramCtx* ctx,
                InterpretationConstPtr interpretation,
                const Tuple& input,
                const Tuple& pattern,
                ID eatomID = ID_FAIL,
                const InterpretationPtr predicateInputMask = InterpretationPtr(),
                const InterpretationConstPtr assigned = InterpretationConstPtr(),
                const InterpretationConstPtr changed = InterpretationConstPtr()):
            ctx(ctx),
                interpretation(interpretation),
                input(input),
                pattern(pattern),
                eatomID(eatomID),
                predicateInputMask(predicateInputMask),
                assigned(assigned),
            changed(changed) {
            }
            /**
             * \brief Assignes the query in depth (copy internal data structures rather than share).
             * @param q2 Query to copy. */
            void assign(const Query& q2);
            /**
             * Equality for hashing the query for caching query results.
             */
            bool operator==(const Query& other) const;
        };

        /**
         * \brief Output of an external atom call.
         *
         * Answer objects are created in external computations, i.e.,
         * in PluginAtom::retrieve or PluginAtom::retrieveCached.
         *
         * The storage of tuples is accessible only via get() such that the object can be
         * copied with low cost (only shared pointer is copied) and such that the Answer
         * cache can be implemented efficiently.
         */
        struct DLVHEX_EXPORT Answer
        {
            /**
             * \brief Constructor.
             */
            Answer();

            /**
             * \brief Access true storage (read/write) and mark answer as used.
             */
            std::vector<Tuple>& get() { used = true; return *output; }

            /**
             * \brief Access true storage (read only). Do NOT mark as used.
             */
            const std::vector<Tuple>& get() const { return *output; }

            /**
             * \brief Access unknown storage (read/write) and mark answer as used.
             */
            std::vector<Tuple>& getUnknown() { used = true; return *unknown; }

            /**
             * \brief Access unknown storage (read only). Do NOT mark as used.
             */
            const std::vector<Tuple>& getUnknown() const { return *unknown; }

            /**
             * \brief Usage report (for cache).
             * @return True if the answer object has been used, otherwise false.
             */
            bool hasBeenUsed() const { return used; }

            /**
             * \brief Mark as used (in case you do not add tuples).
             *
             * Call this method in your PluginAtom::retrieve implementation if you do not
             * return tuples. (Otherwise dlvhex will complain.)
             */
            void use() { used = true; }

            /**
             * \brief Assignment (marks as used).
             * @param other Answer to assign.
             * @return Reference to this object.
             */
            Answer& operator=(const Answer& other)
                { output = other.output; unknown = other.unknown; used = true; return *this; }

            /**
             * \brief Comparison non-implementation (produces linker error on purpose).
             *
             * Rationale: shallow comparison may yield wrong results, deep comparison is
             * inefficient and should (at the moment) never be necessary, as answer tuples
             * are immediately integrated into ordinary ground atoms.
             * @param other Answer to compare to.
             * @return True if this object is equivalent to \p other.
             */
            bool operator==(const Answer& other) const;

            private:
                // \brief stores the positive output values; shared_ptr storage for have low-cost-copying of this object and for a more efficient query answer cache implementation.
                boost::shared_ptr<std::vector<Tuple> > output;
                // \brief stores the unknown output values (those neither in output nor in unknown are false); shared_ptr storage for have low-cost-copying of this object and for a more efficient query answer cache implementation.
                boost::shared_ptr<std::vector<Tuple> > unknown;
                // usage marker: true if this was default-constructed and never used
                bool used;
        };

        /**
         * \brief Type of input parameter.
         *
         * Three types of input parameters can be specified: PREDICATE, CONSTANT, and TUPLE.
         *
         * Specifying the input parameters' types is necessary for reducing the
         * interpretation that will be passed to the external atom as well as
         * improving the dependency information used by the internal
         * evaluation strategies of dlvhex.
         */
        typedef enum {
            /** \brief An input argument of type PREDICATE means that the atom needs those facts
             *        of the interpretation whose predicate match the value of this argument. */
            PREDICATE,
            /** \brief An input argument of type CONSTANT means that only its value is relevent
             *        to the external atom, regardless of the interpretation. */
            CONSTANT,
            /** \brief An input argument of type TUPLE may be specified as last input type only, it
             *        means that an unspecified number of CONSTANT values may be specified after
             *        other inputs.
             */
            TUPLE
        } InputType;

    protected:
        /**
         * \brief Constructor.
         *
         * Call this constructor when creating custom external atoms.
         *
         * \param predicate
         *   The name of the external atom as it appears in the HEX program
         *   must be specified here.
         *   As it is useful to have a tight connection of each PluginAtom
         *   with its name, every plugin atom can have only one name.
         * \para monotonic
         *   This boolean indicates whether the atom is monotonic or not.
         *   Specifying false is always allowed, but might degrade evaluation
         *   performance in cases where a fixed point calculation could be used
         *   instead of guess and check model building.
         *
         * Within the derived constructor,
         * - you may define inputs using PluginAtom::addInput..., and
         * - you must call setOutputArity().
         */
        PluginAtom(const std::string& predicate, bool monotonic):
        predicate(predicate),
        allmonotonic(monotonic) {
            prop.pa = this;
        }

        // The following functions are to be used in the constructor only.

        /**
         * \brief Adds an input parameter of type PREDICATE.
         *
         * @param nameIsRelevant If true, the name of the predicate parameter might influence the result,
         *                       otherwise only its extension is relevant.
         *
         * Only use in constructor!
         */
        void addInputPredicate(bool nameIsRelevant = false);

        /**
         * \brief Adds an input parameter of type CONSTANT.
         *
         * Only use in constructor!
         */
        void addInputConstant();

        /**
         * \brief Adds an input parameter of type TUPLE.
         *
         * Only use in constructor!
         */
        void addInputTuple();

        /**
         * \brief Specifies the output arity of the external Atom.
         *
         * Only use in constructor!
         *
         * Use prop.setVariableOutputArity(true) to define an external atom with variable output arity.
         *
         * @param arity The desired output arity >= 0.
         */
        void setOutputArity(unsigned arity);

    public:
        /**
         * \brief Destructor.
         */
        virtual ~PluginAtom() {}

        /**
         * \brief Get input arity.
         * \return int Input arity, where a tuple parameter is counted once.
         */
        int getInputArity() const;

        /**
         * \brief Get output arity.
         * \return int Output arity.
         */
        int getOutputArity() const;

        /**
         * \brief Checks the input arity of the external atom against the
         * specified arity.
         *
         * The input arity follows from the number of specified predicates and might be variable.
         *
         * \returns True if arity matches and false otherwise.
         */
        bool checkInputArity(unsigned arity) const;

        /**
         * \brief Checks whether the output arity of the external atom is
         * compatible with the specified arity (also under consideration of prop.hasVariableOutputArity()).
         * @return True if the check is passed and false otherwise.
         */
        bool checkOutputArity(const ExtSourceProperties& prop, unsigned arity) const;

        /**
         * \brief Can be used to customize the generic properties of an external source for a particular external atom.
         *
         * The function is called once for each external atom in order to finalize the initialization of its properties.
         * While the properties in class PluginAtom apply to all external atoms of the type,
         * this method can modify the properties for selected external atoms.
         * It may inspect the input list and set additional external atom properties,
         * which do not hold in general but only for a certain usage of the external source.
         *
         * Example: Let \code{.txt}&sql[r1, ..., rn, query](X1, ..., Xm)\endcode be an SQL-query processor
         *          over relations r1, ..., rn.
         *          Then &sql is in general not monotonic, but if query is a simple
         *          selection of all tuples, then it becomes monotonic.
         *
         * When this method is called, \p eatom.prop already contains the properties
         * derived from the plugin and specified directly in the HEX-program (using syntax <...>).
         * This method may modify them or add additional properties.
         *
         * @param eatom Reference to the external atom whose properties are to be initialized.
         */
        virtual void setupProperties(const ExternalAtom& eatom) {}

        /**
         * \brief Decides for a support set instance whether to keep or drop it.
         *
         * Decides for a ground support set if it should be kept (in possibly modified form) or rejected.
         * This allows for checking the satisfaction of guard literals in the nogood, which depend on the external source.
         * @param keep Must be set to true if ng should be kept.
         * @param ng The ground nogood, for which the method calls must decide if it should be kept.
         *           If \p keep is true, then ng might be modified such that the resulting nogood is a subset of the original one.
         *           If \p keep is false, then ng must be unchanged.
         * @param eaReplacement The ID of the external atom replacement in ng (just for user convenience)
         */
        virtual void guardSupportSet(bool& keep, Nogood& ng, const ID eaReplacement) { assert(ng.isGround()); keep = true; }

        /**
         * \brief Single interface to answer an external query. Will delegate the call to the other retrieve functions depending on the settings.
         *
         * The function performs the following operations.
         * 1. It splits the query into atomic queries using PluginAtom::splitQuery.
         * 2. It answers the atomic queries by calling either PluginAtom::retrieveCached or PluginAtom::retrieve, depending on the settings.
         * 3. It applies default learning methods to the result (e.g. learning from IO behavior, monotonicity, functionality, etc.).
         *
         * This method must not be overridden.
         *
         * @param query Input to the external source.
         * @param answer Output of the external source.
         * @param nogoods Here, nogoods learned from the external source can be added to prune the search space; see Nogood, NogoodContainer and ExternalLearningHelper.
         * @param useCache True to use the cache (if possible), false to answer the query directly.
         * 
         * @return True if the query was answered from cache and false otherwise.
         */
        bool retrieveFacade(const Query& query, Answer& answer, NogoodContainerPtr nogoods, bool useCache);

        /**
         * \brief Retrieve answer object according to a query by using the cache if possible.
         *
         * This function implements the query cache, if enabled, and will rarely need to be
         * overridden. If the query cannot be answered from the cache, it will be forwarded
         * to the PluginAtom::retrieve method.
         *
         * @param query Input to the external source.
         * @param answer Output of the external source.
         * @param nogoods Here, nogoods learned from the external source can be added to prune the search space; see Nogood, NogoodContainer and ExternalLearningHelper.
         *
         * @return True if the query was answered from cache and false otherwise.
         */
        virtual bool retrieveCached(const Query& query, Answer& answer, NogoodContainerPtr nogoods);

        /**
         * \brief Retrieve answer to a query (external computation happens here).
         *
         * This function implements the external atom computation with customized learning functions.
         *
         * Answer tuples must conform to the content of the pattern tuple in Query:
         * - they must contain the same number of terms as pattern
         * - constants in pattern must match constants in answer tuples
         * - variables in pattern must be replaced by constants in answer tuples
         *
         * See also documentation of Query and Answer classes.
         *
         * If both
         * retrieve(const Query& query, Answer& answer)
         * and retrieve(const Query& query, Answer& answer, NogoodContainerPtr nogoods)
         * are overridden, the latter takes over precedence.
         *
         * @param query Input to the external source.
         * @param answer Output of the external source.
         * @param nogoods Here, nogoods learned from the external source can be added to prune the search space; see Nogood, NogoodContainer and ExternalLearningHelper.
         */
        virtual void retrieve(const Query& query, Answer& answer, NogoodContainerPtr nogoods);

        /**
         * \brief Retrieve answer to a query (external computation happens here).
         *
         * This function implements the external atom computation without customized learning functions.
         *
         * Answer tuples must conform to the content of the pattern tuple in Query:
         * - they must contain the same number of terms as pattern
         * - constants in pattern must match constants in answer tuples
         * - variables in pattern must be replaced by constants in answer tuples
         *
         * See also documentation of Query and Answer classes.
         *
         * If both
         * retrieve(const Query& query, Answer& answer)
         * and retrieve(const Query& query, Answer& answer, NogoodContainerPtr nogoods)
         * are overridden, the latter takes over precedence.
         *
         * @param query Input to the external source.
         * @param answer Output of the external source.
         */
        virtual void retrieve(const Query& query, Answer& answer);

        /**
         * \brief Is called for learning support sets. Needs to be implemented if PluginAtom::prop declares
         * that the external source provides support sets.
         *
         * This function implements the learning of support sets from external atoms.
         * The learned support sets should be added to \nogoods.
         *
         * @param query Input to the external source.
         * @param nogoods Support sets learned from the external source.
         */
        virtual void learnSupportSets(const Query& query, NogoodContainerPtr nogoods);

        /**
         * \brief Tries to generalize learned nogoods to nonground nogoods. Should only be overridden by experienced users.
         *
         * @param ng A learned nogood with some external atom auxiliary over this external predicate.
         * @param ctx Program context.
         * @param nogoods The nogood container where related nogoods shall be added.
         */
        virtual void generalizeNogood(Nogood ng, ProgramCtx* ctx, NogoodContainerPtr nogoods);

        /**
         * \brief Splits a non-atomic query up into a set of atomic queries, such that the result of the
         *        composed query corresponds to the union of the results to the atomic queries.
         *        Should only be overridden by experienced users.
         *        By default, queries will be split according to the linearity properties defined in \p
         *        (if an external source does not have such properties, then the query is passed as is to one of the
         *        retrieve methods).
         *        By overriding the method, customized decomposition strategies may be implemented.
         * @param query The original query to split up.
         * @param prop External source properties.
         * @return A set of subqueries to be sent to one of the retrieve methods.
         */
        virtual std::vector<Query> splitQuery(const Query& query, const ExtSourceProperties& prop);

        /**
         * \brief Returns the type of the input argument specified by position.
         *
         * Should not be overridden.
         *
         * Indexing starts with 0.
         * Returns TUPLE for TUPLE input arguments (variably many).
         * @return Type of input parameter with index \p index
         */
        InputType getInputType(unsigned index) const;

        /**
         * \brief Return vector of input types.
         *
         * Should not be overridden.
         *
         * TUPLE may be returned as last type.
         * @return Vector of input types
         */
        const std::vector<InputType>& getInputTypes() const
            { return inputType; }

        /**
         * \brief Returns if this external atom provides a custom model generator factory.
         *
         * @return True if this plugin provides a custom heuristics for external atom evaluations, false otherwise.
         */
        virtual bool providesCustomExternalAtomEvaluationHeuristicsFactory() const { return false; }

        /**
         * \brief Must create a model generator factory for the component described by ci.
         * Needs to be implemented only if PluginAtom::providesCustomExternalAtomEvaluationHeuristicsFactory return true;
         * @return True if this plugin provides a custom heuristics for external atom evaluations, false otherwise.
         */
        virtual ExternalAtomEvaluationHeuristicsFactoryPtr getCustomExternalAtomEvaluationHeuristicsFactory() const
            { assert(false && "This plugin atom does not provide a custom external atom evaluation heuristics factory"); return ExternalAtomEvaluationHeuristicsFactoryPtr(); }

        /**
         * @return External source properties associated with this plugin atom (applies to all external atoms of this kind, unless they override the properties).
         */
        const ExtSourceProperties& getExtSourceProperties() const
        {
            return prop;
        }

        // Associate plugin atom with registry pointer.
        // (This implicitly calculates the predicate ID.)

        /**
         * \brief Associate plugin atom with registry pointer.
         *
         * Should not be overridden.
         *
         * This implicitly calculates the predicate ID.
         * If overridden, the original method must be called.
         *
         * As PluginAtom internally stores IDs, it has to be associated with a fixed
         * Registry for its lifetime. Create several instantiations if you need the same
         * atom in several registries.
         *
         * PluginContainer calls this method automatically when loading a plugin and its
         * atoms.
         *
         * @param reg Pointer to the registry to set.
         */
        virtual void setRegistry(RegistryPtr reg);

        /**
         * \brief Get Registry associcated with atom.
         *
         * Should not be overridden.
         *
         * As PluginAtom internally stores IDs, it has to be associated with a fixed
         * Registry for its lifetime. Create several instantiations if you need the same
         * atom in several registries.
         *
         * @return Pointer to the registry used by this plugin.
         */
        RegistryPtr getRegistry() const
            { return registry; }

        /**
         * \brief Get ID of the predicate name.
         *
         * Should not be overridden.
         *
         * @return ID of the predicate name specified in the constructor or ID_FAIL if no registry is set.
         */
        ID getPredicateID() const
            { return predicateID; }

        /**
         * \brief Get predicate name (as specified in constructor).
         *
         * Should not be overridden.
         *
         * @return Predicate name specified in the constructor (without leading &)
         */
        const std::string& getPredicate() const
            { return predicate; }

        /** \brief Returns a mask of all positive replacement atoms which are currently in the registry and match with this PluginAtom. */
        PredicateMaskPtr getReplacements(){ replacements->updateMask(); return replacements; }

        /**
         * \brief Erase all elements from queryAnswerNogoodCache.
         */
        void resetCache() {
            queryAnswerNogoodCache.clear();
        }

    protected:
        // \brief Predicate of the atom as it appears in HEX programs (without leading &)
        //
        // This is not stored as ID, because plugins must be allowed to create
        // atoms without knowing the registry they will be used with.
        //
        // Calling setRegistry provides the predicate ID.
        std::string predicate;

        /** \brief ID of the predicate name, ID_FAIL if no registry is set. */
        ID predicateID;

        /** \brief Whether the function is monotonic in all parameters (this will automatically declare all predicate input parameters as monotonic, see PluginAtom::prop). */
        bool allmonotonic;

        /** \brief General properties of the external source (may be overridden on atom-level). */
        ExtSourceProperties prop;

        /** \brief Type of each input argument (only last may be TUPLE). */
        std::vector<InputType> inputType;

        /** \brief Number of output arguments. */
        unsigned outputSize;

        // Query/Answer cache
        /** \brief Type used for associating an Answer and a NogoodContainer to a Query. */
        typedef boost::unordered_map<const Query, std::pair<Answer, SimpleNogoodContainerPtr> > QueryAnswerNogoodCache;
        /** \brief Type used for associating a container of learned nogoods to a Query. */
        typedef boost::unordered_map<const Query, SimpleNogoodContainerPtr> QueryNogoodCache;
        /** \brief Associates an Answer and a NogoodContainer to a Query. */
        QueryAnswerNogoodCache queryAnswerNogoodCache;
        /** \brief Mutex for accessing PluginAtom::queryAnswerNogoodCache. */
        boost::mutex cacheMutex;

        /** \brief Mask of all positive replacement atoms of this external atom. */
        PredicateMaskPtr replacements;

        /** \brief Output tuples generated so far (used for learning for functional sources). */
        std::vector<Tuple> otuples;

        /** \brief Registry associated with this atom.
         *
         * This association cannot be done by the plugin itself, it is done by
         * the PluginContainer loading or receiving the plugin.
         */
        RegistryPtr registry;

    private:
        /**
         * \brief Copy-constructor of the plugin atom.
         * @param pa Other plugin atom to copy.
         */
        PluginAtom(const PluginAtom& pa){}

        /**
         * \brief Assignment operator.
         * @param pa Other plugin atom to assign.
         * @return Reference to this object.
         */
        const PluginAtom& operator=(const PluginAtom& pa){ return *this; }
};

typedef boost::shared_ptr<PluginAtom> PluginAtomPtr;
typedef boost::weak_ptr<PluginAtom> PluginAtomWeakPtr;
// hash function for QueryAnswerCache
std::size_t hash_value(const PluginAtom::Query& q);

/**
 * \brief Converter class (input stream rewriter).
 *
 * A converter is a rewriter for the raw input stream.
 * A converter must return a parseable HEX-program in its output stream.
 * Note, that the definition of parseable depends on whether your plugin
 * also replaces or extends the HEX input parser.
 *
 * Other possibilities for modifying the way input is prepared for
 * evaluation are:
 * - PluginInterface::createRewriter (rewrite the parsed program),
 * - PluginInterface::createOptimizer (rewrites the dependency graph and
 *   the EDB), and
 * - PluginInterface::createParser and PluginInterface::createParserModules.
 */
class DLVHEX_EXPORT PluginConverter
{
    public:
        virtual ~PluginConverter() { }

        /**
         * Conversion function.
         *
         * The input program is read from \p i. The output must be passed on to the
         * stream \p o, either the original input stream or the result of a conversion.
         * @param i Input stream (concatenation of all input files and stdin).
         * @param o Output stream which is expected to contain a HEX-program when this method returns.
         */
        virtual void convert(std::istream& i, std::ostream& o) = 0;
};
typedef boost::shared_ptr<PluginConverter> PluginConverterPtr;

/**
 * \brief Callback functor for processing complete models of the HEX program.
 *
 * Setup such callbacks in PluginInterface::setupProgramCtx
 * Register such callback in ProgramCtx::modelCallbacks
 * A callback can abort the model building process.
 *
 * Use this, e.g., for accumulating data over all models.
 * (For an example see QueryPlugin.cpp.)
 */
class ModelCallback:
public std::unary_function<bool, AnswerSetPtr>
{
    public:
        virtual ~ModelCallback() {}
        /**
         * \brief Method called for each complete model of the program.
         *
         * @param as Pointer to the current answer set.
         * @return True continues the model generation process, false stops the model generation process.
         */
        virtual bool operator()(AnswerSetPtr as) = 0;
};
typedef boost::shared_ptr<ModelCallback> ModelCallbackPtr;

/**
 * \brief Callback functor after model enumeration finished or aborted.
 *
 * Setup such callbacks in PluginInterface::setupProgramCtx
 * Register such callbacsk in ProgramCtx::finalCallbacks
 *
 * Use this, e.g., to output data accumulated over all models.
 * (For an example see QueryPlugin.cpp.)
 */
class FinalCallback
{
    public:
        virtual ~FinalCallback() {}
        /**
         * \brief Method called after model enumeration finished or aborted.
         */
        virtual void operator()() = 0;
};
typedef boost::shared_ptr<FinalCallback> FinalCallbackPtr;

/**
 * \brief Rewriter class.
 *
 * The purpose of a plugin rewriter is to modify the parsed HEX program and
 * EDB, before the dependency graph is constructed. This can, e.g., be used
 * to implement partial pregrounding. (Note that HEX processes nonground
 * programs.)
 *
 * Other possibilities for modifying the way input is prepared for
 * evaluation are:
 * - PluginInterface::createConverter (rewrite the raw input stream),
 * - PluginInterface::createOptimizer (rewrites the dependency graph and
 *   the EDB), and
 * - PluginInterface::createParser and PluginInterface::createParserModules.
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
         * @param ctx The current program context
         *
         * Especially ctx.edb and ctx.idb may be the subject of rewriting.
         */
        virtual void rewrite(ProgramCtx& ctx) = 0;
};
typedef boost::shared_ptr<PluginRewriter> PluginRewriterPtr;

/**
 * \brief Optimizer class.
 *
 * The purpose of an optimizer is to modify (prune) the dependency graph
 * and the EDB before evaluation.
 *
 * Other possibilities for modifying the way input is prepared for
 * evaluation are:
 * - PluginInterface::createRewriter (rewrite the parsed program),
 * - PluginInterface::createConverter (rewrite the raw input stream),
 * - PluginInterface::createParser and PluginInterface::createParserModules.
 */
class DLVHEX_EXPORT PluginOptimizer
{
    public:
        virtual ~PluginOptimizer() {}

        /**
         * \brief Optimize EDB and dependency graph.
         * @param edb Set of facts of the program
         * @param depgraph Current dependency graph which can be modified to optimize it
         */
        virtual void optimize(InterpretationPtr edb, DependencyGraphPtr depgraph) = 0;
};

//
// deleters
//

/**
 * \brief Special library deleter to be used with boost smart pointers.
 *
 * This class should be used as a "deleter" for boost::shared_ptr if the library
 * returning the pointer wants to return an object allocated with "new"
 * this must stay inline! (the correct delete operator must be used by the library)
 *
 * You will not need to care about this unless you get segmentation faults
 * when dlvhex shuts down.
 */
template<typename AllocatedT>
struct PluginPtrDeleter
{
    PluginPtrDeleter() {}
    /**
     * \brief Deleter for plugins
     * @param ptr Pointer to the plugin
     */
    inline void operator()(const AllocatedT* ptr) const
    {
        delete ptr;
    }
};

/**
 * \brief Special library deleter to be used with boost smart pointers.
 *
 * This class should be used as a "deleter" for boost::shared_ptr if the library
 * returning the pointer wants to return a statically linked object (i.e., do not free)
 * [this is used for all PluginInterface pointers, as this pointer is returned as a
 * POD type via an extern "C" function]
 *
 * You will not need to care about this unless you get segmentation faults
 * when dlvhex shuts down.
 */
template<typename AllocatedT>
struct PluginPtrNOPDeleter
{
    PluginPtrNOPDeleter() {}
    /**
     * \brief Deleter for plugins without actual functionality
     * @param ptr Pointer to the plugin
     */
    inline void operator()(const AllocatedT* ptr) const
    {
        // NOP = do not delete
    }
};

DLVHEX_NAMESPACE_END
#endif                           // _DLVHEX_PLUGININTERFACE_H


// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:

