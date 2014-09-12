/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010, 2011 Thomas Krennwallner
 * Copyright (C) 2009, 2010, 2011 Peter Schüller
 * Copyright (C) 2011, 2012, 2013 Christoph Redl
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
 * dlvhex evaluates Answer Set Programs with external atoms. One important
 * design principle was to provide a mechanism to easily add further external
 * atoms without having to recompile the main application.
 * With dlvhex version 2.4.0 a \em Python plugin interface was introduced,
 * which supports Python scripts that provide functions to realise custom external atoms.
 *
 * This Section gives an overview of dlvhex Python plugins and external atoms.
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
 *   return ( ("concat", "c", "c", 1), ("isbn", "p", 1), () )
 * \endcode
 * It returns a tuple with one \em entry for each external atom. Each \em entry is
 * again a tuple of arity 2 or greater: the first element is the external predicate name,
 * the last element is the output arity, and in between there is a list of input parameter types ("c" for constant, "p" for predicate, "t" for tuple).
 *
 * Each external predicate name (e.g. \em concat) needs to be implemented in form of another Python function.
 * \code
 * def concat(a, b):
 *   dlvhex.outputValues(dlvhex.getValue(a), dlvhex.getValue(b))
 * \endcode
 * Here, \em a and \em b are the input parameters (of type constant).
 * The function just takes the values of these parameters and outputs their string concatenation.
 * Note that akin to the \ref pluginframework "C++ API", terms and atoms are represented by IDs and the retrieval of the value behind
 *
 * usually requires the use of the \em getValue method; some methods combine this with other functionalities (see method list below).
 *
 * In more detail, the \em dlvhex Python module provides the following methods:
 * \em getTuple: Return the IDs of the elements of a dlvhex atom.
 * \em getTupleValues: Return the values of the elements of a dlvhex atom.
 * \em getValue: Return the value of an atom or term ID).
 * \em storeString: Stores a string as dlvhex object.
 * \em storeInteger: Stores an integer as dlvhex object.
 * \em storeAtom: Transforms a sequence of terms into a dlvhex atom.
 * \em negate: Negates an atom ID.
 * \em learn: Learns a nogood.
 * \em getOutputAtom: Constructs an output atom from term IDs (for learning purposes).
 * \em output: Adds a ground atom represented by an ID to the external source output.
 * \em outputValues: Adds a ground atom to the external source output.
 * \em getInputAtoms: Returns a tuple of all input atoms to this external atom.
 * \em getInputAtomCount: Checks if an input atom is assigned.
 * \em isAssigned: Checks if an input atom is assigned.
 * \em isTrue: Checks if an input atom is assigned to true.
 *
 * In order to load a Python-implemented plugin, make sure that the according Python file \em myPlugin.py is in the plugin
 * search path of \em dlvhex and pass the additional option \code --pythonmodule=myPlugin \endcode (without filename extension .py!).
 */
#ifndef PYTHON_PLUGIN__HPP_INCLUDED
#define PYTHON_PLUGIN__HPP_INCLUDED

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/PluginInterface.h"

#include <string>
#include <vector>

#ifdef HAVE_PYTHON

#include <Python.h>

DLVHEX_NAMESPACE_BEGIN

class PythonPlugin:
  public PluginInterface
{
public:
	// stored in ProgramCtx, accessed using getPluginData<PythonPlugin>()
	class CtxData:
	public PluginData
	{
		public:

		std::vector<std::string> pythonScripts;

		CtxData();
		virtual ~CtxData() {}
	};

public:
	PythonPlugin();
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

	// no atoms!
};

DLVHEX_NAMESPACE_END

#endif

#endif

