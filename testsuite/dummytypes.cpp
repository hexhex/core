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
 * @file   dummytypes.cpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Implementation of dummy replacement types for testing (model building) templates.
 */

#include "dummytypes.hpp"

TestModelGeneratorFactory::ModelGenerator::ModelGenerator(
    InterpretationConstPtr input,
    TestModelGeneratorFactory& factory):
  ModelGeneratorBase<TestInterpretation>(input),
  factory(factory),
  models(),
  mit(models.begin())
{
  LOG_METHOD("ModelGenerator()", this);
  const std::string& rules = factory.ctx.rules;
  LOG("rules '" << rules << "'");
  if( input )
    LOG("input '" << *input << "'");

  // hardcode models of given programs with given inputs
  if( rules == "plan(a) v plan(b)." )
  {
    assert(!input);
    TestAtomSet ma;
    ma.insert("plan(a)");
    TestAtomSet mb;
    mb.insert("plan(b)");
    models.push_back(TestInterpretation::Ptr(new TestInterpretation(ma)));
    models.push_back(TestInterpretation::Ptr(new TestInterpretation(mb)));
    mit = models.begin();
  }
  else if( rules == "need(p,C) :- &cost[plan](C). :- need(_,money)." )
  {
    assert(input);
    const TestAtomSet& inp = input->getAtoms();
    assert(inp.size() == 1);
    if( inp.count("plan(a)") == 1 )
    {
      // no models (constraint violated)
    }
    else if( inp.count("plan(b)") == 1 )
    {
      TestAtomSet a;
      a.insert("need(p,time)");
      models.push_back(TestInterpretation::Ptr(new TestInterpretation(a)));
      mit = models.begin();
    }
    else
    {
      assert(false);
    }
  }
  else if( rules == "use(X) v use(Y) :- plan(P), choose(P,X,Y). choose(a,c,d). choose(b,e,f)." )
  {
    assert(input);
    const TestAtomSet& inp = input->getAtoms();
    assert(inp.size() == 1);
    if( inp.count("plan(a)") == 1 )
    {
      TestAtomSet ma;
      ma.insert("use(c)");
      TestAtomSet mb;
      mb.insert("use(d)");
      models.push_back(TestInterpretation::Ptr(new TestInterpretation(ma)));
      models.push_back(TestInterpretation::Ptr(new TestInterpretation(mb)));
      mit = models.begin();
    }
    else if( inp.count("plan(b)") == 1 )
    {
      TestAtomSet ma;
      ma.insert("use(e)");
      TestAtomSet mb;
      mb.insert("use(f)");
      models.push_back(TestInterpretation::Ptr(new TestInterpretation(ma)));
      models.push_back(TestInterpretation::Ptr(new TestInterpretation(mb)));
      mit = models.begin();
    }
    else
    {
      assert(false);
    }
  }
  else if( rules == "need(u,C) :- &cost[use](C). :- need(_,money)." )
  {
    assert(input);
    const TestAtomSet& inp = input->getAtoms();
    assert(inp.size() == 2);
    if( inp.count("need(p,time)") == 1 && inp.count("use(e)") )
    {
      TestAtomSet ma;
      ma.insert("need(u,time)");
      models.push_back(TestInterpretation::Ptr(new TestInterpretation(ma)));
      mit = models.begin();
    }
    else if( inp.count("need(p,time)") == 1 && inp.count("use(f)") )
    {
      // no models (constraint violated)
    }
    else
    {
      assert(false);
    }
  }
  else if( rules == 
    "plan(a) v plan(b)."
    "use(X) v use(Y) :- plan(P), choose(P,X,Y)."
    "choose(a,c,d). choose(b,e,f)." )
  {
    assert(!input);
    TestAtomSet mac;
    mac.insert("plan(a)");
    mac.insert("use(c)");
    TestAtomSet mad;
    mad.insert("plan(a)");
    mad.insert("use(d)");
    TestAtomSet mbe;
    mbe.insert("plan(b)");
    mbe.insert("use(e)");
    TestAtomSet mbf;
    mbf.insert("plan(b)");
    mbf.insert("use(f)");
    models.push_back(TestInterpretation::Ptr(new TestInterpretation(mac)));
    models.push_back(TestInterpretation::Ptr(new TestInterpretation(mad)));
    models.push_back(TestInterpretation::Ptr(new TestInterpretation(mbe)));
    models.push_back(TestInterpretation::Ptr(new TestInterpretation(mbf)));
    mit = models.begin();
  }
  else if( rules == 
    "need(p,C) :- &cost[plan](C)."
    "need(u,C) :- &cost[use](C)." )
  {
    assert(input);
    const TestAtomSet& inp = input->getAtoms();
    assert(inp.size() == 2);
    TestAtomSet m;
    if( inp.count("plan(a)") == 1 )
      m.insert("need(p,money)");
    else
      m.insert("need(p,time)");
    if( inp.count("use(f)") == 1 )
      m.insert("need(u,money)");
    else
      m.insert("need(u,time)");
    models.push_back(TestInterpretation::Ptr(new TestInterpretation(m)));
    mit = models.begin();
  }
  else if( rules == ":- need(X,money)." )
  {
    assert(input);
    const TestAtomSet& inp = input->getAtoms();
    assert(inp.size() == 2);
    if( inp.count("need(p,money)") == 1 || inp.count("need(u,money)") == 1 )
    {
      // no models (constraint violated)
    }
    else
    {
      // empty model (consistent)
      TestAtomSet m;
      models.push_back(TestInterpretation::Ptr(new TestInterpretation(m)));
      mit = models.begin();
    }
  }
  else
  {
    std::cerr << "TODO hardcode rules '" << rules << "'" << std::endl;
    assert(false);
  }

  LOG_INDENT();
  BOOST_FOREACH(TestInterpretation::Ptr intp, models)
    LOG("model " << *intp);
}
