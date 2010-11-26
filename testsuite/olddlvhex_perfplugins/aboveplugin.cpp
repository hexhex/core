#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "dlvhex/PluginInterface.h"

#include <string>
#include <sstream>
#include <iostream>

#include <cstdio>
#include <cassert>

DLVHEX_NAMESPACE_BEGIN

class AboveAtom : public PluginAtom
{
public:
	AboveAtom()
	{
		addInputPredicate();
		addInputConstant();
		setOutputArity(1);
	}

	virtual void
	retrieve(const Query& query, Answer& answer) throw(PluginError)
	{
    const Tuple& in = query.getInputTuple();
  
    const std::string& pred = in[0].getUnquotedString();
    const std::string& cons = in[1].getUnquotedString();
    
    AtomSet setpred;
    query.getInterpretation().matchPredicate(pred, setpred);

    std::vector<Tuple> out;

    for(AtomSet::const_iterator it = setpred.begin();
        it != setpred.end(); ++it)
    {
      const Tuple& args = it->getArguments();
      assert(args.size() == 1);
      assert(args[0].isSymbol());

      if( args[0].getString() >= cons )
      {
        Tuple t;
        t.push_back(args[0]);
        out.push_back(t);
      }
    }
    answer.addTuples(out);
  }
};

class AbovePlugin : public PluginInterface
{
public:
	virtual void
	getAtoms(AtomFunctionMap& a)
	{
	  boost::shared_ptr<PluginAtom> above(new AboveAtom);

	  a["above"] = above;
	}

	virtual void
	setOptions(bool /* doHelp */, std::vector<std::string>& /* argv */, std::ostream& /* out */)
	{
		//
		// no options yet
		//
		return;
	}
};

AbovePlugin theTestPlugin;

DLVHEX_NAMESPACE_END

extern "C"
DLVHEX_NAMESPACE AbovePlugin*
PLUGINIMPORTFUNCTION()
{
	DLVHEX_NAMESPACE theTestPlugin.setPluginName("dlvhex-aboveplugin");
	DLVHEX_NAMESPACE theTestPlugin.setVersion(0,0,1);
	return & DLVHEX_NAMESPACE theTestPlugin;
}

/* vim: set noet sw=4 ts=4 tw=80: */


// Local Variables:
// mode: C++
// End:
