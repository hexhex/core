/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2007, 2008 Thomas Krennwallner
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
 * @file State.cpp
 * @author Thomas Krennwallner
 * @date 
 *
 * @brief State class.
 *
 *
 *
 */


#include "dlvhex/State.h"

#include "dlvhex/ProgramCtx.h"
#include "dlvhex/GraphProcessor.h"
#include "dlvhex/GraphBuilder.h"
#include "dlvhex/ComponentFinder.h"
#include "dlvhex/BoostComponentFinder.h"
#include "dlvhex/globals.h"
#include "dlvhex/Error.h"
#include "dlvhex/ResultContainer.h"
#include "dlvhex/OutputBuilder.h"
#include "dlvhex/TextOutputBuilder.h"
#include "dlvhex/SafetyChecker.h"
#include "dlvhex/HexParserDriver.h"
#include "dlvhex/PrintVisitor.h"
#include "dlvhex/PluginContainer.h"
#include "dlvhex/DependencyGraph.h"
#include "dlvhex/GraphProcessor.h"
#include "dlvhex/Component.h"
#include "dlvhex/URLBuf.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>


DLVHEX_NAMESPACE_BEGIN


void
State::changeState(ProgramCtx* ctx, const boost::shared_ptr<State>& s)
{
  ctx->changeState(s);
}

void
State::openPlugins(ProgramCtx*)
{ }

void
State::convert(ProgramCtx*)
{ }

void
State::parse(ProgramCtx*)
{ }

void
State::rewrite(ProgramCtx*)
{ }

void
State::createNodeGraph(ProgramCtx*)
{ }

void
State::optimize(ProgramCtx*)
{ }


void
State::createDependencyGraph(ProgramCtx*)
{ }

void
State::safetyCheck(ProgramCtx*)
{ }

void
State::strongSafetyCheck(ProgramCtx*)
{ }

void
State::setupProgramCtx(ProgramCtx*)
{ }

void
State::evaluate(ProgramCtx*)
{ }

void
State::postProcess(ProgramCtx*)
{ }

void
State::output(ProgramCtx*)
{ }


void
OpenPluginsState::openPlugins(ProgramCtx* ctx)
{
  DEBUG_START_TIMER;

  PluginContainer* container = ctx->getPluginContainer();

  std::vector<PluginInterface*> plugins = container->importPlugins();
  ctx->addPlugins(plugins);

  //
  // set options in the found plugins
  //
  for (std::vector<PluginInterface*>::const_iterator pi = ctx->getPlugins()->begin();
       pi != ctx->getPlugins()->end(); ++pi)
    {
      PluginInterface* plugin = *pi;
	  
      if (plugin != 0)
	{
	  // print plugin's version information
	  if (!Globals::Instance()->getOption("Silent"))
	    {
	      Globals::Instance()->getVerboseStream() << "opening "
						      << plugin->getPluginName()
						      << " version "
						      << plugin->getVersionMajor() << "."
						      << plugin->getVersionMinor() << "."
						      << plugin->getVersionMicro() << std::endl;
	    }

	  std::stringstream pluginhelp;
			  
	  plugin->setOptions(Globals::Instance()->getOption("HelpRequested"), *ctx->getOptions(), pluginhelp);

	  if (!pluginhelp.str().empty())
	    {
	      Globals::Instance()->getVerboseStream() << std::endl << pluginhelp.str();
	    }
	}
    }
  
  boost::shared_ptr<State> next(new ConvertState);
  changeState(ctx, next);

  //                123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-
  DEBUG_STOP_TIMER("Importing plugins:                      ");
}


void
ConvertState::convert(ProgramCtx* ctx)
{
  DEBUG_START_TIMER;

  ///@todo move file/uri opening into its own state
  
  const std::vector<std::string>& allFiles = ctx->getInputSources();
  assert(allFiles.size() > 0);

  //
  // store filename of (first) logic program, we might use this somewhere
  // else (e.g., when writing the graphviz file in the boost-part
  //
  const std::string& lpfile = *allFiles.begin();
  Globals::Instance()->lpfilename = lpfile.substr(lpfile.find_last_of("/") + 1) + ".dot";

  ///@todo hm, maybe boost::iostream::chain helps???
  for (std::vector<std::string>::const_iterator f = allFiles.begin(); f != allFiles.end(); ++f)
    {
      //
      // stream to store the url/file/stdin content
      //
      std::stringstream tmpin;

      // URL
      if (f->find("http://") == 0)
	{
	  URLBuf ubuf;
	  ubuf.open(*f);
	  std::istream is(&ubuf);
	  
	  tmpin << is.rdbuf();
	  
	  if (ubuf.responsecode() == 404)
	    {
	      throw GeneralError("Requested URL " + *f + " was not found");
	    }
	}
      else if (*f == "--") // stdin requested
	{
	  // copy stdin
	  tmpin << std::cin.rdbuf();
	}
      else // file
	{
	  std::ifstream ifs;
	      
	  ifs.open(f->c_str());
	      
	  if (!ifs.is_open())
	    {
	      throw GeneralError("File " + *f + " not found");
	    }
	      
	  tmpin << ifs.rdbuf();
	  ifs.close();
	}

      //
      // create a stringbuffer on the heap (will be deleted later) to
      // hold the file-content. put it into the context input
      //	
      ctx->getInput().rdbuf(new std::stringbuf(tmpin.str()));
      
      //
      // new output stream with stringbuffer on the heap
      //
      std::ostream converterResult(new std::stringbuf);
      
      for (std::vector<PluginInterface*>::iterator pi = ctx->getPlugins()->begin();
	   pi != ctx->getPlugins()->end();
	   ++pi)
	{
	  std::vector<PluginConverter*> pcs = (*pi)->createConverters();
	  
	  if (pcs.size() > 0)
	    {
	      //
	      // go through all converters and rewrite input to converterResult
	      //
	      for (std::vector<PluginConverter*>::iterator it = pcs.begin();
		   it != pcs.end(); ++it)
		{
		  (*it)->convert(ctx->getInput(), converterResult);
		  
		  //
		  // old input buffer can be deleted now
		  //
		  delete ctx->getInput().rdbuf();
		  
		  //
		  // store the current output buffer
		  //
		  std::streambuf* tmp = converterResult.rdbuf();
		  
		  //
		  // make a new buffer for the output (=reset the output)
		  //
		  converterResult.rdbuf(new std::stringbuf);
		  
		  //
		  // set the input buffer to be the output of the last
		  // rewriting. now, after each loop, the converted
		  // program is in input.
		  //
		  ctx->getInput().rdbuf(tmp);
		}
	    }
	}

      // result of last converter can be removed now
      delete converterResult.rdbuf();

      
      //
      // at this point, the whole program is in the context input stream - either
      // directly read from the file or as a result of some previous
      // rewriting!
      //

      ///@todo move dlt code outside!
      
// 	  FILE* fp = 0;

// 	  //
// 	  // now call dlt if needed
// 	  //
// 	  if (optiondlt)
// 	    {
// 	      char tempfile[L_tmpnam];
// 	      mkstemp(tempfile);
	      
// 	      std::ofstream dlttemp(tempfile);
	      
// 	      //
// 	      // write program into tempfile
// 	      //
// 	      dlttemp << input.rdbuf();
			    
// 	      dlttemp.close();

// 	      std::string execPreParser("dlt -silent -preparsing " + std::string(tempfile));
			    
// 	      fp = popen(execPreParser.c_str(), "r");
			    
// 	      if (fp == NULL)
// 		{
// 		  throw GeneralError("Unable to call Preparser dlt");
// 		}

// 	      __gnu_cxx::stdio_filebuf<char>* fb;
// 	      fb = new __gnu_cxx::stdio_filebuf<char>(fp, std::ios::in);
			    
// 	      std::istream inpipe(fb);

// 	      //
// 	      // now we have a program rewriten by dlt - since it should
// 	      // be in the stream "input", we have to delete the old
// 	      // input-buffer and set input to the buffer from the
// 	      // dlt-call
// 	      //
// 	      delete input.rdbuf(); 
// 	      input.rdbuf(fb);
// 	    }
    }

  boost::shared_ptr<State> next(new ParseState);
  changeState(ctx, next);
  
  //                123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-
  DEBUG_STOP_TIMER("Calling plugin converters:              ");
}


