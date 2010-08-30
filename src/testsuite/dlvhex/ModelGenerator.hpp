#ifndef MODEL_GENERATOR_HPP_INCLUDED__30082010
#define MODEL_GENERATOR_HPP_INCLUDED__30082010

#include <boost/shared_ptr.hpp>

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
};

#endif //MODEL_GENERATOR_HPP_INCLUDED__30082010
