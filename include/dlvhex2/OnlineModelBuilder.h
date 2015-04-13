/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005-2007 Roman Schindlauer
 * Copyright (C) 2006-2015 Thomas Krennwallner
 * Copyright (C) Peter Schüller
 * Copyright (C) 2011-2015 Christoph Redl
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
 * @file   OnlineModelBuilder.h
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 *
 * @brief  Template for online model building of a ModelGraph based on an EvalGraph.
 */

#ifndef ONLINE_MODEL_BUILDER_HPP_INCLUDED__23092010
#define ONLINE_MODEL_BUILDER_HPP_INCLUDED__23092010

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/Logger.h"
#include "dlvhex2/ModelGenerator.h"
#include "dlvhex2/ModelBuilder.h"

#include <iomanip>

DLVHEX_NAMESPACE_BEGIN

/** \brief Template for online model building of a ModelGraph based on an EvalGraph. */
template<typename EvalGraphT>
class OnlineModelBuilder:
public ModelBuilder<EvalGraphT>
{
    // types
    public:
        typedef ModelBuilder<EvalGraphT>
            Base;
        typedef OnlineModelBuilder<EvalGraphT>
            Self;

        // import typedefs from base class
        typedef typename Base::MyEvalGraph
            MyEvalGraph;
        typedef typename Base::EvalUnit
            EvalUnit;
        typedef typename Base::EvalUnitPropertyBundle
            EvalUnitPropertyBundle;
        typedef typename Base::Interpretation
            Interpretation;
        typedef typename Base::InterpretationPtr
            InterpretationPtr;
        typedef typename Base::MyModelGraph
            MyModelGraph;
        typedef typename Base::Model
            Model;
        typedef typename Base::OptionalModel
            OptionalModel;

        // import members from base class
        using Base::eg;
        using Base::mg;

        // our own typedefs
        typedef typename MyEvalGraph::EvalUnitDep
            EvalUnitDep;
        typedef typename MyEvalGraph::PredecessorIterator
            EvalUnitPredecessorIterator;

        typedef typename MyModelGraph::ModelDep
            ModelDep;
        typedef typename MyModelGraph::ModelPropertyBundle
            ModelPropertyBundle;
        typedef typename MyModelGraph::ModelList
            ModelList;
        typedef boost::optional<typename MyModelGraph::ModelList::const_iterator>
            OptionalModelListIterator;
        typedef typename MyModelGraph::PredecessorIterator
            ModelPredecessorIterator;
        typedef typename MyModelGraph::SuccessorIterator
            ModelSuccessorIterator;
        typedef boost::optional<typename MyModelGraph::SuccessorIterator>
            OptionalModelSuccessorIterator;

        /** Properties required at each eval unit for model building:
         * * model generator factory
         * * current models and refcount. */
        struct EvalUnitModelBuildingProperties
        {
            // storage

            // currently running model generator
            // (such a model generator is bound to some input model)
            // (it is reinitialized for each new input model)
            typename ModelGeneratorBase<Interpretation>::Ptr currentmg;

            bool needInput;

            unsigned orefcount;

            protected:
                /** \brief imodel currently being present in iteration (dummy if !needInput). */
                OptionalModel imodel;

            public:
                /** \brief Current successor of imodel. */
                OptionalModelSuccessorIterator currentisuccessor;

                /** \brief Constructor. */
                EvalUnitModelBuildingProperties():
                currentmg(), needInput(false), orefcount(0),
                    imodel(), currentisuccessor()
                    {}
                /** \brief Return input model.
                 * @return Input model or nothing. */
                inline const OptionalModel& getIModel() const
                {
                    return imodel;
                }

                /** \brief Set input model.
                 * @param m Input model or nothing. */
                void setIModel(OptionalModel m) {
                    // we can change the imodel iff currentmg is null
                    assert(!(!!m && imodel != m && currentmg != 0));
                    // log warning if we unset the imodel if currentmg is not null
                    if( !m && imodel != m && currentmg != 0 ) {
                        LOG(WARNING,"WARNING: unsetting imodel while currentmg is null -> unsetting currentmg too");
                        currentmg.reset();
                    }
                    imodel = m;
                }

                /** \brief Checks if an output model is present.
                 * @return True if an output model is present. */
                bool hasOModel() const
                    { return !!currentisuccessor; }
        };
        typedef boost::vector_property_map<EvalUnitModelBuildingProperties>
            EvalUnitModelBuildingPropertyMap;

        /** \brief Helper for printEUMBP.
         * @param o The stream to print to.
         * @param p EvalUnitModelBuildingProperties.
         * @return \p o. */
        std::ostream& printEUMBPhelper(
            std::ostream& o, const EvalUnitModelBuildingProperties& p) const
        {
            o <<
                "currentmg = " << std::setw(9) << printptr(p.currentmg) <<
                ", needInput = " << p.needInput <<
                ", orefcount = " << p.orefcount <<
                ", imodel = " << std::setw(9) << printopt(p.getIModel()) <<
                ", currentisuccessor = ";
            if( !!p.currentisuccessor )
                o << mg.sourceOf(*p.currentisuccessor.get())
                    << " -> "
                    << mg.targetOf(*p.currentisuccessor.get());
            else
                o << "unset";
            return o;
        }

        /** \brief Helper for printEUMBP.
         * @param p EvalUnitModelBuildingProperties.
         * @return print_container pointer. */
        print_container* printEUMBP(
            const EvalUnitModelBuildingProperties& p) const
        {
            return print_function(boost::bind(&Self::printEUMBPhelper, this, _1, p));
        }

        /** \brief Returns the output model.
         * @param p EvalUnitModelBuildingProperties.
         * @return Output model. */
        Model getOModel(const EvalUnitModelBuildingProperties& p) const
        {
            assert(!!p.currentisuccessor);
            return mg.sourceOf(*p.currentisuccessor.get());
        }

    protected:
        /** \brief Clears the interpretation of an input model.
         * @param m Input model. */
        void clearIModel(Model m) {
            mg.propsOf(m).interpretation.reset();
        }

        /** \brief Clears the interpretation of an output model.
         * @param msi ModelSuccessorIterator. */
        void clearOModel(ModelSuccessorIterator msi) {
            mg.propsOf(mg.sourceOf(*msi)).interpretation.reset();
        }

    private:
        /** \brief Observer. */
        typedef typename EvalGraphT::Observer EvalGraphObserverBase;
        class EvalGraphObserver:
    public EvalGraphObserverBase
    {
        public:
            /** \brief Constructor.
             * @param omb Model builder. */
            EvalGraphObserver(Self& omb): omb(omb) {}
            /** \brief Destructor. */
            virtual ~EvalGraphObserver() {}
            /** \brief Adds a unit.
             * @param u Evaluation unit. */
            virtual void addUnit(EvalUnit u) {
                DBGLOG(DBG,"observing addUnit(" << u << ")");
                EvalUnitModelBuildingProperties& mbprops =
                    omb.mbp[u];
                mbprops.needInput = false;
            }
            /** \brief Adds a dependency.
             * @param g Dependency. */
            virtual void addDependency(EvalUnitDep d) {
                DBGLOG(DBG,"observing addDependency(" << omb.eg.sourceOf(d) << " -> " << omb.eg.targetOf(d) << ")");
                EvalUnitModelBuildingProperties& mbprops =
                    omb.mbp[omb.eg.sourceOf(d)];
                mbprops.needInput = true;
            }

        protected:
            /** \brief Model builder. */
            Self& omb;
    };

    // members
    protected:
        /** \brief Model building properties. */
        EvalUnitModelBuildingPropertyMap mbp;
        /** \brief EvalGraphObserver. */
        boost::shared_ptr<EvalGraphObserver> ego;
        /** \brief See ModelBuilderConfig. */
        bool redundancyElimination;
        /** \brief See ModelBuilderConfig. */
        bool constantSpace;

        // methods
    public:
        /** \brief Constructor.
         * @param cfg Configuration. */
        OnlineModelBuilder(ModelBuilderConfig<EvalGraphT>& cfg):
        Base(cfg),
            mbp(),
        // setup observer to do the things below in case EvalGraph is changed
        // after the creation of this OnlineModelBuilder
            ego(new EvalGraphObserver(*this)),
            redundancyElimination(cfg.redundancyElimination),
        constantSpace(cfg.constantSpace) {
            EvalGraphT& eg = cfg.eg;
            // allocate full mbp (plus one unit, as we will likely get an additional vertex)
            EvalUnitModelBuildingProperties& mbproptemp = mbp[eg.countEvalUnits()];
            (void)mbproptemp;

            // initialize mbp for each vertex in eg
            typename EvalGraphT::EvalUnitIterator it, end;
            for(boost::tie(it, end) = eg.getEvalUnits(); it != end; ++it) {
                EvalUnit u = *it;
                DBGLOG(DBG,"initializing mbp for unit " << u);
                EvalUnitModelBuildingProperties& mbprops = mbp[u];
                EvalUnitPredecessorIterator it, end;
                boost::tie(it, end) = eg.getPredecessors(u);
                if( it != end )
                    mbprops.needInput = true;
                else {
                    mbprops.needInput = false;
                    assert(!eg.propsOf(u).iproject);
                }
            }
            eg.addObserver(ego);
        }

        /** \brief Destructor. */
        virtual ~OnlineModelBuilder() { }

    protected:
        /** \brief Helper for getNextIModel.
         * @param u Evaluation Unit.
         * @return Model. */
        Model createIModelFromPredecessorOModels(EvalUnit u);

        /**
         * nonrecursive "get next" wrt. a mandatory imodel
         * @param u Evaluation unit.
         * @return OptionalModel.
         */
        OptionalModel advanceOModelForIModel(EvalUnit u);
        /** \brief Helper for advanceOModelForIModel.
         * @param u Evaluation unit.
         * @return OptionalModel. */
        OptionalModel createNextModel(EvalUnit u);
        /** \brief Helper for advanceOModelForIModel.
         * @param u Evaluation unit.
         * @param cursor Cursor.
         * @return EvalUnitPredecessorIterator or nothing. */
        boost::optional<EvalUnitPredecessorIterator>
            ensureModelIncrement(EvalUnit u, EvalUnitPredecessorIterator cursor);

        /** \brief Removes a model from the model graph to keep the evaluation in constant space.
         * @param m Model to remove. */
        void removeIModelFromGraphs(Model m);

    public:
        // get next input model (projected if projection is configured) at unit u
        virtual OptionalModel getNextIModel(EvalUnit u);

        // get next output model (projected if projection is configured) at unit u
        virtual OptionalModel getNextOModel(EvalUnit u);

        // debugging methods
    public:
        virtual void printEvalGraphModelGraph(std::ostream&);
        virtual void printModelBuildingPropertyMap(std::ostream&);
};

template<typename EvalGraphT>
void
OnlineModelBuilder<EvalGraphT>::printEvalGraphModelGraph(std::ostream& o)
{
    o << "eval graph/model graph" << std::endl;
    typename EvalGraphT::EvalUnitIterator uit, ubegin, uend;
    boost::tie(ubegin, uend) = eg.getEvalUnits();
    for(uit = ubegin; uit != uend; ++uit) {
        std::string indent = "  ";
        EvalUnit u = *uit;
        std::stringstream s; s << "u " << u << " ";
        indent = s.str();
        o << indent << "=unit " << std::endl;

        // EvalUnitProjectionProperties
        o << indent << "iproject = " << eg.propsOf(u).iproject << " oproject = " << eg.propsOf(u).oproject << std::endl;

        // EvalUnitModelGeneratorFactoryProperties
        if( eg.propsOf(u).mgf ) {
            o << indent  <<
                "model generator factory = " << printptr(eg.propsOf(u).mgf) <<
                ":" << *eg.propsOf(u).mgf << std::endl;
        }
        else {
            o << indent  <<
                "no model generator factory" << std::endl;
        }

        // unit dependencies
        typename EvalGraphT::PredecessorIterator pit, pbegin, pend;
        boost::tie(pbegin, pend) = eg.getPredecessors(u);
        for(pit = pbegin; pit != pend; ++pit) {
            o << indent <<
                "-> depends on unit " << eg.targetOf(*pit) <<
                "/join order " << eg.propsOf(*pit).joinOrder << std::endl;
        }

        // models
        indent += "models ";
        for(ModelType t = MT_IN; t <= MT_OUTPROJ; t = static_cast<ModelType>(static_cast<unsigned>(t)+1)) {
            const ModelList& modelsAt = mg.modelsAt(u, t);
            typename MyModelGraph::ModelList::const_iterator mit;
            for(mit = modelsAt.begin(); mit != modelsAt.end(); ++mit) {
                Model m = *mit;
                o << indent <<
                    toString(t) << "@" << m << ": " << mg.propsOf(m) << std::endl;
                // model dependencies (preds)
                ModelPredecessorIterator pit, pbegin, pend;
                boost::tie(pbegin, pend) = mg.getPredecessors(m);
                for(pit = pbegin; pit != pend; ++pit) {
                    o << indent <<
                        "-> depends on model " << mg.targetOf(*pit) <<
                        "/join order " << mg.propsOf(*pit).joinOrder << std::endl;
                }
                // model dependencies (succs)
                ModelSuccessorIterator sit, sbegin, send;
                boost::tie(sbegin, send) = mg.getSuccessors(m);
                for(sit = sbegin; sit != send; ++sit) {
                    o << indent <<
                        "<- input for model  " << mg.sourceOf(*sit) <<
                        "/join order " << mg.propsOf(*sit).joinOrder << std::endl;
                }
            }
            if( modelsAt.empty() )
                o << indent << toString(t) << " empty" << std::endl;
        }
    }
}


