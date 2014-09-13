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
 * @file PythonPlugin.cpp
 * @author Christoph Redl
 *
 * @brief Supports Python-implemented plugins.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef HAVE_PYTHON

//#include <Python.h>
//#include <structmember.h>

#include <boost/python.hpp>

//#define BOOST_SPIRIT_DEBUG

#include "dlvhex2/PythonPlugin.h"
#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/Printhelpers.h"
#include "dlvhex2/PredicateMask.h"
#include "dlvhex2/Logger.h"
#include "dlvhex2/HexParser.h"
#include "dlvhex2/HexParserModule.h"
#include "dlvhex2/HexGrammar.h"
#include "dlvhex2/ExternalLearningHelper.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>

#ifdef WIN32
#include <Windows.h>
#endif

DLVHEX_NAMESPACE_BEGIN

#ifdef POSIX
	#define setenv(VAR, VAL, V) ::setenv(VAR, VAL, V)
	#define unsetenv(VAR) ::unsetenv(VAR)
#else
	void setenv(const char* var, const char* val, int v){
		SetEnvironmentVariable(var, val);
	}

	void unsetenv(const char* var){
		SetEnvironmentVariable(var, NULL);
	}
#endif

PythonPlugin::CtxData::CtxData()
{
}

PythonPlugin::PythonPlugin():
	PluginInterface()
{
	setNameVersion("dlvhex-pythonplugin[internal]", 2, 0, 0);
}

PythonPlugin::~PythonPlugin()
{
}

// output help message for this plugin
void PythonPlugin::printUsage(std::ostream& o) const
{
  //    123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-
	o << "     --pythonplugin=[PATH]    Add Python script \"PATH\" as new plugin." << std::endl;
}

// accepted options: --pythonplugin=[PATH]
//
// processes options for this plugin, and removes recognized options from pluginOptions
// (do not free the pointers, the const char* directly come from argv)
void PythonPlugin::processOptions(
		std::list<const char*>& pluginOptions,
		ProgramCtx& ctx)
{
	PythonPlugin::CtxData& ctxdata = ctx.getPluginData<PythonPlugin>();

	typedef std::list<const char*>::iterator Iterator;
	Iterator it;
	WARNING("create (or reuse, maybe from potassco?) cmdline option processing facility")
	it = pluginOptions.begin();
	while( it != pluginOptions.end() )
	{
		bool processed = false;
		const std::string str(*it);
		if( boost::starts_with(str, "--pythonplugin=") )
		{
			ctxdata.pythonScripts.push_back(str.substr(std::string("--pythonplugin=").length()));
			processed = true;
		}

		if( processed )
		{
			// return value of erase: element after it, maybe end()
			DBGLOG(DBG,"PythonPlugin successfully processed option " << str);
			it = pluginOptions.erase(it);
		}
		else
		{
			it++;
		}
	}
}

namespace
{

typedef PythonPlugin::CtxData CtxData;

class PythonRewriter:
	public PluginRewriter
{
private:
	PythonPlugin::CtxData& ctxdata;
public:
	PythonRewriter(PythonPlugin::CtxData& ctxdata) : ctxdata(ctxdata) {}
	virtual ~PythonRewriter() {}

	virtual void rewrite(ProgramCtx& ctx);
};

void PythonRewriter::rewrite(ProgramCtx& ctx)
{
}

} // anonymous namespace

// rewrite program
PluginRewriterPtr PythonPlugin::createRewriter(ProgramCtx& ctx)
{
	PythonPlugin::CtxData& ctxdata = ctx.getPluginData<PythonPlugin>();
	return PluginRewriterPtr(new PythonRewriter(ctxdata));
}

// register auxiliary printer for strong negation auxiliaries
void PythonPlugin::setupProgramCtx(ProgramCtx& ctx)
{
	PythonPlugin::CtxData& ctxdata = ctx.getPluginData<PythonPlugin>();
	RegistryPtr reg = ctx.registry();
}

namespace{

inline long IDToLong(ID id){
	long l = ((long)id.kind << 32 ) | ((long)id.address);
	DBGLOG(DBG, "Stored ID " << id << " as " << l);
	return l;
}

inline ID longToID(long l){
	ID id;
	id.kind = (l >> 32) & 0xFFFFFFFF;
	id.address = l & 0xFFFFFFFF;
	return id;
}

}

namespace PythonAPI{
	ProgramCtx* emb_ctx;
	const PluginAtom::Query* emb_query;
	PluginAtom::Answer* emb_answer;
	NogoodContainerPtr emb_nogoods;
	std::vector<PluginAtomPtr> *emb_pluginAtoms;
	boost::python::object main;
	boost::python::object dict;
}

class PythonAtom : public PluginAtom
{
	public:
		PythonAtom(ProgramCtx& ctx, std::string module, std::string functionName, std::vector<InputType> inputParameters, int outputArity)
			: PluginAtom(functionName.c_str(), false)
		{
			BOOST_FOREACH (InputType type, inputParameters){
				switch (type){
					case CONSTANT: addInputConstant(); break;
					case PREDICATE: addInputPredicate(); break;
					case TUPLE: addInputTuple(); break;
				}
			}		
			setOutputArity(outputArity);
		}

		virtual void
		retrieve(const Query& query, Answer& answer) throw (PluginError)
		{
			assert (false);
		}

		virtual void
		retrieve(const Query& query, Answer& answer, NogoodContainerPtr nogoods) throw (PluginError)
		{
			try{
				PythonAPI::emb_query = &query;
				PythonAPI::emb_answer = &answer;
				PythonAPI::emb_nogoods = nogoods;

				boost::python::tuple t;
				BOOST_FOREACH (ID inp, query.input) t += boost::python::make_tuple(inp);
				PythonAPI::main.attr(getPredicate().c_str())(t);

				PythonAPI::emb_query = NULL;
				PythonAPI::emb_answer = NULL;
				PythonAPI::emb_nogoods.reset();
			}catch(boost::python::error_already_set& e){
				PyErr_Print();
			}
		}
};

namespace PythonAPI{

void addAtom(std::string name, boost::python::tuple args, int outputArity){
	if (!emb_pluginAtoms) throw PluginError("Cannot create external atoms at this point");
	std::vector<PluginAtom::InputType> inputParameters;
	for (int p = 0; p < boost::python::len(args); ++p){
		std::string arg = boost::python::extract<std::string>(args[p]);
		if (arg == std::string("c")) inputParameters.push_back(PluginAtom::CONSTANT);
		else if (arg == std::string("p")) inputParameters.push_back(PluginAtom::PREDICATE);
		else if (arg == std::string("t")) inputParameters.push_back(PluginAtom::TUPLE);
		else throw PluginError("Unknown parameter type: \"" + arg+ "\"");
	}

	// return smart pointer with deleter (i.e., delete code compiled into this plugin)
	emb_pluginAtoms->push_back(dlvhex::PluginAtomPtr(new PythonAtom(*emb_ctx, "unknown", name, inputParameters, outputArity), PluginPtrDeleter<PluginAtom>()));
}

boost::python::tuple getTuple(ID id) {
	if (!id.isAtom() && !id.isLiteral()) throw PluginError("dlvhex.getTuple: Parameter must an atom or literal ID");
	const OrdinaryAtom& ogatom = emb_ctx->registry()->lookupOrdinaryAtom(id);
	boost::python::tuple t;
	BOOST_FOREACH (ID term, ogatom.tuple) t += boost::python::make_tuple(term);
	return t;
}

std::string getValue(ID id){
	std::stringstream ss;
	RawPrinter printer(ss, emb_ctx->registry());
	printer.print(id);
	std::string str = ss.str();
	return ss.str();
}

int getIntValue(ID id){
	if (!id.isTerm() || !id.isIntegerTerm()) throw PluginError("dlvhex.getIntValue: given value does not represent an integer");
	return id.address;
}

boost::python::tuple getTupleValues(ID id) {
	if (!id.isAtom() && !id.isLiteral()) throw PluginError("dlvhex.getTuple: Parameter must an atom or literal ID");
	const OrdinaryAtom& ogatom = emb_ctx->registry()->lookupOrdinaryAtom(id);
	boost::python::tuple t;
	BOOST_FOREACH (ID term, ogatom.tuple) t += boost::python::make_tuple(getValue(term));
	return t;
}

ID storeInteger(int i){
	return ID::termFromInteger(i);
}

ID storeString(std::string str){
	return emb_ctx->registry()->storeConstantTerm(str);
}

ID storeAtom(boost::python::tuple args) {
	OrdinaryAtom atom(dlvhex::ID::MAINKIND_ATOM | dlvhex::ID::SUBKIND_ATOM_ORDINARYG);
	for (int i = 0; i < boost::python::len(args); ++i){
		atom.tuple.push_back(boost::python::extract<ID>(args[i]));
	}

	return emb_ctx->registry()->storeOrdinaryGAtom(atom);
}

ID negate(ID id) {
	if (!id.isLiteral()) throw PluginError("dlvhex.negate: Can only negate literal IDs");
	id.kind ^= dlvhex::ID::NAF_MASK;
	return id;
}

bool learn(boost::python::tuple args) {

	if (!!emb_nogoods){
		Nogood ng;
		for (int i = 0; i < boost::python::len(args); ++i){
			dlvhex::ID id = boost::python::extract<ID>(args[i]);
			if (!id.isAtom() && !id.isLiteral()) throw PluginError("dlvhex.learn: Parameters must be positive or negated atom IDs");
			ng.insert(NogoodContainer::createLiteral(id));
		}
		emb_nogoods->addNogood(ng);

		return true;
	}else{
		return false;
	}
}

ID getOutputAtom(boost::python::tuple args) {

	Tuple outputTuple;
	for (int i = 0; i < boost::python::len(args); ++i){
		dlvhex::ID id = boost::python::extract<ID>(args[i]);
		if (!id.isTerm()) throw PluginError("dlvhex.output: Parameters must be term IDs");
		outputTuple.push_back(id);
	}
	return ExternalLearningHelper::getOutputAtom(*emb_query, outputTuple, true);
}

void output(boost::python::tuple args) {

	Tuple outputTuple;
	for (int i = 0; i < boost::python::len(args); ++i){
		dlvhex::ID id = boost::python::extract<ID>(args[i]);
		if (!id.isTerm()) throw PluginError("dlvhex.output: Parameters must be term IDs");
		outputTuple.push_back(id);
	}
	emb_answer->get().push_back(outputTuple);
}

void outputValues(boost::python::tuple args) {

	Tuple outputTuple;
	for (int i = 0; i < boost::python::len(args); ++i){
		boost::python::extract<int> get_int(args[i]);
		if (get_int.check()){
			outputTuple.push_back(dlvhex::ID::termFromInteger(get_int()));
		}else{
			// store as string
			outputTuple.push_back(emb_ctx->registry()->storeConstantTerm(boost::python::extract<std::string>(args[i])));
		}
	}
	emb_answer->get().push_back(outputTuple);
}

boost::python::tuple getInputAtoms() {

	bm::bvector<>::enumerator en = emb_query->predicateInputMask->getStorage().first();
	bm::bvector<>::enumerator en_end = emb_query->predicateInputMask->getStorage().end();
	long i = 0;
	boost::python::tuple t;
	while (en < en_end){
		t += boost::python::make_tuple(emb_query->interpretation->getRegistry()->ogatoms.getIDByAddress(*en));
		i++;
		en++;
	}
	return t;
}

int getInputAtomCount() {
	return emb_query->predicateInputMask->getStorage().count();
}

bool isAssigned(ID id) {
	if (!!emb_query->assigned || emb_query->assigned->getFact(id.address)) return true;
	else return false;
}

bool isTrue(ID id) {
	if (emb_query->interpretation->getFact(id.address)) return true;
	else return false;
}

};

