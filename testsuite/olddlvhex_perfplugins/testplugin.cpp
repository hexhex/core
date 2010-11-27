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

class SenseNotArmed2PluginAtom : public PluginAtom
{
public:
	SenseNotArmed2PluginAtom()
	{
		addInputPredicate();
		addInputPredicate();
		addInputConstant();
		setOutputArity(0);
	}

	virtual void
	retrieve(const Query& query, Answer& answer) throw(PluginError)
	{
    const Tuple& in = query.getInputTuple();
  
    /*
    {
      const std::string& disarm = in[0].getUnquotedString();
      const std::string& look = in[1].getUnquotedString();
      const std::string& time = in[2].getUnquotedString();
      std::cerr << "query with " << disarm << " " << look << " " << time << std::endl;
    }
    */
    //std::cerr << "query with " << in[0] << " " << in[1] << " " << in[2] << std::endl;

    AtomSet setdis;
    {
      Tuple tupdis;
      tupdis.push_back(in[0]);
      tupdis.push_back(in[2]);
      AtomPtr dis(new Atom(tupdis));
      query.getInterpretation().matchAtom(dis, setdis);
    }

    AtomSet setlook;
    {
      Tuple tuplook;
      tuplook.push_back(in[1]);
      tuplook.push_back(in[2]);
      AtomPtr look(new Atom(tuplook));
      query.getInterpretation().matchAtom(look, setlook);
    }

    std::vector<Tuple> out;
    if( !setdis.empty() && !setlook.empty() )
    {
      Tuple t;
      out.push_back(t);
      answer.addTuples(out);
    }
  }
};

// the name Gen2 is due to another generic Gen1 in TestPlainHEX
class Gen2PluginAtom : public PluginAtom
{
public:
	Gen2PluginAtom(unsigned arity):
    PluginAtom(false)
	{
		setOutputArity(0);
		addInputPredicate();
    for(unsigned u = 0; u < arity; ++u)
      addInputConstant();
	}

	virtual void
	retrieve(const Query& query, Answer& answer) throw(PluginError)
	{
    const Tuple& in = query.getInputTuple();
    AtomPtr atm(new Atom(in));

    AtomSet setatm;
    {
      query.getInterpretation().matchAtom(atm, setatm);
    }

    if( !setatm.empty() )
    {
      std::vector<Tuple> out;
      Tuple t;
      out.push_back(t);
      answer.addTuples(out);
    }
  }
};


class BenchTestPlugin : public PluginInterface
{
public:
	virtual void
	getAtoms(AtomFunctionMap& a)
	{
	  boost::shared_ptr<PluginAtom> above(new AboveAtom);
	  boost::shared_ptr<PluginAtom> senseNotArmed2(new SenseNotArmed2PluginAtom);
	  boost::shared_ptr<PluginAtom> gen2_1(new Gen2PluginAtom(1));
	  boost::shared_ptr<PluginAtom> gen2_2(new Gen2PluginAtom(2));
	  boost::shared_ptr<PluginAtom> gen2_3(new Gen2PluginAtom(3));

	  a["above"] = above;
	  a["senseNotArmed2"] = senseNotArmed2;
	  a["gen1"] = gen2_1;
	  a["gen2"] = gen2_2;
	  a["gen3"] = gen2_3;
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

BenchTestPlugin theTestPlugin;

DLVHEX_NAMESPACE_END

extern "C"
DLVHEX_NAMESPACE BenchTestPlugin*
PLUGINIMPORTFUNCTION()
{
	DLVHEX_NAMESPACE theTestPlugin.setPluginName("dlvhex-benchtestplugin");
	DLVHEX_NAMESPACE theTestPlugin.setVersion(0,0,1);
	return & DLVHEX_NAMESPACE theTestPlugin;
}

/* vim: set noet sw=4 ts=4 tw=80: */


// Local Variables:
// mode: C++
// End:
