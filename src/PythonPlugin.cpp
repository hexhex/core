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

#include <boost/python.hpp>

#include "dlvhex2/PythonPlugin.h"
#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/State.h"
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

#include <cstring>

DLVHEX_NAMESPACE_BEGIN

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
	o << "     --python-plugin=[PATH]" << std::endl
          << "                      Add Python script \"PATH\" as new plugin." << std::endl;
	o << "     --python-main=PATH" << std::endl
          << "                      Call method \"main\" in the specified Python script (with dlvhex support) instead of evaluating a program." << std::endl;
	o << "     --python-arg=ARG  Passes arguments to Python (sys.argv) (can be used multiple times)." << std::endl;
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

	ctx.config.setOption("HavePythonMain", 0);
	while( it != pluginOptions.end() )
	{
		bool processed = false;
		const std::string str(*it);
		if( boost::starts_with(str, "--python-plugin=") )
		{
			ctxdata.pythonScripts.push_back(str.substr(std::string("--python-plugin=").length()));
			processed = true;
		}
		else if( boost::starts_with(str, "--pythonplugin=") )	// option renamed in order to have a consistent naming schema, keep for backwards compatibility
		{
			ctxdata.pythonScripts.push_back(str.substr(std::string("--pythonplugin=").length()));
			processed = true;
		}
		else if( boost::starts_with(str, "--python-main=") )
		{
			ctx.config.setStringOption("PythonMain", str.substr(std::string("--python-main=").length()));
			ctx.config.setOption("HavePythonMain", 1);
			processed = true;
		}
		else if( boost::starts_with(str, "--pythonmain=") )	// option renamed in order to have a consistent naming schema, keep for backwards compatibility
		{
			ctx.config.setStringOption("PythonMain", str.substr(std::string("--pythonmain=").length()));
			ctx.config.setOption("HavePythonMain", 1);
			processed = true;
		}
		else if( boost::starts_with(str, "--python-arg=") )
		{
			ctxdata.commandlineArguments.push_back(str.substr(std::string("--python-arg=").length()));
			processed = true;
		}
		else if( boost::starts_with(str, "--pythonarg=") )	// option renamed in order to have a consistent naming schema, keep for backwards compatibility
		{
			ctxdata.commandlineArguments.push_back(str.substr(std::string("--pythonarg=").length()));
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

// register context data
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
		PythonAtom(ProgramCtx& ctx, std::string module, std::string functionName, std::vector<InputType> inputParameters, int outputArity, dlvhex::ExtSourceProperties prop)
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

			// update properties
			this->prop = prop;
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
				DBGLOG(DBG, "Preparing Python for query");
				PythonAPI::emb_query = &query;
				PythonAPI::emb_answer = &answer;
				PythonAPI::emb_nogoods = nogoods;

				boost::python::tuple t;
				DBGLOG(DBG, "Constructing input tuple");
				for (int i = 0; i < getInputArity(); ++i){
					if (getInputType(i) != TUPLE) t += boost::python::make_tuple(query.input[i]);
					else {
						boost::python::tuple tupleparameters;
						for (int var = i; var < query.input.size(); ++var) tupleparameters += boost::python::make_tuple(query.input[var]);
						t += boost::python::make_tuple(tupleparameters);
					}
				}

				DBGLOG(DBG, "Calling " << getPredicate() << "_caller helper function");
				PythonAPI::main.attr((getPredicate() + "_caller").c_str())(t);

				DBGLOG(DBG, "Resetting Python");
				PythonAPI::emb_query = NULL;
				PythonAPI::emb_answer = NULL;
				PythonAPI::emb_nogoods.reset();
			}catch(boost::python::error_already_set& e){
				PyErr_Print();
			}
		}
};

namespace PythonAPI{

void addAtomWithProperties(std::string name, boost::python::tuple args, int outputArity, dlvhex::ExtSourceProperties prop){
	if (!emb_pluginAtoms) throw PluginError("Cannot create external atoms at this point");
	std::vector<PluginAtom::InputType> inputParameters;
	for (int p = 0; p < boost::python::len(args); ++p){
		int arg = boost::python::extract<int>(args[p]);
		if (arg == PluginAtom::CONSTANT || arg == PluginAtom::PREDICATE || arg == PluginAtom::TUPLE) inputParameters.push_back((PluginAtom::InputType)arg);
		else throw PluginError("dlvhex.addAtom: Unknown external atom parameter type");
	}
	std::stringstream passargs;
	DBGLOG(DBG, "Defining helper function " << name << "_caller(input)");
	for (int i = 0; i < boost::python::len(args); ++i) passargs << (i > 0 ? ", " : "") << "input[" << i << "]";
	boost::python::exec(("def " + name + "_caller(input):\n " + name + "(" + passargs.str() + ")").c_str(), dict, dict);

	// return smart pointer with deleter (i.e., delete code compiled into this plugin)
	emb_pluginAtoms->push_back(dlvhex::PluginAtomPtr(new PythonAtom(*emb_ctx, "unknown", name, inputParameters, outputArity, prop), PluginPtrDeleter<PluginAtom>()));
}

void addAtom(std::string name, boost::python::tuple args, int outputArity){
	addAtomWithProperties(name, args, outputArity, dlvhex::ExtSourceProperties());
}

boost::python::tuple getTuple(ID id) {
	if (!id.isAtom() && !id.isLiteral()) throw PluginError("dlvhex.getTuple: Parameter must an atom or literal ID");
	const OrdinaryAtom& ogatom = emb_ctx->registry()->lookupOrdinaryAtom(id);
	boost::python::tuple t;
	BOOST_FOREACH (ID term, ogatom.tuple) t += boost::python::make_tuple(term);
	return t;
}

boost::python::tuple ID_tuple(ID* this_){
	return getTuple(*this_);
}

std::string getValue(ID id){
	std::stringstream ss;
	RawPrinter printer(ss, emb_ctx->registry());
	if (id.kind & ID::NAF_MASK) ss << "-";
	ID id_ = id;
	id_.kind &= (ID::ALL_ONES ^ ID::NAF_MASK);
	printer.print(id_);
	std::string str = ss.str();
	return ss.str();
}

std::string ID_value(ID* this_){
	return getValue(*this_);
}

boost::python::tuple getExtension(ID id){

	bm::bvector<>::enumerator en = emb_query->interpretation->getStorage().first();
	bm::bvector<>::enumerator en_end = emb_query->interpretation->getStorage().end();
	boost::python::tuple t;
	while (en < en_end){
		const OrdinaryAtom& atom = emb_query->interpretation->getRegistry()->ogatoms.getByAddress(*en);
		if (atom.tuple[0] == id){
			boost::python::tuple currentTup;
			for (int i = 1; i < atom.tuple.size(); ++i){
				currentTup += boost::python::make_tuple(atom.tuple[i]);
			}
			t += boost::python::make_tuple(currentTup);
		}
		en++;
	}
	return t;
}

boost::python::tuple ID_extension(ID* this_){
	return getExtension(*this_);
}

int getIntValue(ID id){
	if (!id.isTerm() || !id.isIntegerTerm()) throw PluginError("dlvhex.getIntValue: given value does not represent an integer");
	return id.address;
}

std::string getValueOfTuple(boost::python::tuple tup){

	std::stringstream ret;
	ret << "{";
	std::string delim = " ";
	for (int i = 0; i < boost::python::len(tup); ++i){
		boost::python::extract<boost::python::tuple> get_tuple(tup[i]);
		if (get_tuple.check()){
			ret << delim << getValueOfTuple(get_tuple());
		}else{
			boost::python::extract<dlvhex::ID> get_ID(tup[i]);
			if (!get_ID.check()){
				throw PluginError("dlvhex.getValue: parameter must be an ID or a tuple");
			}
			ret << delim << getValue(get_ID());
		}
		delim = ", ";
	}
	ret << " }";
	return ret.str();
}

int ID_intValue(ID* this_){
	return getIntValue(*this_);
}

boost::python::tuple getTupleValues(ID id) {
	if (!id.isAtom() && !id.isLiteral()) throw PluginError("dlvhex.getTuple: Parameter must an atom or literal ID");
	const OrdinaryAtom& ogatom = emb_ctx->registry()->lookupOrdinaryAtom(id);
	boost::python::tuple t;
	BOOST_FOREACH (ID term, ogatom.tuple) t += boost::python::make_tuple(getValue(term));
	return t;
}

boost::python::tuple ID_tupleValues(ID* this_){
	return getTupleValues(*this_);
}

ID storeInteger(int i){
	return ID::termFromInteger(i);
}

ID storeString(std::string str){
	return emb_ctx->registry()->storeConstantTerm(str);
}

ID storeAtom(boost::python::tuple args) {
	OrdinaryAtom atom(dlvhex::ID::MAINKIND_ATOM);
	bool nonground = false;
	for (int i = 0; i < boost::python::len(args); ++i){
		boost::python::extract<int> get_int(args[i]);
		if (get_int.check()){
			// store as int
			atom.tuple.push_back(dlvhex::ID::termFromInteger(get_int()));
		}else{
			boost::python::extract<std::string> get_string(args[i]);
			if (get_string.check()){
				// store as string
				std::string str = boost::python::extract<std::string>(args[i]);
				if (str[0] == '_' || (str[0] >= 'A' && str[0] <= 'Z')){
					atom.tuple.push_back(emb_ctx->registry()->storeVariableTerm(str));
					nonground = true;
				}else{
					atom.tuple.push_back(emb_ctx->registry()->storeConstantTerm(str));
				}
			}else{
				boost::python::extract<ID> get_ID(args[i]);
				if (get_ID.check()){
					if (!get_ID().isTerm()) throw PluginError("dlvhex.output: Parameters must be term IDs");
					atom.tuple.push_back(get_ID());
				}else{
					PluginError("dlvhex.output: unknown parameter type");
				}
			}
		}
	}
	if (nonground){
		atom.kind |= dlvhex::ID::SUBKIND_ATOM_ORDINARYN;
		return emb_ctx->registry()->storeOrdinaryNAtom(atom);
	}else{
		atom.kind |= dlvhex::ID::SUBKIND_ATOM_ORDINARYG;
		return emb_ctx->registry()->storeOrdinaryGAtom(atom);
	}
}

ID storeExternalAtom(std::string pred, boost::python::tuple iargs, boost::python::tuple oargs) {
	ExternalAtom eatom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_EXTERNAL);
	eatom.predicate = emb_ctx->registry()->storeConstantTerm(pred);
	for (int i = 0; i < boost::python::len(iargs); ++i){
		boost::python::extract<int> get_int(iargs[i]);
		if (get_int.check()){
			// store as int
			eatom.inputs.push_back(dlvhex::ID::termFromInteger(get_int()));
		}else{
			boost::python::extract<std::string> get_string(iargs[i]);
			if (get_string.check()){
				// store as string
				std::string str = boost::python::extract<std::string>(iargs[i]);
				if (str[0] == '_' || (str[0] >= 'A' && str[0] <= 'Z')){
					eatom.inputs.push_back(emb_ctx->registry()->storeVariableTerm(str));
				}else{
					eatom.inputs.push_back(emb_ctx->registry()->storeConstantTerm(str));
				}
			}else{
				boost::python::extract<ID> get_ID(iargs[i]);
				if (get_ID.check()){
					if (!get_ID().isTerm()) throw PluginError("dlvhex.storeExternalAtom: Parameters must be term IDs");
					eatom.inputs.push_back(get_ID());
				}else{
					PluginError("dlvhex.storeExternalAtom: unknown parameter type");
				}
			}
		}
	}
	for (int i = 0; i < boost::python::len(oargs); ++i){
		boost::python::extract<int> get_int(oargs[i]);
		if (get_int.check()){
			// store as int
			eatom.tuple.push_back(dlvhex::ID::termFromInteger(get_int()));
		}else{
			boost::python::extract<std::string> get_string(oargs[i]);
			if (get_string.check()){
				// store as string
				std::string str = boost::python::extract<std::string>(oargs[i]);
				if (str[0] == '_' || (str[0] >= 'A' && str[0] <= 'Z')){
					eatom.tuple.push_back(emb_ctx->registry()->storeVariableTerm(str));
				}else{
					eatom.tuple.push_back(emb_ctx->registry()->storeConstantTerm(str));
				}
			}else{
				boost::python::extract<ID> get_ID(oargs[i]);
				if (get_ID.check()){
					if (!get_ID().isTerm()) throw PluginError("dlvhex.storeExternalAtom: Parameters must be term IDs");
					eatom.tuple.push_back(get_ID());
				}else{
					PluginError("dlvhex.storeExternalAtom: unknown parameter type");
				}
			}
		}
	}
	return emb_ctx->registry()->eatoms.storeAndGetID(eatom);
}

ID storeRule(boost::python::tuple head, boost::python::tuple pbody, boost::python::tuple nbody) {

	Rule rule(ID::MAINKIND_RULE);
	if (boost::python::len(head) == 0) rule.kind |= ID::SUBKIND_RULE_CONSTRAINT;
	for (int i = 0; i < boost::python::len(head); ++i){
		boost::python::extract<ID> get_ID(head[i]);
		if (!get_ID.check()){
			throw PluginError("dlvhex.storeRule: Parameters must be term IDs");
		}
		rule.head.push_back(get_ID());
		if (i > 0) rule.kind |= ID::PROPERTY_RULE_DISJ;
	}
	for (int i = 0; i < boost::python::len(pbody); ++i){
		boost::python::extract<ID> get_ID(pbody[i]);
		if (!get_ID.check()){
			throw PluginError("dlvhex.storeRule: Parameters must be term IDs");
		}
		rule.body.push_back(ID::posLiteralFromAtom(get_ID()));
		if (get_ID().isExternalAtom()) rule.kind |= ID::PROPERTY_RULE_EXTATOMS;
	}
	for (int i = 0; i < boost::python::len(nbody); ++i){
		boost::python::extract<ID> get_ID(nbody[i]);
		if (!get_ID.check()){
			throw PluginError("dlvhex.storeRule: Parameters must be term IDs");
		}
		rule.body.push_back(ID::nafLiteralFromAtom(get_ID()));
		if (get_ID().isExternalAtom()) rule.kind |= ID::PROPERTY_RULE_EXTATOMS;
	}
	return emb_ctx->registry()->storeRule(rule);
}

boost::python::tuple evaluateSubprogram(boost::python::tuple tup) {

	boost::python::extract<boost::python::tuple> facts(tup[0]);
	boost::python::extract<boost::python::tuple> rules(tup[1]);

	if (!facts.check() || !rules.check()) throw PluginError("dlvhex.evaluateSubprogram: Input must be a pair of facts and rules");

	InterpretationPtr edb(new Interpretation(emb_ctx->registry()));
	for (int i = 0; i < boost::python::len(facts()); ++i){
		boost::python::extract<ID> get_ID(facts()[i]);
		if (!get_ID.check() || !get_ID().isAtom() || !get_ID().isOrdinaryGroundAtom()){
			throw PluginError("dlvhex.evaluateSubprogram: Facts must be a tuple of ground atom IDs");
		}
		edb->setFact(get_ID().address);
	}
	std::vector<ID> idb;
	for (int i = 0; i < boost::python::len(rules()); ++i){
		boost::python::extract<ID> get_ID(rules()[i]);
		if (!get_ID.check() || !get_ID().isRule()){
			throw PluginError("dlvhex.evaluateSubprogram: Facts must be a tuple of ground atom IDs");
		}
		idb.push_back(get_ID());
	}
	std::vector<InterpretationPtr> answersets = emb_ctx->evaluateSubprogram(edb, idb);
	boost::python::tuple pythonResult;
	BOOST_FOREACH (InterpretationConstPtr answerset, answersets){
		boost::python::tuple pythonAS;
		bm::bvector<>::enumerator en = answerset->getStorage().first();
		bm::bvector<>::enumerator en_end = answerset->getStorage().end();
		while (en < en_end){
			// TODO: use auxiliary mask to filter so that auxiliaries are not given to python
			pythonAS += boost::python::make_tuple(emb_ctx->registry()->ogatoms.getIDByAddress(*en));
			en++;
		}
		pythonResult += boost::python::make_tuple(pythonAS);
	}
	return pythonResult;
}

boost::python::tuple loadSubprogram(std::string filename) {

	ProgramCtx pc = *emb_ctx;
	pc.idb.clear();
	pc.edb = InterpretationPtr(new Interpretation(emb_ctx->registry()));
	pc.currentOptimum.clear();
	pc.config.setOption("NumberOfModels",0);
	InputProviderPtr ip(new InputProvider());
	ip->addFileInput(filename);
	pc.inputProvider = ip;
	ip.reset();

	DBGLOG(DBG, "Resetting context");
	pc.config.setOption("NestedHEX", 1);
	pc.state.reset();
	pc.modelBuilder.reset();
	pc.parser.reset();
	pc.evalgraph.reset();
	pc.compgraph.reset();
	pc.depgraph.reset();

	pc.config.setOption("DumpDepGraph",0);
	pc.config.setOption("DumpCyclicPredicateInputAnalysisGraph",0);
	pc.config.setOption("DumpCompGraph",0);
	pc.config.setOption("DumpEvalGraph",0);
	pc.config.setOption("DumpModelGraph",0);
	pc.config.setOption("DumpIModelGraph",0);
	pc.config.setOption("DumpAttrGraph",0);

	if( !pc.evalHeuristic ) {
		assert(false);
		throw GeneralError("No evaluation heuristics found");
	}

	pc.changeState(StatePtr(new ConvertState()));
	pc.convert();
	pc.parse();
	if( pc.maxint > emb_ctx->maxint ) {
		DBGLOG(DBG, "updating maxint of emb_ctx from " << emb_ctx->maxint << " to " << pc.maxint);
		emb_ctx->maxint = pc.maxint;
	}

	bm::bvector<>::enumerator en = pc.edb->getStorage().first();
	bm::bvector<>::enumerator en_end = pc.edb->getStorage().end();
	boost::python::tuple pyedb, pyidb;
	while (en < en_end){
		pyedb += boost::python::make_tuple(emb_ctx->registry()->ogatoms.getIDByAddress(*en));
		en++;
	}
	BOOST_FOREACH (ID ruleID, pc.idb) {
		pyidb += boost::python::make_tuple(ruleID);
	}

	return boost::python::make_tuple(pyedb, pyidb);
}

ID negate(ID id) {
	if (!id.isAtom() && !id.isLiteral()) throw PluginError("dlvhex.negate: Can only negate literal IDs");
	if (!!emb_nogoods && emb_ctx->registry()->ogatoms.getIDByAddress(id.address).isExternalAuxiliary()){
		DBGLOG(DBG, "Negating external atom output atom " << id);
		return emb_ctx->registry()->swapExternalAtomAuxiliaryAtom(emb_ctx->registry()->ogatoms.getIDByAddress(id.address));
	}else{
		DBGLOG(DBG, "Negating ordinary literal " << id);
		id.kind ^= dlvhex::ID::NAF_MASK;
		return id;
	}
}

ID ID_negate(ID* this_){
	return negate(*this_);
}

bool learn(boost::python::tuple args) {

	if (!!emb_nogoods && emb_ctx->config.getOption("ExternalLearningUser")){
		Nogood ng;
		for (int i = 0; i < boost::python::len(args); ++i){
			dlvhex::ID id = boost::python::extract<ID>(args[i]);
			if (!id.isAtom() && !id.isLiteral()) throw PluginError("dlvhex.learn: Parameters must be positive or negated atom IDs");
			ng.insert(NogoodContainer::createLiteral(id));
		}
		DBGLOG(DBG, "Learning nogood " << ng.getStringRepresentation(emb_ctx->registry()) << " from python plugin");
		emb_nogoods->addNogood(ng);

		return true;
	}else{
		return false;
	}
}

ID storeOutputAtomWithSign(boost::python::tuple args, bool sign) {

	Tuple outputTuple;
	for (int i = 0; i < boost::python::len(args); ++i){
		boost::python::extract<int> get_int(args[i]);
		if (get_int.check()){
			// store as int
			outputTuple.push_back(dlvhex::ID::termFromInteger(get_int()));
		}else{
			boost::python::extract<std::string> get_string(args[i]);
			if (get_string.check()){
				// store as string
				outputTuple.push_back(emb_ctx->registry()->storeConstantTerm(boost::python::extract<std::string>(args[i])));
			}else{
				boost::python::extract<ID> get_ID(args[i]);
				if (get_ID.check()){
					if (!get_ID().isTerm()) throw PluginError("dlvhex.output: Parameters must be term IDs");
					outputTuple.push_back(get_ID());
				}else{
					PluginError("dlvhex.output: unknown parameter type");
				}
			}
		}
	}
	return ExternalLearningHelper::getOutputAtom(*emb_query, outputTuple, sign);
}

ID storeOutputAtom(boost::python::tuple args) {
	return storeOutputAtomWithSign(args, true);
}

void output(boost::python::tuple args) {

	Tuple outputTuple;
	for (int i = 0; i < boost::python::len(args); ++i){

		boost::python::extract<int> get_int(args[i]);
		if (get_int.check()){
			// store as int
			outputTuple.push_back(dlvhex::ID::termFromInteger(get_int()));
		}else{
			boost::python::extract<std::string> get_string(args[i]);
			if (get_string.check()){
				// store as string
				outputTuple.push_back(emb_ctx->registry()->storeConstantTerm(boost::python::extract<std::string>(args[i])));
			}else{
				boost::python::extract<ID> get_ID(args[i]);
				if (get_ID.check()){
					if (!get_ID().isTerm()) throw PluginError("dlvhex.output: Parameters must be term IDs");
					outputTuple.push_back(get_ID());
				}else{
					PluginError("dlvhex.output: unknown parameter type");
				}
			}
		}
	}
	emb_answer->get().push_back(outputTuple);
}

ID getExternalAtomID() {
	return emb_query->eatomID;
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

boost::python::tuple getInputAtomsOfPredicate(ID pred) {

	bm::bvector<>::enumerator en = emb_query->predicateInputMask->getStorage().first();
	bm::bvector<>::enumerator en_end = emb_query->predicateInputMask->getStorage().end();
	boost::python::tuple t;
	while (en < en_end){
		if (emb_query->interpretation->getRegistry()->ogatoms.getByAddress(*en).tuple[0] == pred){
			t += boost::python::make_tuple(emb_query->interpretation->getRegistry()->ogatoms.getIDByAddress(*en));
		}
		en++;
	}
	return t;
}

boost::python::tuple getTrueInputAtoms() {

	bm::bvector<>::enumerator en = emb_query->interpretation->getStorage().first();
	bm::bvector<>::enumerator en_end = emb_query->interpretation->getStorage().end();
	long i = 0;
	boost::python::tuple t;
	while (en < en_end){
		t += boost::python::make_tuple(emb_query->interpretation->getRegistry()->ogatoms.getIDByAddress(*en));
		i++;
		en++;
	}
	return t;
}

boost::python::tuple getTrueInputAtomsOfPredicate(ID pred) {

	bm::bvector<>::enumerator en = emb_query->interpretation->getStorage().first();
	bm::bvector<>::enumerator en_end = emb_query->interpretation->getStorage().end();
	boost::python::tuple t;
	while (en < en_end){
		if (emb_query->interpretation->getRegistry()->ogatoms.getByAddress(*en).tuple[0] == pred){
			t += boost::python::make_tuple(emb_query->interpretation->getRegistry()->ogatoms.getIDByAddress(*en));
		}
		en++;
	}
	return t;
}

int getInputAtomCount() {
	return emb_query->predicateInputMask->getStorage().count();
}

int getTrueInputAtomCount() {
	return emb_query->interpretation->getStorage().count();
}

bool isInputAtom(ID id) {
	return emb_query->predicateInputMask->getFact(id.address);
}

bool ID_isInputAtom(ID* this_){
	return isInputAtom(*this_);
}

bool isAssignmentComplete() {
	return !emb_query->assigned;
}

bool isAssigned(ID id) {
	if (!emb_query->assigned || emb_query->assigned->getFact(id.address)) return true;
	else return false;
}

bool ID_isAssigned(ID* this_){
	return isAssigned(*this_);
}

bool hasChanged(ID id) {
	if (!emb_query->changed || emb_query->changed->getFact(id.address)) return true;
	else return false;
}

bool ID_hasChanged(ID* this_){
	return hasChanged(*this_);
}

bool isTrue(ID id) {
	if (isAssigned(id) && emb_query->interpretation->getFact(id.address)) return true;
	else return false;
}

bool ID_isTrue(ID* this_){
	return isTrue(*this_);
}

bool isFalse(ID id) {
	if (isAssigned(id) && !emb_query->interpretation->getFact(id.address)) return true;
	else return false;
}

bool ID_isFalse(ID* this_){
	return isFalse(*this_);
}

void resetCacheOfPlugins() {
	if( PythonAPI::emb_ctx == NULL ) {
		LOG(ERROR,"cannot reset plugin cache - emb_ctx = NULL");
	} else {
		PythonAPI::emb_ctx->resetCacheOfPlugins(false); // false -> reset all caches
	}
}

};