template<typename EvalGraphT>
void
OnlineModelBuilder<EvalGraphT>::printModelBuildingPropertyMap(std::ostream& o)
{
    o << "model building property map" << std::endl;
    typename std::vector<EvalUnitModelBuildingProperties>::const_iterator
        it, end;
    unsigned u = 0;
    it = mbp.storage_begin();
    end = mbp.storage_end();
    if( it == end ) {
        o << "empty" << std::endl;
    }
    else {
        for(; it != end; ++it, ++u) {
            const EvalUnitModelBuildingProperties& uprop = *it;
            o << " " << u << "=>" << printEUMBP(uprop) << std::endl;
        }
    }
}


template<typename EvalGraphT>
typename OnlineModelBuilder<EvalGraphT>::Model
OnlineModelBuilder<EvalGraphT>::createIModelFromPredecessorOModels(
EvalUnit u)
{
                                 // only called from within object -> do not log this ptr
    LOG_SCOPE(MODELB,"cIMfPOM",true);
    DBGLOG(DBG,"=OnlineModelBuilder<...>::createIModelFromPredecessorOModels(" << u << ")");

    // create vector of dependencies
    std::vector<Model> deps;
    typename EvalGraphT::PredecessorIterator pit, pend;
    boost::tie(pit, pend) = eg.getPredecessors(u);
    for(; pit != pend; ++pit) {
        EvalUnit pred = eg.targetOf(*pit);
        EvalUnitModelBuildingProperties& predmbprops = mbp[pred];
        LOG(MODELB,"found predecessor unit " << pred << " with current omodel mbprops: " << printEUMBP(predmbprops));
        Model predmodel = getOModel(predmbprops);
        deps.push_back(predmodel);
    }

    if( redundancyElimination ) {
        // check if there is an existing model created from these predecessors
        // if yes, just return this model
        OptionalModel oexisting = mg.getSuccessorIntersection(u, deps);
        if( !!oexisting ) {
            LOG(MODELB,"found and will return existing successor imodel " << oexisting.get());
            return oexisting.get();
        }
    }

    // create interpretation
    InterpretationPtr pjoin;
    if( deps.size() == 1 ) {
        // only link
        LOG(MODELB,"only one predecessor -> linking to omodel");
        pjoin = mg.propsOf(deps.front()).interpretation;
        assert(pjoin != 0);
    }
    else {
        // create joined interpretation
        LOG(MODELB,"more than one predecessor -> joining omodels");
        typename std::vector<Model>::const_iterator it;
        for(it = deps.begin(); it != deps.end(); ++it) {
            InterpretationPtr predinterpretation = mg.propsOf(*it).interpretation;
            DBGLOG(DBG,"predecessor omodel " << *it <<
                " has interpretation " << printptr(predinterpretation) <<
                " with contents " << *predinterpretation);
            assert(predinterpretation != 0);
            if( pjoin == 0 ) {
                // copy interpretation
                pjoin.reset(new Interpretation(*predinterpretation));
            }
            else {
                // merge interpretation
                pjoin->add(*predinterpretation);
            }
            DBGLOG(DBG,"pjoin now has contents " << *pjoin);
        }
    }

    // create model
    Model m = mg.addModel(u, MT_IN, deps);
    LOG(MODELB,"returning new MT_IN model " << m);
    mg.propsOf(m).interpretation = pjoin;
    return m;
}


// helper for advanceOModelForIModel
// TODO: comments from hexeval.tex
template<typename EvalGraphT>
boost::optional<typename OnlineModelBuilder<EvalGraphT>::EvalUnitPredecessorIterator>
OnlineModelBuilder<EvalGraphT>::ensureModelIncrement(
EvalUnit u, typename OnlineModelBuilder<EvalGraphT>::EvalUnitPredecessorIterator cursor)
{
    LOG_VSCOPE(MODELB,"eMI",u,true);
    #ifndef NDEBUG
    typename EvalGraphT::EvalUnit ucursor1 =
        eg.targetOf(*cursor);
    std::ostringstream dbgstr;
    dbgstr << "eMI[" << u << "," << ucursor1 << "]";
    DBGLOG_SCOPE(MODELB,dbgstr.str(),true);
    DBGLOG(DBG,"=OnlineModelBuilder<...>::ensureModelIncrement(" << u << "," << ucursor1 << ")");
    #endif

    EvalUnitPredecessorIterator pbegin, pend;
    boost::tie(pbegin, pend) = eg.getPredecessors(u);
    assert(pbegin != pend);
    do {
        typename EvalGraphT::EvalUnit ucursor =
            eg.targetOf(*cursor);
        #ifndef NDEBUG
        EvalUnitModelBuildingProperties& ucursor_mbprops =
            mbp[ucursor];
        DBGLOG(DBG,"ucursor = " << ucursor << " with mbprops = {" << printEUMBP(ucursor_mbprops) << "}");
        assert(ucursor_mbprops.hasOModel());
        assert(ucursor_mbprops.orefcount >= 1);
        #endif

        OptionalModel om = getNextOModel(ucursor);
        if( !om ) {
            if( cursor == pbegin ) {
                LOG(MODELB,"cannot advance previous, returning null cursor");
                return boost::none;
            }
            else {
                LOG(MODELB,"trying to advance previous");
                cursor--;
            }
        }
        else
            break;
    }
    while(true);

    #ifndef NDEBUG
    typename EvalGraphT::EvalUnit ucursor2 =
        eg.targetOf(*cursor);
    EvalUnitModelBuildingProperties& ucursor2_mbprops =
        mbp[ucursor2];
    DBGLOG(DBG,"returning cursor: unit = " << ucursor2 << " with mbprops = {" << printEUMBP(ucursor2_mbprops) << "}");
    assert(ucursor2_mbprops.hasOModel());
    #endif
    return cursor;
}


