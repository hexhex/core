/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005-2007 Roman Schindlauer
 * Copyright (C) 2006-2015 Thomas Krennwallner
 * Copyright (C) 2009-2015 Peter Schüller
 * Copyright (C) 2011-2015 Christoph Redl
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
 * @file PythonPlugin.h
 * @author Christoph Redl
 *
 * @brief Supports Python-implemented plugins.
 */

/**
 * \defgroup pythonpluginframework The Python Plugin Framework
 *
 * \section introduction Introduction
 *
 * The dlvhex reasoner evaluates Answer %Set Programs with external atoms. One important
 * design principle was to provide a mechanism to easily add further external
 * atoms without having to recompile the main application. A \em plugin is a
 * shared library that provides functions to realise custom external atoms.
 * Furthermore, a plugin can supply rewriting facilities, which may alter the
 * input logic program prior to evaluation.
 * Plugins can be written in Python as described below or in C++ as described
 * \ref pluginframework "here"; the former does not have all features of the C++
 * version but should be sufficient in almost all cases.
 *
 * This Section gives an overview of dlvhex Python plugins and external atoms.
 *
 * \section pyextatom The External Atom Function
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
 * \subsection pyinformationflow Information Flow
 *
 * The interface that is used by dlvhex to access a plugin follows very closely
 * these semantics. For each atom, a retrieval function has to be implemented,
 * which receives a query-object and has to return an an answer-object. The
 * query-object carries the input interpretation as well as the ground input
 * parameters of the external atom call, while the answer object is a container
 * for the output tuples of the external atom's function.
 *
 * \subsection pyinputtypes Types of Input Parameters
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
 * \section writingpythonplugin Writing a Python Plugin
 * We wanted to keep the interface between dlvhex and the plugins as lean as
 * possible.
 * Necessary tasks are:
 * - Write a new Python script which contains a \em register function and imports the package \em dlvhex
 * - Write another function for each external atom, and export this function using the \em register function
 *
 * The \em register function has the following form:
 * \code
 * def register():
 *   dlvhex.addAtom ("concat", (dlvhex.CONSTANT, dlvhex.CONSTANT), 1) )
 * \endcode
 * It adds one \em entry for each external atom. Each \em entry is
 * again a tuple of arity 3: the first element is the external predicate name,
 * the third element is the output arity, and the second is another tuple of input parameter types (dlvhex.CONSTANT, dlvhex.PREDICATE, dlvhex.TUPLE).
 *
 * Each external predicate name (e.g. \em concat) needs to be implemented in form of another Python function with an appropriate number of input parameters.
 *
 * Example:
 * \code
 * def concat(a, b):
 *   dlvhex.outputValues(dlvhex.getValue(a), dlvhex.getValue(b))
 * \endcode
 * The function just takes the values of these parameters and outputs their string concatenation.
 * Here, \em a and \em b are the input parameters (of type constant). If an external atom specifies an input parameter of type TUPLE,
 * the elements will be passed as a Python tuple.
 *
 * Example:
 * \code
 * def concat(tup):
 *   ret = ""
 *   for x in tup:
 *     ret = reg + x
 *   dlvhex.outputValues((ret, ))
 * \endcode
 * Note that akin to the \ref pluginframework "C++ API", terms and atoms are represented by IDs and the retrieval of the value behind
 * usually requires the use of the \em getValue method; some methods combine this with other functionalities (see method list below).
 *
 * In addition to the actual semantics of an external atom, the Python API can also be used for defining custom learning techniques (described in the following list).
 * Advanced plugin features, such as providing converters, rewriters and dependency graph optimizations. are, however, only possible with the \ref pluginframework "C++ API".
 *
 * In more detail, the \em dlvhex Python module provides the methods described in the following paragraphs.
 *
 * <b>Management of dlvhex IDs</b>
 * <ul>
 *   <li>\code{.txt}tuple getTuple(aID)\endcode Return the IDs of the elements of a dlvhex atom identified by ID \em aID.</li>
 *   <li>\code{.txt}tuple getTupleValues(aID)\endcode Return the values of the elements of a dlvhex atom identified by ID \em aID.</li>
 *   <li>\code{.txt}string getValue(id)\endcode Return the value of an atom or term ID \em id.</li>
 *   <li>\code{.txt}int getIntValue(id)\endcode Return the value of an integer term ID \em id as integer.</li>
 *   <li>\code{.txt}string getValue(tup)\endcode Print the tuple \em tup recursively, i.e., the elements of the tuple can be further tuples or IDs. IDs \em id are printed by calling <em>dlvhex.getValue(id)</em>, they are delimited by <em>,</em> and the output is enclosed in curly braces.</li>
 *   <li>\code{.txt}tuple getExtension(id)\endcode Returns all tuples \em tup in the extension of the predicate represented by \em id (wrt. the input interpretation).</li>
 *   <li>\code{.txt}dlvhex.ID storeString(str)\endcode Stores a string \em str as dlvhex object and returns its ID.</li>
 *   <li>\code{.txt}dlvhex.ID storeInteger(int)\endcode Stores an integer \em int as dlvhex object and returns its ID.</li>
 *   <li>\code{.txt}dlvhex.ID storeAtom(args)\endcode Transforms a sequence of terms or values \em args into a dlvhex atom.</li>
 *   <li>\code{.txt}dlvhex.ID negate(aID)\endcode Negates an atom ID \em aID.</li>
 *   <li>\code{.txt}void addAtom(name, args, ar, [prop])\endcode Add external atom \em name with arguments \em args (see above), output arity \em ar and external source properties \em prop ("prop" is optional).</li>
 *   <li>\code{.txt}void storeExternalAtom(pred, input, output)\endcode Stores an external atom with predicate \em pred, input parameters \em input and output parameters \em output (can be terms or their IDs) and returns its ID.</li>
 * </ul>
 *
 * <b>Basic Plugin Functionality</b>
 * <ul>
 *   <li>\code{.txt}void output(args)\endcode Adds a tuple of IDs or values \em args to the external source output.</li>
 *   <li>\code{.txt}ID getExternalAtomID()\endcode Returns the ID of the currently evaluated external atom; the changed information (cf. hasChanged) is relative to the last call for the same external atom</li>
 *   <li>\code{.txt}tuple getInputAtoms([pred])\endcode Returns a tuple of \em all input atoms (\em not only true ones!) to this external atom; \em pred is an optional predicate ID, which allows for restricting the tuple to atoms over this predicate.</li>
 *   <li>\code{.txt}tuple getTrueInputAtoms([pred])\endcode Returns a tuple of all input atoms to this external atom <em>which are currently true</em>; \em pred is an optional predicate ID, which allows for restricting the tuple to atoms over this predicate.</li>
 *   <li>\code{.txt}int getInputAtomCount()\endcode Returns the number of \em input atoms (\em not only true ones!).</li>
 *   <li>\code{.txt}int getTrueInputAtomCount()\endcode Returns the number of input atoms <em>which are currently true</em>.</li>
 *   <li>\code{.txt}bool isInputAtom(id)\endcode Checks if atom \em id belongs to the input of the current external atom.</li>
 *   <li>\code{.txt}bool isTrue(id)\endcode Checks if an input atom identified by ID \em id is assigned to true.</li>
 *   <li>\code{.txt}bool isFalse(id)\endcode Checks if an input atom identified by ID \em id is assigned to false.</li>
 *   <li>\code{.txt}void resetCacheOfPlugins()\endcode Empties all caches of external atom evaluation results and all cached nogoods from external learning.</li>
 * </ul>
 *
 * <b>Conflict-driven Learning</b><br/>
 *
 * Usually, learned nogoods consist of 1. a set of positive or negated input atoms, and 2. a <em>negative</em> output atom, where 1. is the justification for the (positive) output atom to be true.
 * Note that a nogood is a set of atoms which must not be all simultanously true.
 * Therefore, this encodes that whenever all atoms from 1. are true, then the output atom <em>must not be false</em> (this is why the negative output atom is added).
 * <ul>
 *   <li>\code{.txt}bool learn(tup)\endcode Learns a nogood as a tuple of atom IDs or their negations \em tup; returns if learning was enabled.</li>
 *   <li>\code{.txt}ID storeOutputAtom(args, [sign])\endcode Constructs an external atom output atom from IDs or values \em args (for learning purposes) and its sign, where true (default) means positive and false means negative, and returns its ID.</li>
 * </ul>
 * For instance, the nogood <em>{ p(a), -q(a), -&diff[p,q](a) }</em> encodes that whenever the atom <em>p(a)</em> is true and the atom <em>q(a)</em> is false, then the atom &diff[p,q](a) must be true (i.e. must not be false) since constant <em>a</em> will be in the output of the set difference of <em>p</em> and <em>q</em>.
 *
 * <b>Inremental External Query Answering</b>
 * <ul>
 *   <li>\code{.txt}bool isAssignmentComplete()\endcode Returns true if the external source is evaluated over an interpretation which is complete for sure (if it returns false, then the assignment is \em possibly partial but it might still be complete).</li>
 *   <li>\code{.txt}bool isAssigned(id)\endcode Provided that the source declared that it is interested in this property (cf. setCaresAboutAssigned), the method checks if an input atom identified by ID \em id is assigned.</li>
 *   <li>\code{.txt}bool hasChanged(id)\endcode Provided that the source declared that it is interested in this property (cf. setCaresAboutChanged), the method checks if an input atom has \em possibly changes since the last call (if the method returns false, then it has not changed for sure).</li>
 *   <li>\code{.txt}ID storeRule(head, pbody, nbody)\endcode Stores a rule with head atoms \em head, positive body atoms \em pbody and negative body atoms \em nbody and returns its ID; all parameters need to be a tuples of IDs.</li>
 * </ul>
 *
 * <b>Subprogram Evaluation</b>
 * <ul>
 *   <li>\code{.txt}tuple evaluateSubprogram(tup)\endcode Evaluates the subprogram specified by a tuple \code{.txt}facts, rules\endcode consisting of facts \em facts (tuple of IDs of ground atoms) and rules \em rules (tuple of rule IDs) and returns the number of answer sets; the result is a tuple of answer sets, where each answer set is again a tuple of the ground atom IDs which are true in the respective answer set.</li>
 *   <li>\code{.txt}tuple loadSubprogram(filename)\endcode Loads the program stored in file \em filename and returns a pair \code{.txt}(edb, idb)\endcode consisting of a tuple \em edb of facts (ground atom IDs) and a tuple \em idb of rule IDs.</li>
 * </ul>
 *
 * <b>External Source Properties Declaration</b><br/>
 *
 * An instance of <em>dlvhex.ExtSourceProperties</em> can be passed to the <em>addAtom</em> method as last parameter to specify properties of the external atom,
 * which might help the reasoner to speed up evaluation. The structure can be configured using the following methods:
 * <ul>
 *   <li>\code{.txt}void prop.addMonotonicInputPredicate(index)\endcode Declare argument \em index as monotonic predicate parameter.</li>
 *   <li>\code{.txt}void prop.addAntimonotonicInputPredicate(index)\endcode Declare argument \em index as antimonotonic predicate parameter.</li>
 *   <li>\code{.txt}void prop.addPredicateParameterNameIndependence(index)\endcode Declare argument \em index as independent of the predicate name (only its extension is relevant).</li>
 *   <li>\code{.txt}void prop.addFiniteOutputDomain(index)\endcode Declare that output argument \em index has a finite domain.</li>
 *   <li>\code{.txt}void prop.addRelativeFiniteOutputDomain(index1, index2)\endcode Declare that output argument \em index2 has a finite domain wrt. input argument \em index1.</li>
 *   <li>\code{.txt}void prop.setFunctional(value)\endcode Declare the source as functional.</li>
 *   <li>\code{.txt}void prop.setFunctionalStart(index)\endcode Declare the source as functional beginning at term index + 1.</li>
 *   <li>\code{.txt}void prop.setSupportSets(value)\endcode Declare that the source provides support sets.</li>
 *   <li>\code{.txt}void prop.setCompletePositiveSupportSets(value)\endcode Declare that the source provides complete positive support sets.</li>
 *   <li>\code{.txt}void prop.setCompleteNegativeSupportSets(value)\endcode Declare that the source provides complete negative support sets.</li>
 *   <li>\code{.txt}void prop.setVariableOutputArity(value)\endcode Declare that the source has a variable output arity.</li>
 *   <li>\code{.txt}void prop.setCaresAboutAssigned(value)\endcode Declare that the sources wants to know the assigned values.</li>
 *   <li>\code{.txt}void prop.setCaresAboutChanged(value)\endcode Declare that the sources wants to know the values which potentially changed since the previous call.</li>
 *   <li>\code{.txt}void prop.setAtomlevellinear(value)\endcode Declare the source as linear on the atom level.</li>
 *   <li>\code{.txt}void prop.setUsesEnvironment(value)\endcode Declare the source as linear on the tuple level.</li>
 *   <li>\code{.txt}void prop.setFiniteFiber(value)\endcode Declare that the source has a finite fiber.</li>
 *   <li>\code{.txt}void prop.addWellorderingStrlen(index1, index2)\endcode Declare that output argument \em index1 has a string length wellordering wrt. input argument \em index2.</li>
 *   <li>\code{.txt}void prop.addWellorderingNatural(index1, index2)\endcode Declare that output argument \em index1 has a natural wellordering wrt. input argument \em index2.</li>
 * </ul>
 *
 * Moreover, for an ID object \em id, there are the following shortcuts:
 * <ul>
 *   <li><em>id.value()</em> for <em>dlvhex.getValue(id)</em></li>
 *   <li><em>id.extension()</em> for <em>dlvhex.getExtension(id)</em></li>
 *   <li><em>id.intValue()</em> for <em>dlvhex.getIntValue(id)</em></li>
 *   <li><em>id.tuple()</em> for <em>dlvhex.getTuple(id)</em></li>
 *   <li><em>id.tupleValues()</em> for <em>dlvhex.getTupleValues(id)</em></li>
 *   <li><em>id.negate()</em> for <em>dlvhex.negate(id)</em></li>
 *   <li><em>id.isInputAtom()</em> for <em>dlvhex.isInputAtom(id)</em></li>
 *   <li><em>id.isAssigned()</em> for <em>dlvhex.isAssigned(id)</em></li>
 *   <li><em>id.hasChanged()</em> for <em>dlvhex.hasChanged(id)</em></li>
 *   <li><em>id.isTrue()</em> for <em>dlvhex.isTrue(id)</em></li>
 *   <li><em>id.isFalse()</em> for <em>dlvhex.isFalse(id)</em></li>
 * </ul>
 *
 * \section pyusage Using a Python Plugin
 *
 * In order to load a Python-implemented plugin stored in file PATH,
 * pass the additional option \code --pythonplugin=PATH \endcode to dlvhex.
 */
