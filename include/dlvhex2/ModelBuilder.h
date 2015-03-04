/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Thomas Krennwallner
 * Copyright (C) 2009, 2010 Peter Schüller
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

/** \brief Generic configuration for all model builders. */
template<typename EvalGraphT>
struct ModelBuilderConfig
{
	/** \brief Constructor.
	  * @param eg See ModelBuilderConfig::eg. */
	ModelBuilderConfig(EvalGraphT& eg):
	       	eg(eg), redundancyElimination(true), constantSpace(false) {}
	/** \brief Evaluation graph to use for model building. */
	EvalGraphT& eg;
	/** \brief True to optimize redundant parts in the model building process. */
	bool redundancyElimination; 
	/** \brief True to work with constant space. */
	bool constantSpace; 
};

/** \brief Base class for all model builders. */
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

  /** \brief Properties of a model. */
  struct ModelProperties
  {
    /** \brief The interpretation data of this model. */
    InterpretationPtr interpretation;

    // for input models only:

    /** \brief Whether this model is an input dummy for a root eval unit. */
    bool dummy;
    /** \brief Whether we already tried to create all output models for this (MT_IN/MT_INPROJ) model. */
    bool childModelsGenerated;

    /** \brief Constructor. */
    ModelProperties();
    /** \brief Prints this properties strcture.
      * @param o Stream to print to.
      * @return \p o. */
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
  /** \brief Evaluation graph to use. */
  EvalGraphT& eg;
  /** \brief Model graph to be constructed during model building. */
  MyModelGraph mg;

  // methods
public:
  /** \brief Constructor.
    * @param cfg Configuration. */
  ModelBuilder(ModelBuilderConfig<EvalGraphT>& cfg):
    eg(cfg.eg), mg(cfg.eg) {}
  /** \brief Destructor. */
  virtual ~ModelBuilder() {}
  /** \brief Returns the internal evaluation graph.
    * @return Evaluation graph. */
  inline EvalGraphT& getEvalGraph() { return eg; }
  /** \brief Returns the internal model graph.
    * @return Model graph. */
  inline MyModelGraph& getModelGraph() { return mg; }

  /** \brief Get next input model (projected if projection is configured) at e given unit.
    * @param u The unit whose next input model shall be returned.
    * @return Next input model at unit \p u. */
  virtual OptionalModel getNextIModel(EvalUnit u) = 0;

  /** \brief Get next output model (projected if projection is configured) at e given unit.
    * @param u The unit whose next output model shall be returned.
    * @return Next output model at unit \p u. */
  virtual OptionalModel getNextOModel(EvalUnit u) = 0;

  // debugging methods
  /** \brief Prints both the evaluation and the model graph for debugging purposes.
    * @param o The stream to print to. */
  virtual void printEvalGraphModelGraph(std::ostream& o) = 0;
  /** \brief Prints the model building properties for debugging purposes.
    * @param o The stream to print to. */
  virtual void printModelBuildingPropertyMap(std::ostream& o) = 0;
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