/*
 * TODO get documentation from hexeval.tex
 */
template<typename EvalGraphT>
typename OnlineModelBuilder<EvalGraphT>::OptionalModel
OnlineModelBuilder<EvalGraphT>::getNextIModel(
EvalUnit u)
{
    LOG_VSCOPE(MODELB,"gnIM",u,true);
    DBGLOG(DBG,"=OnlineModelBuilder<...>::getNextIModel(" << u << ")");

    #ifndef NDEBUG
    if( Logger::Instance().shallPrint(Logger::MODELB) && Logger::Instance().shallPrint(Logger::DBG) )
        printModelBuildingPropertyMap(std::cerr);
    const EvalUnitPropertyBundle& uprops = eg.propsOf(u);
    DBGLOG(DBG,"uprops: " << uprops);
    #endif

    EvalUnitModelBuildingProperties& mbprops = mbp[u];
    DBGLOG(DBG,"mbprops: " << printEUMBP(mbprops));

    // did we have an imodel upon function entry?
    bool hadIModel = !!mbprops.getIModel();

    // dummy handling for units without input
    if( !mbprops.needInput ) {
        DBGLOG(DBG,"unit needs no input");
        OptionalModel odummy;
        if( hadIModel ) {
            LOG(MODELB,"removing dummy model and failing");
            odummy = boost::none;
        }
        else {
            Model dummy;
            if( mg.modelsAt(u, MT_IN).empty() ) {
                dummy = mg.addModel(u, MT_IN);
                mg.propsOf(dummy).dummy = true;
                LOG(MODELB,"setting new dummy model " << dummy);
            }
            else {
                dummy = mg.modelsAt(u, MT_IN).front();
                LOG(MODELB,"setting existing dummy model " << dummy);
                assert(mg.propsOf(dummy).dummy);
            }
            odummy = dummy;
        }
        if( hadIModel && constantSpace )
            clearIModel(mbprops.getIModel().get());
        mbprops.setIModel(odummy);
        LOG(MODELB,"returning model " << printopt(odummy));
        #ifndef NDEBUG
        if( Logger::Instance().shallPrint(Logger::MODELB) && Logger::Instance().shallPrint(Logger::DBG) )
            printModelBuildingPropertyMap(std::cerr);
        #endif
        return odummy;
    }

    LOG(MODELB,"unit needs input");

    // prepare cursor handling
    typename EvalGraphT::PredecessorIterator pbegin, pend;
    typename EvalGraphT::PredecessorIterator cursor;
    boost::tie(pbegin, pend) = eg.getPredecessors(u);

    if( hadIModel ) {
        LOG(MODELB,"have imodel -> phase 1");
        boost::optional<EvalUnitPredecessorIterator> ncursor =
            ensureModelIncrement(u, pend - 1);
        if( !ncursor ) {
            LOG(MODELB,"got null cursor, returning no imodel");
            mbprops.setIModel(boost::none);
            #ifndef NDEBUG
            if( Logger::Instance().shallPrint(Logger::MODELB) && Logger::Instance().shallPrint(Logger::DBG) )
                printModelBuildingPropertyMap(std::cerr);
            #endif
            return boost::none;
        }
        else {
            LOG(MODELB,"got some increment");
            cursor = ncursor.get();
        }
        // if( cursor == (pend - 1) )
        // "cursor++;" will increment it to pend
        // phase 2 loop will not be executed
        // model will be created and returned
        cursor++;
    }
    else {
        cursor = pbegin;
    }

    // now, cursor is index of first unit where we do not hold a refcount
    LOG(MODELB,"phase 2");

    while(cursor != pend) {
        typename EvalGraphT::EvalUnit ucursor =
            eg.targetOf(*cursor);
        EvalUnitModelBuildingProperties& ucursor_mbprops =
            mbp[ucursor];
        if( ucursor_mbprops.hasOModel() ) {
            LOG(MODELB,"predecessor " << ucursor <<
                " has omodel " << mg.sourceOf(*ucursor_mbprops.currentisuccessor.get()) <<
                " with refcount " << ucursor_mbprops.orefcount);
            ucursor_mbprops.orefcount++;
        }
        else {
            LOG(MODELB,"predecessor " << ucursor << " has no omodel");
            OptionalModel om = getNextOModel(ucursor);
            LOG(MODELB,"got next omodel " << printopt(om) << " at unit " << ucursor);
            if( !om ) {
                if( cursor == pbegin ) {
                    LOG(MODELB,"backtracking impossible, returning no imodel");
                    mbprops.setIModel(boost::none);
                    #ifndef NDEBUG
                    if( Logger::Instance().shallPrint(Logger::MODELB) && Logger::Instance().shallPrint(Logger::DBG) )
                        printModelBuildingPropertyMap(std::cerr);
                    #endif
                    return boost::none;
                }
                else {
                    LOG(MODELB,"backtracking possible");
                    boost::optional<EvalUnitPredecessorIterator> ncursor =
                        ensureModelIncrement(u, cursor - 1);
                    if( !ncursor ) {
                        LOG(MODELB,"got null cursor, returning no imodel");
                        mbprops.setIModel(boost::none);
                        #ifndef NDEBUG
                        if( Logger::Instance().shallPrint(Logger::MODELB) && Logger::Instance().shallPrint(Logger::DBG) )
                            printModelBuildingPropertyMap(std::cerr);
                        #endif
                        return boost::none;
                    }
                    else {
                        LOG(MODELB,"backtracking was successful");
                        cursor = ncursor.get();
                    }
                }
            }
        }
        cursor++;
    }                            // while(cursor != pend)

    LOG(MODELB,"found full input model, creating imodel!");
    Model im = createIModelFromPredecessorOModels(u);
    LOG(MODELB,"returning newly created imodel " << im);
    if( hadIModel && constantSpace )
        clearIModel(mbprops.getIModel().get());
    mbprops.setIModel(im);
    #ifndef NDEBUG
    if( Logger::Instance().shallPrint(Logger::MODELB) && Logger::Instance().shallPrint(Logger::DBG) )
        printModelBuildingPropertyMap(std::cerr);
    #endif
    return im;
}


// [checks if model generation is still possible given current input model]
// [checks if no model is currently stored as current omodel]
// if no model generator is running
//   determines input interpretation
//   start model generator
// get next model from model generator
// if successful
//   create in model graph
//   return model
// else
//   set finished for model generation
//   return null
template<typename EvalGraphT>
typename OnlineModelBuilder<EvalGraphT>::OptionalModel
OnlineModelBuilder<EvalGraphT>::createNextModel(
EvalUnit u)
{
    LOG_VSCOPE(MODELB,"cNM",u,true);
    DBGLOG(DBG,"=createNextModel(" << u << ")");

    EvalUnitModelBuildingProperties& mbprops = mbp[u];

    #ifndef NDEBUG
    // check if there can be a next model
    assert(!!mbprops.getIModel());
    assert(!mg.propsOf(mbprops.getIModel().get()).childModelsGenerated);
    assert(!mbprops.currentisuccessor);
    assert(mbprops.orefcount == 0);
    #endif

    if( !mbprops.currentmg ) {
        LOG(MODELB,"no model generator running");

        // determine input
        typename Interpretation::ConstPtr input;
        // input for creating model comes from current imodel
        // (this may be a dummy, so interpretation may be NULL which is ok)
        input = mg.propsOf(mbprops.getIModel().get()).interpretation;

        // mgf is of type ModelGeneratorFactory::Ptr
        LOG(MODELB,"creating model generator");
        mbprops.currentmg =
            eg.propsOf(u).mgf->createModelGenerator(input)
            ;
    }

    // use model generator to create new model
    DBGLOG(MODELB,"generating next model");
    assert(mbprops.currentmg);
    InterpretationPtr intp =
        mbprops.currentmg->generateNextModel();

    if( intp ) {
        // create model
        std::vector<Model> deps;
        deps.push_back(mbprops.getIModel().get());
        Model m = mg.addModel(u, MT_OUT, deps);
        // we got a new model
        LOG(MODELB,"stored new model " << m);

        // configure model
        mg.propsOf(m).interpretation = intp;

        // TODO: handle projection here?
        #ifndef NDEBUG
        const EvalUnitPropertyBundle& uprops = eg.propsOf(u);
        #endif
        assert(uprops.iproject == false);
        assert(uprops.oproject == false);

        LOG(MODELB,"setting currentisuccessor iterator");
        ModelSuccessorIterator sbegin, send;
        boost::tie(sbegin, send) = mg.getSuccessors(mbprops.getIModel().get());
        /*{
          for(ModelSuccessorIterator it = sbegin; it != send; ++it)
          {
            LOG("found successor " << mg.sourceOf(*it));
          }
        }*/
        ModelSuccessorIterator sit = send;
        sit--;
        assert(mg.sourceOf(*sit) == m);
        mbprops.currentisuccessor = sit;

        LOG(MODELB,"setting refcount to 1");
        mbprops.orefcount = 1;
        LOG(MODELB,"returning model " << m);
        return m;
    }
    else {
        // no further models for this model generator
        LOG(MODELB,"no further model");

        // mark this input model as finished for creating models
        ModelPropertyBundle& imodelprops = mg.propsOf(mbprops.getIModel().get());
        imodelprops.childModelsGenerated = true;

        // free model generator
        mbprops.currentmg.reset();
        LOG(MODELB,"returning no model");
        return boost::none;
    }
}