#ifndef PYTHON_PLUGIN__HPP_INCLUDED
#define PYTHON_PLUGIN__HPP_INCLUDED

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif                           // HAVE_CONFIG_H

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/PluginInterface.h"

#include <string>
#include <vector>

#ifdef HAVE_PYTHON

//#include <Python.h>

DLVHEX_NAMESPACE_BEGIN

/** \brief Implements a meta-plugin which allows for loading other plugins implemented in Python. */
class PythonPlugin:
public PluginInterface
{
    public:
        // stored in ProgramCtx, accessed using getPluginData<PythonPlugin>()
        class CtxData:
    public PluginData
    {
        public:
            /** \brief List of Python scripts to load. */
            std::vector<std::string> pythonScripts;
            /** \brief List of commandline arguments to be passed to python. */
            std::vector<std::string> commandlineArguments;

            /** \brief Constructor. */
            CtxData();
            /** \brief Destructor. */
            virtual ~CtxData() {}
    };

    public:
        /** \brief Constructor. */
        PythonPlugin();
        /** \brief Destructor. */
        virtual ~PythonPlugin();

        // output help message for this plugin
        virtual void printUsage(std::ostream& o) const;

        // accepted options: --aggregate-enable
        //
        // processes options for this plugin, and removes recognized options from pluginOptions
        // (do not free the pointers, the const char* directly come from argv)
        virtual void processOptions(std::list<const char*>& pluginOptions, ProgramCtx&);

        // rewrite program: rewrite aggregate atoms to external atoms
        virtual PluginRewriterPtr createRewriter(ProgramCtx&);

        // register model callback which transforms all auxn(p,t1,...,tn) back to p(t1,...,tn)
        virtual void setupProgramCtx(ProgramCtx&);

        virtual std::vector<PluginAtomPtr> createAtoms(ProgramCtx&) const;

        /** \brief Runs the main method from a Python script.
         * @param filename Name of a Python script. */
        void runPythonMain(std::string filename);

        // no atoms!
};

DLVHEX_NAMESPACE_END
#endif
#endif

// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