BOOST_PYTHON_MODULE(dlvhex) {
	boost::python::def("addAtom", PythonAPI::addAtom);
	boost::python::def("getValue", PythonAPI::getValue);
	boost::python::def("getIntValue", PythonAPI::getIntValue);
	boost::python::def("getTuple", PythonAPI::getTuple);
	boost::python::def("getTupleValues", PythonAPI::getTupleValues);
	boost::python::def("storeInteger", PythonAPI::storeInteger);
	boost::python::def("storeString", PythonAPI::storeString);
	boost::python::def("storeAtom", PythonAPI::storeAtom);
	boost::python::def("negate", PythonAPI::negate);
	boost::python::def("learn", PythonAPI::learn);
	boost::python::def("getOutputAtom", PythonAPI::getOutputAtom);
	boost::python::def("output", PythonAPI::output);
	boost::python::def("outputValues", PythonAPI::outputValues);
	boost::python::def("getInputAtoms", PythonAPI::getInputAtoms);
	boost::python::def("getInputAtomCount", PythonAPI::getInputAtomCount);
	boost::python::def("isAssigned", PythonAPI::isAssigned);
	boost::python::def("isTrue", PythonAPI::isTrue);
	boost::python::class_<dlvhex::ID>("ID");
}

std::vector<PluginAtomPtr> PythonPlugin::createAtoms(ProgramCtx& ctx) const{

	PythonAPI::emb_ctx = &ctx;
	std::vector<PluginAtomPtr> pluginAtoms;

	// we have to do the program rewriting already here because it creates some side information that we need
	PythonPlugin::CtxData& ctxdata = ctx.getPluginData<PythonPlugin>();

	// load Python plugins
	DBGLOG(DBG, "Initialize Python plugin");
	Py_Initialize();
	BOOST_FOREACH (std::string script, ctxdata.pythonScripts){
		DBGLOG(DBG, "Loading file \"" + script + "\"");
		try{
			Py_Initialize();
			initdlvhex();
			PythonAPI::main = boost::python::import("__main__");
			PythonAPI::dict = PythonAPI::main.attr("__dict__");
			boost::python::exec_file(script.c_str(), PythonAPI::dict, PythonAPI::dict);
			PythonAPI::emb_pluginAtoms = &pluginAtoms;
			PythonAPI::main.attr("register")();
			PythonAPI::emb_pluginAtoms = NULL;
		}catch(boost::python::error_already_set& e){
			PyErr_Print();
		}
	}
	DBGLOG(DBG, "Python plugin initialization done");

	return pluginAtoms;
}

DLVHEX_NAMESPACE_END

#endif

// this would be the code to use this plugin as a "real" plugin in a .so file
// but we directly use it in dlvhex.cpp
#if 0
PythonPlugin thePythonPlugin;

// return plain C type s.t. all compilers and linkers will like this code
extern "C"
void * PLUGINIMPORTFUNCTION()
{
	return reinterpret_cast<void*>(& DLVHEX_NAMESPACE thePythonPlugin);
}

#endif
/* vim: set noet sw=2 ts=2 tw=80: */

// Local Variables:
// mode: C++
// End:
