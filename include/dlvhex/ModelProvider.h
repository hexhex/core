// see also ./Modelbuilding-Concept.txt !

// (at the moment, this file is only a CPP scratchpad)

// this is a node of the evaluation graph
class EvaluationUnit
{
  // evaluation configuration
  bool useInputProjection;
  bool useOutputProjection;

  // evaluation data storage

};

class OutputModelProvider
{
	// initialize provider at evaluation unit
	// (if unit has predecessors, this unit must already have some
	// input model provider)
	OutputModelProvider(EvaluationUnitPtr location, bool retainModels);
	/*
	 * * find input model provider, if necessary, and store them
	 */

	// position provider at begin of list
	void rewind();
	ModelHolder getNextModel()
  /*
	 * * if have no cache and no running evaluator
	 *   * initialize evaluator
	 * * if have running evaluator
	 *   * call evaluator.getNextModel()
	 *   * if model is good
	 *     * if retainModels, store in cache
	 *     * return model
	 *   * else
	 TODO continue here (factorize a bit)
	 *     * getNextInputModel() and restart
	 stop evaluator, set cache to full, return NULL
	 * * else if have full cache + cache position, return cache[position++]
	 */
};

class JoinedInputModelProvider
{
	// initialize provider at evaluation unit
	// (if unit has predecessors, these must already have output model providers)
	// joinSorting specifies the order of iteration:
	//  * first iteration attempt will be done on first item in list
	//  * if this fails, first is reset and next item is tried
	//  * therefore last unit in list will be iterated least frequently
	//  * therefore last unit in list will be the only one iterated only once
	JoinedInputModelProvider(EvaluationUnitPtr location, std::list<EvaluationUnitPtr> joinSorting, bool retainModels);
	/*
	 * * check if joinSorting corresponds to predecessors
	 * * find output model providers at predecessors and store them
	 * * find common ancestor units (CAUs) of unit "location" and store them
	 * * for each output model provider
	 *   * find order of encountered common ancestor units 
	 *   * create corresponding CAUJoinInfo list with initialized unit pointers but NULL model pointers
	 * * set initializeCAUJoinInfos = true (this is done by first call to getNextModel)
	 *
	 */

	// a JoinInfo designates a model at an evaluation unit
	// if an iterator encounters such a model 
	struct JoinInfo
	{
		EvaluationUnitPtr unit;
		ModelPtr model;
	};

	std::vector<std::list<JoinInfo> > CAUJoinInfo;

	/*
	 * * if initializeCAUJoinInfos is true:
	 *   * starting at the last output model provider mp_k, going down to the second mp_2:
	 *     * call getNextModel(joinInfo[mp_i])
	 */
	ModelHolder getNextModel();
};

