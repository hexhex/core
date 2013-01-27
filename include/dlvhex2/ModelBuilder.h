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
 * @file   ModelBuilder.h
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Base template for model building of a ModelGraph based on an EvalGraph.
 */

#ifndef MODEL_BUILDER_HPP_INCLUDED__18012011
#define MODEL_BUILDER_HPP_INCLUDED__18012011

#include "dlvhex2/PlatformDefinitions.h"
#include "EvalGraph.h"
#include "ModelGraph.h"

DLVHEX_NAMESPACE_BEGIN

template<typename EvalGraphT>
struct ModelBuilderConfig
{
	ModelBuilderConfig(EvalGraphT& eg):
	       	eg(eg), redundancyElimination(true), constantSpace(false) {}
	EvalGraphT& eg;
	bool redundancyElimination; 
	bool constantSpace; 
};

template<typename EvalGraphT>
class ModelBuilder
{
  // types
public:
  typedef ModelBuilder<EvalGraphT>
    Self;

  // concept check: EvalGraphT must be an eval graph
  BOOST_CONCEPT_ASSERT((boost::Convertible<
      EvalGraphT,
      EvalGraph<
        typename EvalGraphT::EvalUnitPropertyBase,
        typename EvalGraphT::EvalUnitDepPropertyBase> >));
  typedef EvalGraphT
    MyEvalGraph;
  typedef typename MyEvalGraph::EvalUnit
    EvalUnit;

  // concept check: eval graph must store model generator factory properties for units
  BOOST_CONCEPT_ASSERT((boost::Convertible<
      typename EvalGraphT::EvalUnitPropertyBundle,
      EvalUnitModelGeneratorFactoryProperties<
        typename EvalGraphT::EvalUnitPropertyBundle::Interpretation> >));
  typedef typename EvalGraphT::EvalUnitPropertyBundle
		EvalUnitPropertyBundle;
  // from eval unit properties we get the interpretation type
  typedef typename EvalUnitPropertyBundle::Interpretation
    Interpretation;
  typedef typename EvalUnitPropertyBundle::Interpretation::Ptr
    InterpretationPtr;

  // we need special properties
  struct ModelProperties
  {
    // the interpretation data of this model
    InterpretationPtr interpretation;

    // for input models only:

    // whether this model is an input dummy for a root eval unit
    bool dummy;
    // whether we already tried to create all output models for this (MT_IN/MT_INPROJ) model
    bool childModelsGenerated;

    ModelProperties();
    std::ostream& print(std::ostream& o) const;
  };

  typedef ModelGraph<EvalGraphT, ModelProperties>
    MyModelGraph;
  typedef typename MyModelGraph::Model
    Model;
  typedef boost::optional<Model>
    OptionalModel;

  // members
protected:
  EvalGraphT& eg;
  MyModelGraph mg;

  // methods
public:
  ModelBuilder(ModelBuilderConfig<EvalGraphT>& cfg):
    eg(cfg.eg), mg(cfg.eg) {}
  virtual ~ModelBuilder() {}
  inline EvalGraphT& getEvalGraph() { return eg; }
  inline MyModelGraph& getModelGraph() { return mg; }

  // get next input model (projected if projection is configured) at unit u
  virtual OptionalModel getNextIModel(EvalUnit u) = 0;

  // get next output model (projected if projection is configured) at unit u
  virtual OptionalModel getNextOModel(EvalUnit u) = 0;

  // debugging methods
  virtual void printEvalGraphModelGraph(std::ostream&) = 0;
  virtual void printModelBuildingPropertyMap(std::ostream&) = 0;
};

// impl

template<typename EvalGraphT>
ModelBuilder<EvalGraphT>::ModelProperties::ModelProperties():
  interpretation(),
  dummy(false),
  childModelsGenerated(false)
{
}

template<typename EvalGraphT>
std::ostream&
ModelBuilder<EvalGraphT>::ModelProperties::print(std::ostream& o) const
{
  if( dummy )
    o << "dummy ";
  if( childModelsGenerated )
    o << "childModelsGenerated ";
  o <<
    "interpretation=" << printptr(interpretation);
  if( interpretation )
    o << *interpretation;
  return o;
}

DLVHEX_NAMESPACE_END

#endif // MODEL_BUILDER_HPP_INCLUDED__18012011