/**
 * nonrecursive "get next" wrt. a mandatory imodel
 *
 * two situations:
 * 1) all omodels for that imodel have been generated
 *    -> use model graph only
 * 2) otherwise:
 *   a) no model has been generated (-> no currentmg)
 *      -> start model generator and get first model
 *   b) some models have been generated (-> currentmg)
 *      -> continue to use model generator currentmg
 *
 * our strategy is as follows:
 * advance on model graph if possible
 * if this yields no model and not all models have been generated
 *   if no model generator is running, start one
 *   use model generator
 */
template<typename EvalGraphT>
typename OnlineModelBuilder<EvalGraphT>::OptionalModel
OnlineModelBuilder<EvalGraphT>::advanceOModelForIModel(
EvalUnit u)
{
    LOG_VSCOPE(MODELB,"aOMfIM",u,true);
    DBGLOG(DBG,"=OnlineModelBuilder<...>::advanceOModelForIModel(" << u << ")");

    // prepare
    EvalUnitModelBuildingProperties& mbprops = mbp[u];
    assert(mbprops.orefcount <= 1);
    assert(!!mbprops.getIModel());

    // get imodel + properties
                                 // Model == void* -> no ref!
    Model imodel = mbprops.getIModel().get();
    ModelPropertyBundle& imodelprops = mg.propsOf(imodel);
    LOG(MODELB,"have imodel " << imodel);
    DBGLOG(DBG,"imodel has properties " << print_method(imodelprops));

    // get successor list of imodel
    ModelSuccessorIterator sbegin, send;
    boost::tie(sbegin, send) = mg.getSuccessors(imodel);
    if( sbegin != send )
        LOG(MODELB,"imodel has at least one successor");

    LOG(MODELB,"trying to advance on model graph");
    if( !!mbprops.currentisuccessor ) {
        LOG(MODELB,"currentisuccessor is set");
        assert(mbprops.orefcount == 1);

        ModelSuccessorIterator& currentisuccessor = mbprops.currentisuccessor.get();
        assert(currentisuccessor != send);
        if( constantSpace )
            clearOModel(currentisuccessor);
        currentisuccessor++;
        if( currentisuccessor != send ) {
            Model m = mg.sourceOf(*currentisuccessor);
            LOG(MODELB,"advance successful, returning model " << m);
            return m;
        }
        else {
            LOG(MODELB,"resetting iterator");
            // reset iterator here because we cannot be sure that it can
            // point to a "current" model anymore, and we need to set it anew
            // anyways in case we create a new model below
            mbprops.currentisuccessor = boost::none;
            mbprops.orefcount = 0;
        }
    }
    else {
        LOG(MODELB,"currentisuccessor not set");
        assert(mbprops.orefcount == 0);

        if( sbegin != send ) {
            LOG(MODELB,"there are successors -> using them");
            mbprops.currentisuccessor = sbegin;
            mbprops.orefcount++;
            assert(mbprops.orefcount == 1);
            Model m = mg.sourceOf(*sbegin);
            LOG(MODELB,"returning first successor model " << m);
            return m;
        }
    }

    // here we know: we cannot advance on the model graph
    LOG(MODELB,"advancing on model graph failed");
    assert(!mbprops.currentisuccessor);
    assert(mbprops.orefcount == 0);

    if( imodelprops.childModelsGenerated ) {
        LOG(MODELB,"all successors created -> returning no model");
        return boost::none;
    }

    // here, not all models have been generated
    // -> create model generator if not existing
    // -> use model generator

    DBGLOG(MODELB,"attempting to create new model");
    OptionalModel m = createNextModel(u);
    LOG(MODELB,"returning model " << printopt(m));
    return m;
}


