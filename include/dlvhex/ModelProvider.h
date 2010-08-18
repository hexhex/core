// see also ./Modelbuilding-Concept.txt !

// (at the moment, this file is only a CPP scratchpad)

// proposal: use the following shortcuts for all types:
// EvaluationGraph => EvalGraph
// EvaluationUnit => EvalUnit
// InputModel => IModel
// OutputModel => OModel

typedef EvalGraph::vertex_descriptor EvalUnit;
typedef ModelGraph::vertex_descriptor Model;

// a JoinInfo designates a model at an evaluation unit
// if an iterator encounters such a model
struct JoinInfo
{
  //! the unit where we care about common ancestry
  EvalUnit unit;

  /**
   * reference counter
   *
   * initially 0
   * * value of 0 means
   *   * model is not set
   *   * a model provider which encounters this JoinInfo
   *     * determines the model
   *     * increase refcount to 1
   * * values > 1 mean
   *   * model is set and has to be honored
   *   * a model provider which encounters this Joininfo
   *     * continues iterating until it finds a model equal to the stored one
   *     * if not found -> no model
   *     * if found -> increase refcount
   * * a model provider that increased a refcount
   *   decreases it as soon as it iterates to its next model
   */
  int refcount;

  //! the stored model
  Model model;
};

// this will be the storage of a node of the evaluation graph
class EvalUnitStorage
{
  // evaluation configuration
  bool useIProjection;
  bool useOProjection;

  // model providers

  // if using input projection, this is the projected model provider
  // the nonprojected model provider can be obtained from this one
  IModelProviderPtr inputMP;

  // if using output projection, this is the projected model provider
  // the nonprojected model provider can be obtained from this one
  OModelProviderPtr outputMP;

  // evaluation data storage
};

// stores models at one unit,
// this will be kind of a subgraph of the model graph
// the intended purpose of this class is to provide means
// * for iterating over all (input, output, projected input, projected output) models at a unit
// * for applying a storage policy (retain models vs discard models)
class ModelStorage
{
};

class ModelMgr
{
  typedef ModelStorage::iterator iterator;

  // stores input models
  ModelStorage istorage;
  // stores output models
  ModelStorage ostorage;

  // to generate output models from input models
  ModelGenerator generator;

  iterator begin() { return storage.begin(); }
  iterator end() { return storage.end(); }

  // create next model and add to storage
  boost::optional<iterator> createNextModel();
};

class OModelProvider
{
  ModelMgr mgr;
  ModelMgr::iterator iterator;

	// initialize provider at evaluation unit
	// (if unit has predecessors, this unit must already have some
	// input model provider)
	OModelProvider(EvalUnit location);
	/*
	 * find mgr and store it
	 */

	// position provider at begin of list
	void rewind();
  /*
   * iterator = boost::optional<IteratorT>()
   */

  // get next model as it is stored or can be calculated
	Model getNext();
  /*
	 * if !!iterator [boost::optional]
   *   iterator++
   * else
   *   iterator = mgr.begin() [may return end of storage]
   * if iterator == mgr.end()
   *   iterator = mgr.createNextModel() [will add to storage]
   * if !!iterator
   *   return iterator
   */   
};

typedef std::list<JoinInfo> JoinInfoList;