void
ParseState::parse(ProgramCtx* ctx)
{
  DEBUG_START_TIMER;

  HexParserDriver driver;
	      
  //
  // tell the parser driver where the rules are actually coming
  // from (needed for error-messages)
  //
  driver.setOrigin(*ctx->getInputSources().begin());

  // run the parser
  driver.parse(ctx->getInput(), *ctx->getIDB(), *ctx->getEDB());

  ///@todo move dlt code outside
//       if (optiondlt)
// 	{
// 	  int dltret = pclose(fp);
	  
// 	  if (dltret != 0)
// 	    {
// 	      throw GeneralError("Preparser dlt returned error");
// 	    }
// 	}
	
  //
  // wherever the input-buffer was created before - now we don't
  // need it anymore
  //
  delete ctx->getInput().rdbuf();
      
//      if (optiondlt)
// 	{
// 	  unlink(tempfile);
// 	}

  boost::shared_ptr<State> next(new RewriteState);
  changeState(ctx, next);

  //                123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-
  DEBUG_STOP_TIMER("Parsing input:                          ");
}


void
RewriteState::rewrite(ProgramCtx* ctx)
{
  DEBUG_START_TIMER;
  
  //
  // now call rewriters
  //
  for (std::vector<PluginInterface*>::iterator pi = ctx->getPlugins()->begin();
       pi != ctx->getPlugins()->end();
       ++pi)
    {
      PluginRewriter* pr = (*pi)->createRewriter();

      if (pr != 0)
	{
	  pr->rewrite(*ctx->getIDB(), *ctx->getEDB());
	}
    }

  boost::shared_ptr<State> next;

  if (ctx->getNodeGraph() == 0)
    {
      // no NodeGraph: continue with the SafetyCheck
      next = boost::shared_ptr<State>(new SafetyCheckState);
    }
  else
    {
      next = boost::shared_ptr<State>(new CreateNodeGraph);
    }

  changeState(ctx, next);

  //                123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-
  DEBUG_STOP_TIMER("Calling plugin rewriters:               ");
}


void
CreateNodeGraph::createNodeGraph(ProgramCtx* ctx)
{
  DEBUG_START_TIMER;

  //
  // The GraphBuilder creates nodes and dependency edges from the raw program.
  //
  GraphBuilder gb;

  gb.run(*ctx->getIDB(), *ctx->getNodeGraph(), *ctx->getPluginContainer());

  boost::shared_ptr<State> next(new OptimizeState);
  changeState(ctx, next);

  //                123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-
  DEBUG_STOP_TIMER("Building node graph:                    ");
}


void
OptimizeState::optimize(ProgramCtx* ctx)
{
  DEBUG_START_TIMER;

  //
  // now call optimizers
  //
  for (std::vector<PluginInterface*>::iterator pi = ctx->getPlugins()->begin();
       pi != ctx->getPlugins()->end();
       ++pi)
    {
      PluginOptimizer* po = (*pi)->createOptimizer();

      if (po != 0)
	{
	  po->optimize(*ctx->getNodeGraph(), *ctx->getEDB());
	}
    }

  boost::shared_ptr<State> next(new CreateDependencyGraphState);
  changeState(ctx, next);

  //                123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-
  DEBUG_STOP_TIMER("Calling plugin optimizers:              ");
}



void
CreateDependencyGraphState::createDependencyGraph(ProgramCtx* ctx)
{
  DEBUG_START_TIMER;

  //
  // The ComponentFinder provides functions for finding SCCs and WCCs from a
  // set of nodes.
  //
  ComponentFinder* cf = new BoostComponentFinder;

  //
  // The DependencyGraph identifies and creates the graph components that will
  // be processed by the GraphProcessor.
  //
  // Initializing the DependencyGraph. Its constructor uses the
  // ComponentFinder to find relevant graph
  // properties for the subsequent processing stage.
  //
  DependencyGraph* dg = new DependencyGraph(cf, *ctx);
  ctx->setDependencyGraph(dg);

  boost::shared_ptr<State> next(new SafetyCheckState);
  changeState(ctx, next);

  //                123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-
  DEBUG_STOP_TIMER("Building dependency graph:              ");
}


void
SafetyCheckState::safetyCheck(ProgramCtx* ctx)
{
  DEBUG_START_TIMER;

  //
  // Performing the safety check
  //
  SafetyChecker schecker(*ctx->getIDB());
  schecker();

  boost::shared_ptr<State> next;

  if (ctx->getDependencyGraph() == 0)
    {
      // no dependency graph: continue with the evaluation of the IDB/EDB
      next = boost::shared_ptr<State>(new SetupProgramCtxState);
    }
  else
    {
      next = boost::shared_ptr<State>(new StrongSafetyCheckState);
    }
    
  changeState(ctx, next);
      
  //                123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-
  DEBUG_STOP_TIMER("Safety checking:                        ");
}



void
StrongSafetyCheckState::strongSafetyCheck(ProgramCtx* ctx)
{
  DEBUG_START_TIMER;

  StrongSafetyChecker sschecker(*ctx->getDependencyGraph());
  sschecker();

  boost::shared_ptr<State> next(new SetupProgramCtxState);
  changeState(ctx, next);

  //                123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-
  DEBUG_STOP_TIMER("Strong safety checking:                 ");
}


void
SetupProgramCtxState::setupProgramCtx(ProgramCtx* ctx)
{
  DEBUG_START_TIMER;

  //
  // now let the plugins setup the ProgramCtx
  //
  for (std::vector<PluginInterface*>::iterator pi = ctx->getPlugins()->begin();
       pi != ctx->getPlugins()->end();
       ++pi)
    {
      (*pi)->setupProgramCtx(*ctx);
    }


  boost::shared_ptr<State> next;

  if (ctx->getDependencyGraph() == 0)
    {
      // no dependency graph: continue with the evaluation of the IDB/EDB
      next = boost::shared_ptr<State>(new EvaluateProgramState);
    }
  else
    {
      next = boost::shared_ptr<State>(new EvaluateDepGraphState);
    }

  changeState(ctx, next);

  //                123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-
  DEBUG_STOP_TIMER("Setting up ProgramCtx:                  ");
}


void
EvaluateProgramState::evaluate(ProgramCtx* ctx)
{
  DEBUG_START_TIMER;

  //
  // We don't have a depedency graph, so just dump the program to an
  // OrdinaryModelGenerator and see what happens
  //

  std::vector<AtomSet> models;

  OrdinaryModelGenerator omg(*ctx);

  //
  // The GraphProcessor starts its computation with the program's ground
  // facts as input.
  // But only if the original EDB is consistent, otherwise, we can skip it
  // anyway.
  //
  if (ctx->getEDB()->isConsistent())
    {
      omg.compute(*ctx->getIDB(), *ctx->getEDB(), models);
    }
  
  ///@todo weak contraint prefixes are a bit clumsy here. How can we do better?

  //
  // prepare result container
  //
  // if we had any weak constraints, we have to tell the result container the
  // prefix in order to be able to compute each asnwer set's costs!
  //
  std::string wcprefix;
  
  if (ctx->getIDB()->getWeakConstraints().size() > 0)
    {
      wcprefix = "wch__";
    }
  
  ResultContainer* result = new ResultContainer(wcprefix);
  ctx->setResultContainer(result);
  
  //
  // put GraphProcessor result into ResultContainer
  ///@todo we can do better, for sure
  //
  for (std::vector<AtomSet>::const_iterator it = models.begin();
       it != models.end(); ++it)
    {
      ctx->getResultContainer()->addSet(*it);
    }
  
  boost::shared_ptr<State> next(new PostProcessState);
  changeState(ctx, next);

  //                123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-
  DEBUG_STOP_TIMER("Evaluating Program:                     ");
}


void
EvaluateDepGraphState::evaluate(ProgramCtx* ctx)
{
  DEBUG_START_TIMER;

  //
  // The GraphProcessor provides the actual strategy of how to compute the
  // hex-models of a given dependency graph.
  //
  GraphProcessor gp(*ctx);

  //
  // The GraphProcessor starts its computation with the program's ground
  // facts as input.
  // But only if the original EDB is consistent, otherwise, we can skip it
  // anyway.
  //
  if (ctx->getEDB()->isConsistent())
    {
      gp.run(*ctx->getEDB());
    }
  
  ///@todo weak contraint prefixes are a bit clumsy here. How can we do better?

  //
  // prepare result container
  //
  // if we had any weak constraints, we have to tell the result container the
  // prefix in order to be able to compute each asnwer set's costs!
  //
  std::string wcprefix;
  
  if (ctx->getIDB()->getWeakConstraints().size() > 0)
    {
      wcprefix = "wch__";
    }
  
  ResultContainer* result = new ResultContainer(wcprefix);
  ctx->setResultContainer(result);
  
  //
  // put GraphProcessor result into ResultContainer
  ///@todo we can do better, for sure
  //
  AtomSet* res;
  
  while ((res = gp.getNextModel()) != 0)
    {
      ctx->getResultContainer()->addSet(*res);
    }

  boost::shared_ptr<State> next(new PostProcessState);
  changeState(ctx, next);

  //                123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-
  DEBUG_STOP_TIMER("Evaluating dependency graph:            ");
}


void
PostProcessState::postProcess(ProgramCtx* ctx)
{
  DEBUG_START_TIMER;

  ///@todo filtering the atoms here is maybe to costly, how about
  ///ignoring the aux names when building the output, since the custom
  ///output builders of the plugins may need the aux names? Likewise
  ///for --filter predicates...

  //
  // remove auxiliary atoms
  //
  ctx->getResultContainer()->filterOut(Term::getAuxiliaryNames());

  ///@todo quick hack for dlt
//   if (optiondlt)
//     {
//       ctx->getResultContainer()->filterOutDLT();
//     }


  //
  // apply filter
  //
  //if (optionFilter.size() > 0)
  ctx->getResultContainer()->filterIn(Globals::Instance()->getFilters());

  boost::shared_ptr<State> next(new OutputState);
  changeState(ctx, next);

  //                123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-
  DEBUG_STOP_TIMER("Postprocessing GraphProcessor result:   ");
}


void
OutputState::output(ProgramCtx* ctx)
{
  DEBUG_START_TIMER;

  //
  // output format
  //
  OutputBuilder* outputbuilder = ctx->getOutputBuilder();

  if (outputbuilder == 0)
    {
      // first look if some plugin has an OutputBuilder
      for (std::vector<PluginInterface*>::const_iterator pi = ctx->getPlugins()->begin();
	   pi != ctx->getPlugins()->end(); ++pi)
	{
	  ///@todo this is very clumsy, what should we do if there
	  ///are more than one output builders available from the
	  ///atoms?
	  outputbuilder = (*pi)->createOutputBuilder();
	}

      // if no plugin provides an OutputBuilder, we use our own to output the models
      if (outputbuilder == 0)
	{
	  outputbuilder = new TextOutputBuilder;
	  ctx->setOutputBuilder(outputbuilder);
	}
    }

  ctx->getResultContainer()->print(std::cout, ctx->getOutputBuilder());

  ///@todo explicit endstate which does nothing?
  boost::shared_ptr<State> next(new State);
  changeState(ctx, next);
	    
  //                123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-
  DEBUG_STOP_TIMER("Building output:                        ");
}


DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