// get next output model (projected if projection is configured) at unit u
template<typename EvalGraphT>
typename OnlineModelBuilder<EvalGraphT>::OptionalModel
OnlineModelBuilder<EvalGraphT>::getNextOModel(
EvalUnit u)
{
    LOG_VSCOPE(MODELB,"gnOM",u,true);
    DBGLOG(DBG,"=OnlineModelBuilder<...>::getNextOModel(" << u << "):");

    #ifndef NDEBUG
    const EvalUnitPropertyBundle& uprops = eg.propsOf(u);
    if( Logger::Instance().shallPrint(Logger::MODELB) && Logger::Instance().shallPrint(Logger::DBG) )
        printModelBuildingPropertyMap(std::cerr);
    DBGLOG(DBG,"uprops = " << uprops);
    #endif

    EvalUnitModelBuildingProperties& mbprops = mbp[u];
    DBGLOG(DBG,"mbprops = " << printEUMBP(mbprops));

    // are we allowed to go to the next model here?
    if( mbprops.orefcount > 1 ) {
        LOG(MODELB,"not allowed to continue because of orefcount > 1");
        // no -> give up our model refcount and return no model at all
        mbprops.orefcount--;
        #ifndef NDEBUG
        if( Logger::Instance().shallPrint(Logger::MODELB) && Logger::Instance().shallPrint(Logger::DBG) )
            printModelBuildingPropertyMap(std::cerr);
        #endif
        return OptionalModel();
    }

    // initialization?
    if( !mbprops.getIModel() ) {
        LOG(MODELB,"getting next imodel (none present and we need one)");
        assert(mbprops.orefcount == 0);
        // get next input for this unit (stores into mprops.imodel)
        getNextIModel(u);
        assert(!mbprops.currentisuccessor);
    }

    OptionalModel omodel;
    do {
        // fail if there is no input at this point
        if( !mbprops.getIModel() ) {
            LOG(MODELB,"failing with no input");
            assert(mbprops.orefcount == 0);
            #ifndef NDEBUG
            if( Logger::Instance().shallPrint(Logger::MODELB) && Logger::Instance().shallPrint(Logger::DBG) )
                printModelBuildingPropertyMap(std::cerr);
            #endif
            return boost::none;
        }

        LOG(MODELB,"advancing omodel");
        // advance omodel, maybe advance to null model
        // advancing is only allowed if orefcount <= 1
        omodel = advanceOModelForIModel(u);
        if( !omodel ) {
            LOG(MODELB,"no omodel and have input models -> advancing imodel");
            // no next omodel found
            // -> advance imodel (stores into mbprops.imodel)
            getNextIModel(u);
        }
    }
    while( !omodel );
    assert(mbprops.orefcount == 1);
    LOG(MODELB,"returning omodel " << printopt(omodel));
    #ifndef NDEBUG
    if( Logger::Instance().shallPrint(Logger::MODELB) && Logger::Instance().shallPrint(Logger::DBG) )
        printModelBuildingPropertyMap(std::cerr);
    #endif
    return omodel;
}


DLVHEX_NAMESPACE_END
#endif                           // ONLINE_MODEL_BUILDER_HPP_INCLUDED__23092010