BOOST_PYTHON_MODULE(dlvhex) {
	boost::python::def("addAtom", PythonAPI::addAtomWithProperties);
	boost::python::def("addAtom", PythonAPI::addAtom);
	boost::python::def("getValue", PythonAPI::getValue);
	boost::python::def("getValue", PythonAPI::getValueOfTuple);
	boost::python::def("getExtension", PythonAPI::getExtension);
	boost::python::def("getIntValue", PythonAPI::getIntValue);
	boost::python::def("getTuple", PythonAPI::getTuple);
	boost::python::def("getTupleValues", PythonAPI::getTupleValues);
	boost::python::def("storeInteger", PythonAPI::storeInteger);
	boost::python::def("storeString", PythonAPI::storeString);
	boost::python::def("storeAtom", PythonAPI::storeAtom);
	boost::python::def("negate", PythonAPI::negate);
	boost::python::def("learn", PythonAPI::learn);
	boost::python::def("storeOutputAtom", PythonAPI::storeOutputAtomWithSign);
	boost::python::def("storeOutputAtom", PythonAPI::storeOutputAtom);
	boost::python::def("output", PythonAPI::output);
	boost::python::def("getExternalAtomID", PythonAPI::getExternalAtomID);
	boost::python::def("getInputAtoms", PythonAPI::getInputAtoms);
	boost::python::def("getInputAtoms", PythonAPI::getInputAtomsOfPredicate);
	boost::python::def("getTrueInputAtoms", PythonAPI::getTrueInputAtoms);
	boost::python::def("getTrueInputAtoms", PythonAPI::getTrueInputAtomsOfPredicate);
	boost::python::def("getInputAtomCount", PythonAPI::getInputAtomCount);
	boost::python::def("getTrueInputAtomCount", PythonAPI::getTrueInputAtomCount);
	boost::python::def("isInputAtom", PythonAPI::isInputAtom);
	boost::python::def("isAssignmentComplete", PythonAPI::isAssignmentComplete);
	boost::python::def("isAssigned", PythonAPI::isAssigned);
	boost::python::def("hasChanged", PythonAPI::hasChanged);
	boost::python::def("isTrue", PythonAPI::isTrue);
	boost::python::def("isFalse", PythonAPI::isFalse);
	boost::python::def("storeExternalAtom", PythonAPI::storeExternalAtom);
	boost::python::def("storeRule", PythonAPI::storeRule);
	boost::python::def("evaluateSubprogram", PythonAPI::evaluateSubprogram);
	boost::python::def("loadSubprogram", PythonAPI::loadSubprogram);
	boost::python::def("resetCacheOfPlugins", PythonAPI::resetCacheOfPlugins);
	boost::python::class_<dlvhex::ID>("ID")
		.def("value", &PythonAPI::ID_value)
		.def("extension", &PythonAPI::ID_extension)
		.def("intValue", &PythonAPI::ID_intValue)
		.def("tuple", &PythonAPI::ID_tuple)
		.def("tupleValues", &PythonAPI::ID_tupleValues)
		.def("negate", &PythonAPI::ID_negate)
		.def("isInputAtom", &PythonAPI::ID_isInputAtom)
		.def("isAssigned", &PythonAPI::ID_isAssigned)
		.def("hasChanged", &PythonAPI::ID_hasChanged)
		.def("isTrue", &PythonAPI::ID_isTrue)
		.def("isFalse", &PythonAPI::ID_isFalse)
		.def(boost::python::self == dlvhex::ID());
	boost::python::class_<dlvhex::ExtSourceProperties>("ExtSourceProperties")
		.def("addMonotonicInputPredicate", &dlvhex::ExtSourceProperties::addMonotonicInputPredicate)
		.def("addAntimonotonicInputPredicate", &dlvhex::ExtSourceProperties::addAntimonotonicInputPredicate)
		.def("addPredicateParameterNameIndependence", &dlvhex::ExtSourceProperties::addPredicateParameterNameIndependence)
		.def("addFiniteOutputDomain", &dlvhex::ExtSourceProperties::addFiniteOutputDomain)
		.def("addRelativeFiniteOutputDomain", &dlvhex::ExtSourceProperties::addRelativeFiniteOutputDomain)
		.def("setFunctional", &dlvhex::ExtSourceProperties::setFunctional)
		.def("setFunctionalStart", &dlvhex::ExtSourceProperties::setFunctionalStart)
		.def("setSupportSets", &dlvhex::ExtSourceProperties::setSupportSets)
		.def("setCompletePositiveSupportSets", &dlvhex::ExtSourceProperties::setCompletePositiveSupportSets)
		.def("setCompleteNegativeSupportSets", &dlvhex::ExtSourceProperties::setCompleteNegativeSupportSets)
		.def("setVariableOutputArity", &dlvhex::ExtSourceProperties::setVariableOutputArity)
		.def("setCaresAboutAssigned", &dlvhex::ExtSourceProperties::setCaresAboutAssigned)
		.def("setCaresAboutChanged", &dlvhex::ExtSourceProperties::setCaresAboutChanged)
		.def("setAtomlevellinear", &dlvhex::ExtSourceProperties::setAtomlevellinear)
		.def("setTuplelevellinear", &dlvhex::ExtSourceProperties::setTuplelevellinear)
		.def("setUsesEnvironment", &dlvhex::ExtSourceProperties::setUsesEnvironment)
		.def("setFiniteFiber", &dlvhex::ExtSourceProperties::setFiniteFiber)
		.def("addWellorderingStrlen", &dlvhex::ExtSourceProperties::addWellorderingStrlen)
		.def("addWellorderingNatural", &dlvhex::ExtSourceProperties::addWellorderingNatural);

	boost::python::scope().attr("CONSTANT") = (int)PluginAtom::CONSTANT;
	boost::python::scope().attr("PREDICATE") = (int)PluginAtom::PREDICATE;
	boost::python::scope().attr("TUPLE") = (int)PluginAtom::TUPLE;
}

std::vector<PluginAtomPtr> PythonPlugin::createAtoms(ProgramCtx& ctx) const{

	PythonAPI::emb_ctx = &ctx;
	std::vector<PluginAtomPtr> pluginAtoms;

	// we have to do the program rewriting already here because it creates some side information that we need
	PythonPlugin::CtxData& ctxdata = ctx.getPluginData<PythonPlugin>();

	// load Python plugins
	DBGLOG(DBG, "Initialize Python plugin");


	// prepare sys.argv for Python
	char** pargv;
	int iargv;
	{
		std::vector<char*> argv;
		// first argument = script or empty (should exist!)
		if( !ctxdata.pythonScripts.empty() )
		{
			argv.push_back(strdup(ctxdata.pythonScripts[0].c_str()));
		}
		else argv.push_back(strdup(""));
		// other arguments = as obtained from commandline
		BOOST_FOREACH (std::string& arg, ctxdata.commandlineArguments){
			LOG(DBG,"Handling Python Commandline Argument '" << arg << "'");
			argv.push_back(strdup(arg.c_str()));
		}
		// terminator
		argv.push_back(NULL);
		// now prepare char** which we will give to python (also to free it)
		iargv = argv.size();
		pargv = new char*[iargv];
		for(unsigned i = 0; i < iargv; ++i) pargv[i] = argv[i];
	}

#if PY_MAJOR_VERSION <= 2
		Py_Initialize();
		PySys_SetArgvEx(iargv-1, pargv, 0);
		initdlvhex();
		PythonAPI::main = boost::python::import("__main__");
		PythonAPI::dict = PythonAPI::main.attr("__dict__");
#else
		if (PyImport_AppendInittab("dlvhex", PyInit_dlvhex) == -1){
			throw PluginError("Could not register dlvhex module in Python");
		}
		Py_Initialize();
		wchar_t** wpargv = new wchar_t**[iargv];
		for(int i = 0; i < iargv; ++i) {
			if( pargv[i] == NULL ) wpargv[i] = NULL
			else {
				wpargv[i] = new wchar_t[strlen(pargv[i])+1];
				for(int c = 0; c < strlen(pargv[i])+1; c++) {
					// do a brutal cast (this will work in all 7-bit ASCII cases which is
					// enough for now)
					wpargv[i][c] = wchar_t(pargv[i][c]);
					if( wpargv[i][c] & 0x7F != wpargv[i][c] ) {
						LOG(WARN,"console input argument contains non-7-bit character in
argument '" << pargv[i] << "' !");
					}
				}
			}
		}
		PySys_SetArgvEx(iargv-1, wpargv, 0);
		PythonAPI::main = boost::python::import("__main__");
		PythonAPI::dict = PythonAPI::main.attr("__dict__");
#endif

	BOOST_FOREACH (std::string script, ctxdata.pythonScripts){
		DBGLOG(DBG, "Loading file \"" + script + "\"");
		try{
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

void PythonPlugin::runPythonMain(std::string filename){
	try{
		boost::python::exec_file(filename.c_str(), PythonAPI::dict, PythonAPI::dict);
		PythonAPI::main.attr("main")();
	}catch(boost::python::error_already_set& e){
		PyErr_Print();
	}
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
