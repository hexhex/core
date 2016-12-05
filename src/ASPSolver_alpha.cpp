/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005-2007 Roman Schindlauer
 * Copyright (C) 2006-2015 Thomas Krennwallner
 * Copyright (C) 2009-2016 Peter Schüller
 * Copyright (C) 2011-2016 Christoph Redl
 * Copyright (C) 2015-2016 Tobias Kaminski
 * Copyright (C) 2015-2016 Antonius Weinzierl
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
 * @file ASPSolver_alpha.cpp
 * @author Tobias Kaminski
 *
 * @brief Alpha ASP solver integration
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_ALPHA

#include "dlvhex2/AlphaModelGenerator.h"
#include "dlvhex2/ASPSolver.h"
#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/Benchmarking.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/AnswerSet.h"

#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/foreach.hpp>
#include <list>
#include <jni.h>

DLVHEX_NAMESPACE_BEGIN

        namespace ASPSolver {
    //
    // AlphaSoftware
    //

    AlphaSoftware::Options::Options() :
    ASPSolverManager::GenericOptions(),
    arguments() {
        //arguments.push_back("-silent");

        DBGLOG(DBG, "starting java virtual machine");

        JavaVMInitArgs vm_args;
        JavaVMOption* jvm_options = new JavaVMOption[1];
        static JNIEnv *env;

        jvm_options[0].optionString = HAVE_ALPHA;
        vm_args.version = JNI_VERSION_1_8;
        vm_args.nOptions = 1;
        vm_args.options = jvm_options;
        vm_args.ignoreUnrecognized = JNI_TRUE;

        status = JNI_CreateJavaVM(&jvm, (void**) &env, &vm_args);

        if (status != JNI_ERR) {
            DBGLOG(DBG, "started java virtual machine");
        } else {
            throw FatalError(" error when loading alpha in jvm, make sure path is correct");
        }

        cls = env->FindClass("at/ac/tuwien/kr/alpha/Main");
        cls = reinterpret_cast<jclass> (env->NewGlobalRef(cls));

        JNINativeMethod methods[]{
            { "sendResults", "([[Ljava/lang/String;)V", (void *) &sendResultsCPP},
            { "externalAtomsQuery", "([Ljava/lang/String;[Ljava/lang/String;)[[Ljava/lang/String;", (jobjectArray *) & externalAtomsQuery}
        };

        if (env->RegisterNatives(cls, methods, 2) < 0) {
            if (env->ExceptionOccurred())
                throw FatalError(" exception when registering natives");
            else
                throw FatalError(" ERROR: problem when registering natives");
        }

        mid = env->GetStaticMethodID(cls, "main", "([Ljava/lang/String;)V");

        arr = env->NewObjectArray(2,
                env->FindClass("java/lang/String"),
                env->NewStringUTF("str"));
        env->SetObjectArrayElement(arr, 0, env->NewStringUTF("-str"));

        jvm->DetachCurrentThread();
    }

    AlphaSoftware::Options::~Options() {

    }

    struct AlphaSoftware::Delegate::PreparedResultsImpl :
    public PreparedResults {
    public:
        Options options;
        RegistryPtr reg;
        InterpretationConstPtr mask;
    public:

        PreparedResultsImpl(const Options& options) :
        PreparedResults(),
        options(options) {
            DBGLOG(DBG, "AlphaSoftware::Delegate::PreparedResultsImpl()" << this);
        }

        virtual ~PreparedResultsImpl() {
            DBGLOG(DBG, "AlphaSoftware::Delegate::~PreparedResultsImpl()" << this);
        }

        void answerSetProcessingFunc();

        void getAnswerSets(std::string program_str) {
            if (options.cls != 0) {

                static JNIEnv *env;

                int getEnvStat = options.jvm->GetEnv((void **) &env, JNI_VERSION_1_8);
                if (getEnvStat == JNI_EDETACHED) {
                    DBGLOG(DBG, "[" << this << "]" << "GetEnv: thread not attached to jvm");
                    if (options.jvm->AttachCurrentThread((void **) &env, NULL) != 0) {
                        throw FatalError("failed to attached thread to jvm");
                    }
                } else if (getEnvStat == JNI_EVERSION) {
                    DBGLOG(DBG, "[" << this << "]" << "GetEnv: jvm version not supported");
                }

                if (options.mid == nullptr) {
                    throw FatalError("ERROR: method not found!");
                } else {
                    jstring argument = env->NewStringUTF(program_str.c_str());
                    env->SetObjectArrayElement(options.arr, 1, argument);
                    env->CallStaticVoidMethod(options.cls, options.mid, options.arr);
                    env->DeleteLocalRef(argument);
                }
                if (getEnvStat == JNI_EDETACHED) {
                    options.jvm->DetachCurrentThread();
                }
            }
        }
    };

    void AlphaSoftware::Delegate::PreparedResultsImpl::answerSetProcessingFunc() {
        DBGLOG(DBG, "[" << this << "]" " starting alpha answerSetProcessingFunc");

        for (std::vector<std::vector < std::string>>::iterator it1 = answerSets.begin(); it1 != answerSets.end(); ++it1) {
            AnswerSet::Ptr as(new AnswerSet(reg));
            for (std::vector<std::string>::iterator it2 = it1->begin(); it2 != it1->end(); ++it2) {
                const char* groundatom = it2->c_str();

                ID idga = reg->ogatoms.getIDByString(groundatom);
                if (idga == ID_FAIL) {
                    // parse groundatom, register and store
                    DBGLOG(DBG, "parsing ground atom '" << groundatom << "'");
                    OrdinaryAtom ogatom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
                    ogatom.text = groundatom;
                    // create ogatom.tuple
                    boost::char_separator<char> sep(",()");
                    typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
                    tokenizer tok(ogatom.text, sep);
                    for (tokenizer::iterator it = tok.begin();
                            it != tok.end(); ++it) {
                        DBGLOG(DBG, "got token '" << *it << "'");
                        Term term(ID::MAINKIND_TERM, *it);
                        // the following takes care of int vs const/string
                        ID id = reg->storeTerm(term);
                        assert(id != ID_FAIL);
                        assert(!id.isVariableTerm());
                        if (id.isAuxiliary())
                            ogatom.kind |= ID::PROPERTY_AUX;
                        ogatom.tuple.push_back(id);
                    }
                    idga = reg->ogatoms.storeAndGetID(ogatom);
                }
                assert(idga != ID_FAIL);
                as->interpretation->setFact(idga.address);
            }
            if (mask)
                as->interpretation->getStorage() -= mask->getStorage();
            add(as);
        }

        DBGLOG(DBG, "[" << this << "]" "exiting answerSetProcessingThreadFunc");
    }

    /////////////////////////////////////
    // AlphaSoftware::Delegate::Delegate //
    /////////////////////////////////////

    AlphaSoftware::Delegate::Delegate(const Options& options) :
    results(new PreparedResultsImpl(options)) {
        // keep global reference for callback from jvm
        ASPSolver::delegatePointer = this;
    }

    AlphaSoftware::Delegate::~Delegate() {
    }

    void
    AlphaSoftware::Delegate::useInputProviderInput(InputProvider& inp, RegistryPtr reg) {
        throw std::runtime_error("TODO implement AlphaSoftware::Delegate::useInputProviderInput(const InputProvider& inp, RegistryPtr reg)");
    }

    void
    AlphaSoftware::Delegate::useASTInput(const OrdinaryASPProgram& program) {
        DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "AlphaSoftw:Delegate:useASTInput");

        results->reg = program.registry;
        assert(results->reg);
        results->mask = program.mask;

        std::string program_str;

        std::ostringstream programStream;
        // output program
        RawPrinter printer(programStream, program.registry);

        if (program.edb != 0) {
            // print edb interpretation as facts
            program.edb->printAsFacts(programStream);
            programStream << "\n";
            programStream.flush();
        }

        printer.printmany(program.idb, "\n");
        programStream << std::endl;
        programStream.flush();
        program_str = programStream.str();

        results->getAnswerSets(program_str);
        results->answerSetProcessingFunc();
    }

    ASPSolverManager::ResultsPtr
    AlphaSoftware::Delegate::getResults() {
        DBGLOG(DBG, "AlphaSoftware::Delegate::getResults");
        return results;
    }

    JNIEXPORT jobjectArray JNICALL externalAtomsQuery(JNIEnv *env, jclass o, jobjectArray trueAtoms, jobjectArray falseAtoms) {
        InterpretationPtr currentIntr = InterpretationPtr(new Interpretation(ASPSolver::delegatePointer->results->reg));
        InterpretationPtr currentAssigned = InterpretationPtr(new Interpretation(ASPSolver::delegatePointer->results->reg));

        int trueLength = env->GetArrayLength(trueAtoms);
        int falseLength = env->GetArrayLength(falseAtoms);

        for (int i = 0; i < trueLength; i++) {
            jstring result = (jstring) (env->GetObjectArrayElement(trueAtoms, i));

            const char *nativeResult = env->GetStringUTFChars(result, JNI_FALSE);

            std::string trueAtom(nativeResult);

            const char* groundatom = trueAtom.c_str();

            ID idga = ASPSolver::delegatePointer->results->reg->ogatoms.getIDByString(groundatom);
            if (idga == ID_FAIL) {
                OrdinaryAtom ogatom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
                ogatom.text = groundatom;
                boost::char_separator<char> sep(",()");
                typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
                tokenizer tok(ogatom.text, sep);
                for (tokenizer::iterator it = tok.begin();
                        it != tok.end(); ++it) {
                    Term term(ID::MAINKIND_TERM, *it);
                    ID id = ASPSolver::delegatePointer->results->reg->storeTerm(term);
                    assert(id != ID_FAIL);
                    assert(!id.isVariableTerm());
                    if (id.isAuxiliary())
                        ogatom.kind |= ID::PROPERTY_AUX;
                    ogatom.tuple.push_back(id);
                }
                idga = ASPSolver::delegatePointer->results->reg->ogatoms.storeAndGetID(ogatom);
            }
            assert(idga != ID_FAIL);
            currentIntr->setFact(idga.address);
            currentAssigned->setFact(idga.address);

            env->ReleaseStringUTFChars(result, nativeResult);
        }

        for (int i = 0; i < falseLength; i++) {
            jstring result = (jstring) (env->GetObjectArrayElement(falseAtoms, i));

            const char *nativeResult = env->GetStringUTFChars(result, JNI_FALSE);

            std::string falseAtom(nativeResult);

            const char* groundatom = falseAtom.c_str();

            ID idga = ASPSolver::delegatePointer->results->reg->ogatoms.getIDByString(groundatom);
            if (idga == ID_FAIL) {
                OrdinaryAtom ogatom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
                ogatom.text = groundatom;
                boost::char_separator<char> sep(",()");
                typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
                tokenizer tok(ogatom.text, sep);
                for (tokenizer::iterator it = tok.begin();
                        it != tok.end(); ++it) {
                    Term term(ID::MAINKIND_TERM, *it);
                    ID id = ASPSolver::delegatePointer->results->reg->storeTerm(term);
                    assert(id != ID_FAIL);
                    assert(!id.isVariableTerm());
                    if (id.isAuxiliary())
                        ogatom.kind |= ID::PROPERTY_AUX;
                    ogatom.tuple.push_back(id);
                }
                idga = ASPSolver::delegatePointer->results->reg->ogatoms.storeAndGetID(ogatom);
            }
            assert(idga != ID_FAIL);
            currentAssigned->setFact(idga.address);

            env->ReleaseStringUTFChars(result, nativeResult);
        }


        std::cout << *currentIntr << std::endl;
        std::cout << *currentAssigned << std::endl;

        SimpleNogoodContainerPtr nogoods = SimpleNogoodContainerPtr(new SimpleNogoodContainer());
        AlphaModelGenerator::IntegrateExternalAnswerIntoInterpretationCB cb(currentIntr);

        BOOST_FOREACH(ID eatomid, amgPointer->factory.innerEatoms) {
            amgPointer->evaluateExternalAtomFacade(amgPointer->factory.ctx,
                    eatomid, currentIntr, cb, nogoods, currentAssigned);
        }

        jobjectArray extResults = env->NewObjectArray(nogoods->getNogoodCount(),
                env->FindClass("java/lang/Object"),
                env->NewObjectArray(2,
                env->FindClass("java/lang/String"),
                env->NewStringUTF("str")));

        int ngIndex = 0;

        for (int k = 0; k < nogoods->getNogoodCount(); ++k) {
            jobjectArray ioNogood = env->NewObjectArray(nogoods->getNogood(k).size(),
                    env->FindClass("java/lang/String"),
                    env->NewStringUTF("str"));

            int arrIndex = 0;

            BOOST_FOREACH(ID& iid, nogoods->getNogood(k)) {
                if (amgPointer->factory.ctx.registry()->ogatoms.getIDByAddress(iid.address).isExternalAuxiliary()) {
                    std::stringstream ss;
                    RawPrinter printer(ss, amgPointer->factory.ctx.registry());
                    ss << (iid.isNaf() ? "-" : "");
                    printer.print(iid);

                    env->SetObjectArrayElement(ioNogood, arrIndex, env->NewStringUTF(ss.str().c_str()));
                    arrIndex++;
                }
            }

            BOOST_FOREACH(ID& iid, nogoods->getNogood(k)) {
                if (!amgPointer->factory.ctx.registry()->ogatoms.getIDByAddress(iid.address).isExternalAuxiliary()) {
                    std::stringstream ss;
                    RawPrinter printer(ss, amgPointer->factory.ctx.registry());
                    ss << (iid.isNaf() ? "-" : "");
                    printer.print(iid);

                    env->SetObjectArrayElement(ioNogood, arrIndex, env->NewStringUTF(ss.str().c_str()));
                    arrIndex++;
                }
            }

            env->SetObjectArrayElement(extResults, ngIndex, ioNogood);
            ngIndex++;
        }

        return extResults;
    }

    JNIEXPORT void JNICALL sendResultsCPP(JNIEnv *env, jclass o, jobjectArray resultsArray) {
        int answerSetCount = env->GetArrayLength(resultsArray);

        answerSets.clear();

        for (int i = 0; i < answerSetCount; i++) {
            jobjectArray answerSet = (jobjectArray) env->GetObjectArrayElement(resultsArray, i);

            int answerSetSize = env->GetArrayLength(answerSet);

            std::vector<std::string> answerSetVec;

            for (int j = 0; j < answerSetSize; j++) {
                jstring result = (jstring) (env->GetObjectArrayElement(answerSet, j));

                const char *nativeResult = env->GetStringUTFChars(result, JNI_FALSE);

                std::string answerSetAtom(nativeResult);
                answerSetVec.push_back(answerSetAtom);

                env->ReleaseStringUTFChars(result, nativeResult);
            }

            answerSets.push_back(answerSetVec);
        }
    }

} // namespace ASPSolver

DLVHEX_NAMESPACE_END
#endif                           // HAVE_ALPHA


// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