class MatchingOModelProvider:
  public OModelProvider
{
  // create matching o model provider at location,
  // with certain sublist of join info list (always up to the end)
  // (we could pass the whole list, but we pass iterators to reduce iteration time)
  MatchingOModelProvider(EvalUnit location, JoinInfoList::iterator firstmatch, JoinInfoList::iterator lastmatch);
  /*
   * get subset of JoinInfos relevant for this model provider
   *   (by following unit dependencies up until unit < lastmatch.unit [< wrt graph partial order])
   * for each relevant JoinInfo
   *   store instructions how to get there:
   *     either follow the model graph arcs and try all models (iterate)
   *     or choose a specific arc (for joins)
   * [it can never happen that we need to choose two or more arcs,
   *  as in this case 
   */

  // get next model and care about the part of ancestry specified in matches
  // (some masked ancestors may be determined by this provider,
  //  others must be honored by this provider (refcount decides))
	Model getNext();
  /*
   *
   * candidate = OModelProvider::getNext();
   */

class JoinedIModelCreator
{
	// initialize model creator at evaluation unit
	// (if unit has predecessors, these must already have output model providers)
	// unit predecessor edges must have a total ordering:
	//  * first iteration attempt will be done on first item in list
	//  * if this fails, first is reset and next item is tried
	//  * therefore last unit in list will be iterated least frequently
	//  * therefore last unit in list will be the only one iterated only once
	JoinedIModelCreator(EvaluationUnitPtr location);
	/*
	 * check if joinSorting corresponds to predecessors
	 * find output model providers at predecessors and store them
	 * find common ancestor units (CAUs) of unit "location" and store them
	 * for each output model provider
	 *   find order of encountered common ancestor units 
	 *   create corresponding CAUJoinInfo list with initialized unit pointers but NULL model pointers
	 * set initializeCAUJoinInfos = true (this is done by first call to getNextModel)
   *
	 *
   * n sorted predecessors:
   *
   * * storage per join:
   * // common ancestor unit and join graph storage
   * JoinInfoSubgraph (subgraph of eval graph with special properties) joinGraph;
   * vertex properties:
   * * bool cau = false; // whether this is a CAU
   * * int refcount = 0; // reference count if it is a CAU
   * * ModelStorage::optional_iterator ati; // currently used input model (not for roots of joinGraph)
   * * ModelStorage::optional_iterator ato; // currently used output model (not for join vertex)
   * * vector<vertex> joinInputs; // determined from graph, with certain order (only for join vertex)
   * * int lastValidJoinInput; // index into joinInputs: this is the highest input with a matching model
   *
   * * init join:
   * calculate caus
   * create joinGraph
   * [create joining vertex depending on all joined units]
   *
   * * storage per joining vertex in joinGraph:
   * const int n;
   * int at = 0;
   * 
   * * getnext per joining vertex in joinGraph:
   * do
   *   // try to get next model at current provider
   *   if providers[at].getNextMatching(caus) returns no model
   *     // no more models compatible to providers before 'at'
   *     if at == 0
   *       // backtracking before begin -> no more models
   *       return NULL
   *     // reset this provider for next try
   *     providers[at].rewind()
   *     // continue to look for a good combination in previous providers
   *     at--
   *   else
   *     // found compatible model @ 'at'
   *     if at == (n-1)
   *       // we have found a model for every provider -> success
   *       if not the same as previously created model
   *         [getNextMatching iterates through children and may return the same leaf model over and over again]
   *         return tuple <m_1,...,m_n> where m_i = providers[i].getCurrent();
   *     else
   *       // look for compatible model at next provider
   *       at++
   * while true
   *
   * model providers on subgraph:
   * * iteration layers at each evaluation unit
   *   1) output model iteration (iterate over inputs causing that output)
   *      [always present]
   *      [trivial without output projection]
   *   2) input model iteration (iterate over nonprojected inputs causing that input)
   *      [only present if unit has predecessors in subgraph]
   *      [trivial without input projection]
   *
   * * setting up modelproviders on subgraph:
   *
   * omodelprovider storage:
   * Model oat;
   * Model iat;
   * 
   * omodelprovider getNextMatching:
   * 
	 */

	std::vector<std::list<JoinInfo> > CAUJoinInfo;

  // adds model to model graph
  // each joined model depends on all its joined input models (sorted using some index!)
  // each joined model additionally refers to a set of compatible origin models for each CAU (to make model expansion easier)
	Model createNextModel();
	/*
	 * * if initializeCAUJoinInfos is true:
	 *   * starting at the last output model provider mp_k, going down to the second mp_2:
	 *     * call getNextModel(joinInfo[mp_i])
	 */
  
};

