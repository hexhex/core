
namespace
{
  inline bool bodyContainsExternalAtoms(RegistryPtr registry, const Tuple& body)
  {
    // determine external atom property
    for(Tuple::const_iterator itt = body.begin(); itt != body.end(); ++itt)
    {
      if( itt->isExternalAtom() )
      {
        return true;
      }
      else if( itt->isAggregateAtom() )
      {
        // check recursively within!
        const AggregateAtom& aatom = registry->aatoms.getByID(*itt);
        if( bodyContainsExternalAtoms(registry, aatom.atoms) )
          return true;
      }
    }
    return false;
  }

  void markExternalPropertyIfExternalBody(RegistryPtr registry, Rule& r)
  {
    Tuple eatoms;
    registry->getExternalAtomsInTuple(r.body, eatoms);
    if( !eatoms.empty() )
      r.kind |= ID::PROPERTY_RULE_EXTATOMS;
  }
}

void HexGrammarPTToASTConverter::createASTFromClause(
    node_t& node)
{
  // node is from "clause" rule
  assert(node.children.size() == 1);
  node_t& child = node.children[0];
  if( Logger::Instance().shallPrint(Logger::DBG) )
  {
    LOG(DBG,"createASTFromClause cAFC:");
    printSpiritPT(Logger::Instance().stream(), child, "cAFC");
  }
  switch(child.value.id().to_long())
  {
  case HexGrammar::Constraint:
    {
      //TODO: store file position in rule (it was stored for diagnostics)
      // node.value.value().pos.file, node.value.value().pos.line);

      Rule r(ID::MAINKIND_RULE | ID::SUBKIND_RULE_CONSTRAINT);
      r.body = createRuleBodyFromBody(child.children[1]);
      markExternalPropertyIfExternalBody(ctx.registry(), r);
      ID id = ctx.registry()->rules.storeAndGetID(r);
      ctx.idb.push_back(id);
      DBGLOG(DBG,"added constraint " << r << " with id " << id << " to idb");
    }
    break;
  case HexGrammar::WeakConstraint:
    {
      //TODO: store file position in rule (it was stored for diagnostics)
      // node.value.value().pos.file, node.value.value().pos.line);

      Rule r(
        ID::MAINKIND_RULE | ID::SUBKIND_RULE_WEAKCONSTRAINT,
        ID::termFromInteger(1),
        ID::termFromInteger(1));
      if( child.children.size() > 6 )
      {
        // there is some weight
        unsigned offset = 0;
        if( !child.children[4].children.empty() )
        {
          // found first weight
          r.weight = createTermFromIdentVarNumber(child.children[4]);
          offset = 1;
        }
        if( !child.children[5+offset].children.empty() )
        {
          // found second weight
          r.level = createTermFromIdentVarNumber(child.children[5+offset]);
        }
      }

      r.body = createRuleBodyFromBody(child.children[1]);
      markExternalPropertyIfExternalBody(ctx.registry(), r);
      ID id = ctx.registry()->rules.storeAndGetID(r);
      ctx.idb.push_back(id);
      DBGLOG(DBG,"added weakconstraint " << r << " with id " << id << " to idb");
    }
    break;
  default:
    assert(false && "encountered unknown node in createASTFromClause!");
  }
}

ID HexGrammarPTToASTConverter::createBuiltinPredFromBuiltinPred(node_t& node)
{
  assert(node.value.id() == HexGrammar::BuiltinPred);
  node_t& child = node.children[0];

  BuiltinAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_BUILTIN);
  switch(child.value.id().to_long())
  {
  case HexGrammar::BuiltinTertopPrefix:
    atom.tuple.push_back(ID::termFromBuiltinString(
          createStringFromNode(child.children[0])));
    atom.tuple.push_back(createTermFromTerm(child.children[2]));
    atom.tuple.push_back(createTermFromTerm(child.children[4]));
    atom.tuple.push_back(createTermFromTerm(child.children[6]));
    break;
  case HexGrammar::BuiltinTertopInfix:
    atom.tuple.push_back(ID::termFromBuiltinString(
          createStringFromNode(child.children[3])));
    atom.tuple.push_back(createTermFromTerm(child.children[2]));
    atom.tuple.push_back(createTermFromTerm(child.children[4]));
    atom.tuple.push_back(createTermFromTerm(child.children[0]));
    break;
  case HexGrammar::BuiltinBinopPrefix:
    atom.tuple.push_back(ID::termFromBuiltinString(
          createStringFromNode(child.children[0])));
    atom.tuple.push_back(createTermFromTerm(child.children[2]));
    atom.tuple.push_back(createTermFromTerm(child.children[4]));
    break;
  case HexGrammar::BuiltinBinopInfix:
    atom.tuple.push_back(ID::termFromBuiltinString(
          createStringFromNode(child.children[1])));
    atom.tuple.push_back(createTermFromTerm(child.children[0]));
    atom.tuple.push_back(createTermFromTerm(child.children[2]));
    break;
  case HexGrammar::BuiltinOther:
    if( child.children.size() == 6 )
    {
      // #succ/2
      atom.tuple.push_back(ID::termFromBuiltinString(
            createStringFromNode(child.children[0])));
      atom.tuple.push_back(createTermFromTerm(child.children[2]));
      atom.tuple.push_back(createTermFromTerm(child.children[4]));
    }
    else
    {
      // #int/1
      atom.tuple.push_back(ID::termFromBuiltinString(
            createStringFromNode(child.children[0])));
      atom.tuple.push_back(createTermFromTerm(child.children[2]));
    }
    break;

  default:
    assert(false && "encountered unknown node in createBuiltinPredFromBuiltinPred!");
    return ID_FAIL; // keep the compiler happy
  }

  DBGLOG(DBG,"storing builtin atom " << atom);
  ID id = ctx.registry()->batoms.storeAndGetID(atom);
  DBGLOG(DBG,"stored builtin atom " << atom << " which got id " << id);
  return id;
}

ID HexGrammarPTToASTConverter::createAggregateFromAggregate(node_t& node)
{
  //printSpiritPT(std::cerr, node, "AGG>>");
  assert(node.value.id() == HexGrammar::Aggregate);

  AggregateAtom aatom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_AGGREGATE);
  ID& leftTerm = aatom.tuple[0];
  ID& leftComp = aatom.tuple[1];
  // aggFunc is a builtin and does not cover the whole "aggregate_pred"
  ID& aggFunc = aatom.tuple[2];
  ID& rightComp = aatom.tuple[3];
  ID& rightTerm = aatom.tuple[4];

  node_t& child = node.children[0];
  if( child.value.id() == HexGrammar::AggregateRel )
  {
    // binary relation between aggregate and term:
    // aggregate_rel
    //   = (term >> aggregate_binop >> aggregate_pred)
    //   | (aggregate_pred >> aggregate_binop >> term);
    if( child.children[0].value.id() == HexGrammar::Term )
    {
      leftTerm = createTermFromTerm(child.children[0]);
      leftComp = ID::termFromBuiltinString(
          createStringFromNode(child.children[1]));
      createAggregateFromAggregatePred(
          child.children[2], aggFunc, aatom.variables, aatom.atoms);
    }
    else
    {
      createAggregateFromAggregatePred(
          child.children[0], aggFunc, aatom.variables, aatom.atoms);
      rightComp = ID::termFromBuiltinString(
          createStringFromNode(child.children[1]));
      rightTerm = createTermFromTerm(child.children[2]);
    }
  }
  else
  {
    // aggregate is in (ternary) range between terms
    // aggregate_range
    //   = (term >> aggregate_leq_binop >> aggregate_pred >> aggregate_leq_binop >> term)
    //   | (term >> aggregate_geq_binop >> aggregate_pred >> aggregate_geq_binop >> term);
    assert(child.value.id() == HexGrammar::AggregateRange);
    leftTerm = createTermFromTerm(child.children[0]);
    leftComp = ID::termFromBuiltinString(
        createStringFromNode(child.children[1]));
    createAggregateFromAggregatePred(
        child.children[2], aggFunc, aatom.variables, aatom.atoms);
    rightComp = ID::termFromBuiltinString(
        createStringFromNode(child.children[3]));
    rightTerm = createTermFromTerm(child.children[4]);
  }
  
  DBGLOG(DBG,"storing aggregate atom " << aatom);
  ID aggid = ctx.registry()->aatoms.storeAndGetID(aatom);
  DBGLOG(DBG,"stored aggregate atom " << aatom << " which got id " << aggid);
  return aggid;
}

void HexGrammarPTToASTConverter::createAggregateFromAggregatePred(
    node_t& node,
    ID& predid, Tuple& vars, Tuple& atoms)
{
  assert(node.value.id() == HexGrammar::AggregatePred);
  predid = ID::termFromBuiltinString(
        createStringFromNode(node.children[0]));
  vars = createTupleFromTerms(node.children[2]);
  atoms = createRuleBodyFromBody(node.children[4]);
}

DLVHEX_NAMESPACE_END

// vim: set expandtab:

// Local Variables:
// mode: C++
// End:
