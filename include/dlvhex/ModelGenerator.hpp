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
 * @file   ModelGenerator.hpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Base classes for model generators.
 */

#ifndef MODEL_GENERATOR_HPP_INCLUDED__30082010
#define MODEL_GENERATOR_HPP_INCLUDED__30082010

#include <ostream>
#include <boost/shared_ptr.hpp>

class InterpretationBase
{
  // debug
  std::ostream& print(std::ostream& o) const
  { return o << "InterpretationBase::print() not overloaded"; }
};

//
// A model generator does the following:
// * it is constructed by a ModelGeneratorFactory which knows the program
//   (and can precompute information for evaluation,
//    and may also provide this to the model generator)
// * it is evaluated on a (probably empty) input interpretation
// * this evaluation can be performed online
// * evaluation yields a (probably empty) set of output interpretations
//
template<typename InterpretationT>
class ModelGeneratorBase
{
  // types
public:
  BOOST_CONCEPT_ASSERT((boost::Convertible<InterpretationT, InterpretationBase>));

  typedef InterpretationT Interpretation;
  // those typedefs are just to remove the 'typename's from the interface
  typedef typename Interpretation::ConstPtr InterpretationConstPtr;
  typedef typename Interpretation::Ptr InterpretationPtr;
  typedef boost::shared_ptr<ModelGeneratorBase<Interpretation> > Ptr;

  // storage
protected:
  InterpretationConstPtr input;

  // members
public:
  // initialize with factory and input interpretation
  ModelGeneratorBase(InterpretationConstPtr input):
    input(input) {}
  virtual ~ModelGeneratorBase() {}

  // generate and return next model, return null after last model
  virtual InterpretationPtr generateNextModel() = 0;

  // debug output
  virtual std::ostream& print(std::ostream& o) const
    { return o << "ModelGeneratorBase::print() not overloaded"; }
};

//
// a model generator factory provides model generators
// for a certain types of interpretations
//
template<typename InterpretationT>
class ModelGeneratorFactoryBase
{
  // types
public:
  typedef InterpretationT Interpretation;

public:
  typedef boost::shared_ptr<
		ModelGeneratorFactoryBase<InterpretationT> > Ptr;

  typedef ModelGeneratorBase<InterpretationT> MyModelGeneratorBase;
  typedef typename MyModelGeneratorBase::Ptr ModelGeneratorPtr;
  typedef typename MyModelGeneratorBase::InterpretationConstPtr
		InterpretationConstPtr;

  // methods
public:
  ModelGeneratorFactoryBase() {}
  virtual ~ModelGeneratorFactoryBase() {}

  virtual ModelGeneratorPtr createModelGenerator(
      InterpretationConstPtr input) = 0;
  virtual std::ostream& print(std::ostream& o) const
    { return o << "ModelGeneratorFactoryBase::print() not overloaded"; }
};

// model generator factory properties for eval units
// such properties are required by model builders
template<typename InterpretationT>
struct EvalUnitModelGeneratorFactoryProperties
{
  BOOST_CONCEPT_ASSERT((boost::Convertible<InterpretationT, InterpretationBase>));
	typedef InterpretationT Interpretation;

  // aka model generator factory
  typename ModelGeneratorFactoryBase<InterpretationT>::Ptr
		mgf; // aka model generator factory
};

#endif //MODEL_GENERATOR_HPP_INCLUDED__30082010
