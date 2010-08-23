#include <iostream>
#include <set>
#include <string>
#include <vector>

//#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/test/minimal.hpp>

class EvalUnitPropertyBundle;
class ModelPropertyBundle;
class JoinOrder;

typedef boost::adjacency_list<
  boost::vecS, boost::vecS, boost::bidirectionalS,
  EvalUnitPropertyBundle, JoinOrder> EvalGraph;

typedef boost::adjacency_list<
  boost::listS, boost::listS, boost::directedS,
  ModelPropertyBundle, JoinOrder> ModelGraph;

typedef EvalGraph::vertex_descriptor EvalUnit;
typedef EvalGraph::edge_descriptor EvalUnitDep;
typedef ModelGraph::vertex_descriptor Model;
typedef ModelGraph::edge_descriptor ModelDep;

struct EvalUnitConfiguration
{
  std::string rules; // for testing we use stupid types
  bool iproject;
  bool oproject;

  EvalUnitConfiguration(const std::string& rules):
    rules(rules), iproject(false), oproject(false)
    {}
};

struct EvalUnitModelBuildingData
{
  Model imodel;
  Model omodel;
  int orefcount;

  EvalUnitModelBuildingData():
    imodel(), omodel(), orefcount(0)
    {}
};

struct EvalUnitPropertyBundle:
  public EvalUnitConfiguration,
  public EvalUnitModelBuildingData
{
  EvalUnitPropertyBundle():
    EvalUnitConfiguration("[uninitialized rules]"),
    EvalUnitModelBuildingData()
    {}
  EvalUnitPropertyBundle(const std::string& rules):
    EvalUnitConfiguration(rules),
    EvalUnitModelBuildingData()
    {}
};

struct JoinOrder
{
  int idx;
  JoinOrder(int idx): idx(idx) {}
};

// for testing we use stupid types
typedef std::set<std::string> AtomSet;

class Interpretation
{
public:
  // create empty
  Interpretation(const AtomSet& as): atoms(as) {}
  // create as union
  Interpretation(const Interpretation& i1, const Interpretation& i2);
  // destruct
  ~Interpretation() {}
  // output
  std::ostream& print(std::ostream& o) const
  {
    AtomSet::const_iterator it = atoms.begin();
    o << "{" << *it;
    ++it;
    for(;it != atoms.end(); ++it)
      o << "," << *it;
    o << "}";
    return o;
  }

private:
  AtomSet atoms;
};

// syntactic operator<< sugar for printing interpretations
std::ostream& operator<<(std::ostream& o, const Interpretation& i)
{
  return i.print(o);
}

enum ModelType
{
  MT_IN,
  MT_INPROJ,
  MT_OUT,
  MT_OUTPROJ,
};

struct ModelConfiguration
{
  EvalUnit unit;
  ModelType type;
};

struct ModelPropertyBundle:
  public Interpretation,
  public ModelConfiguration
{
};

int test_main(int argn, char** argv)
{
  EvalGraph eg;

  EvalUnit u1 = boost::add_vertex(EvalUnitPropertyBundle(
    "plan(a) v plan(b)."), eg); 
  EvalUnit u2 = boost::add_vertex(EvalUnitPropertyBundle(
    "need(p,C) :- &cost[plan](C). :- need(_,money)."), eg); 
  EvalUnit u3 = boost::add_vertex(EvalUnitPropertyBundle(
    "use(X) v use(Y)."), eg); 
  EvalUnit u4 = boost::add_vertex(EvalUnitPropertyBundle(
    "need(u,C) :- &cost[use](C). :- need(_,money)."), eg); 

  bool success;
  EvalUnitDep e21;
  tie(e21,success) = boost::add_edge(u2, u1, JoinOrder(0), eg);
  BOOST_REQUIRE(success);
  EvalUnitDep e31;
  tie(e31,success) = boost::add_edge(u3, u1, JoinOrder(0), eg);
  BOOST_REQUIRE(success);
  EvalUnitDep e42;
  tie(e42,success) = boost::add_edge(u4, u2, JoinOrder(0), eg);
  BOOST_REQUIRE(success);
  EvalUnitDep e43;
  tie(e43,success) = boost::add_edge(u4, u3, JoinOrder(1), eg);
  BOOST_REQUIRE(success);

  ModelGraph mg;

  // BOOST_CHECK(predicate)
  // BOOST_REQUIRE(predicate)
  // BOOST_ERROR(message)
  // BOOST_FAIL(message)
  return 0;
}
