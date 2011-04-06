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
 * 02110-1301 USA
 */


/**
 * @file   ModuleStar.cpp
 * @author Tri Kurniawan Wijaya
 * @date   Tue 05 Apr 2011 06:37:56 PM CEST 
 * 
 * @brief  Generate a random program (with some parameter settings) for benchmarking
 *         For star topology
 */

#include <iostream>
#include <stdlib.h>
#include <sstream>
#include <fstream>
#include <sys/time.h>

void createModuleHeader(int idxModule, int numParam, std::ostream& oss)
{
  oss << "#module(mod" << idxModule << ", [";
  for (int i=0;i<numParam;i++)
    {
      if (i>0) oss << ", ";
      oss << "p" << idxModule << "p" << i << "/1";
    }
  oss << "]).";  
}


void generateFacts(int idxModule, int numConstant, int numPred, int numFacts, std::ostream& oss)
{
  int constant;
  int predicate;
  for (int i=0;i<numFacts;i++)
    {
      predicate = rand() % numPred;
      constant = rand() % numConstant;
      oss << "p" << idxModule << "p" << predicate << "(c" << constant << "). ";           
    }
}


void generateRules(int idxModule, int numConstant, int numPred, int sizeOfHead, int sizeOfBody, int notProbability, int numRules, std::ostream& oss)
{
  int constant;
  int predicate;
  int notProbs;
  char vars;
  // generate rules
  for (int i=0;i<numRules;i++)
    {
      // generate head
      for (int j=0;j<sizeOfHead;j++)
	{
	  predicate = rand() % numPred;
	  constant = rand() % numConstant;
	  if ( j>0 ) oss << " v ";
          vars = j+65;  // generate variable
          oss << "p" << idxModule << "p" << predicate << "(" << vars << ")";           
	} 
      oss << " :- ";
      // generate body
      for (int j=0;j<sizeOfHead;j++)
	{
	  predicate = rand() % numPred;
	  vars = (j) + 65; // generate variables
	  notProbs = rand() % 100;
	  if ( j>0 ) oss << ", ";
          oss << "p" << idxModule <<"p" << predicate << "(" << vars << ")";           
	} 
      for (int j=sizeOfHead;j<sizeOfBody;j++)
	{
	  predicate = rand() % numPred;
	  vars = (rand() % sizeOfHead) + 65; // generate variables
	  notProbs = rand() % 100;
	  if ( j>0 ) oss << ", ";
	  if ( notProbs < notProbability ) oss << "not ";
          oss << "p" << idxModule << "p" << predicate << "(" << vars << ")";           
	} 
      oss << "." << std::endl;
    }
}


void generateModuleCall(int idxModuleSrc, int idxModuleDest, int numPred, std::ostream& oss)
{
  oss << "out" << idxModuleSrc << " :- @mod" << idxModuleDest << "[p" << idxModuleSrc << "p" << rand()%numPred << "]::out" << idxModuleDest << ".";
}


void createMainModule(int numConstant, int numPred, int sizeOfHead, int sizeOfBody, int notProbability, int numRules, int numModules, std::ostream& oss)
{
  // generate fact as many as the rules
  createModuleHeader(0,0, oss);
  oss << std::endl;

  // generate facts
  generateFacts(0, numConstant, numPred, (numConstant*numPred*2/3), oss);
  oss << std::endl;

  // generate rules
  generateRules(0, numConstant, numPred, sizeOfHead, sizeOfBody, notProbability, numRules, oss);

  // generate module calls 
  for (int i=1;i<=numModules;i++)
    {
      generateModuleCall(0, i, numPred, oss);
      oss << std::endl;
    }
} 

void createLibraryModule(int idxModule, int numConstant, int numPred, int sizeOfHead, int sizeOfBody, int notProbability, int numRules, std::ostream& oss)
{
  // generate fact as many as the rules
  createModuleHeader(idxModule, 1, oss);
  oss << std::endl;

  // generate facts
  generateFacts(idxModule, numConstant, numPred, (numConstant*numPred*1/3), oss);
  oss << std::endl;

  // generate rules
  generateRules(idxModule, numConstant, numPred, sizeOfHead, sizeOfBody, notProbability, numRules, oss);

  // generate module calls 
  generateModuleCall(idxModule, idxModule, numPred, oss);
}

// the parameter should be: numOfModules sizeOfBody numOfRules numConstant numPreds
int main(int argc, char *argv[])
{
  // set the random seed	
  struct timeval tv;
  gettimeofday(&tv, NULL);
  srand(tv.tv_sec + tv.tv_usec);

  // read params
  int numParam = 8;
  if (argc <= numParam) 
    {
      std::cerr << "Required 8 paramaters: numConstant numPredicate sizeOfHead sizeOfBody notProbability numRules numModules outputFilePrefix" << std::endl;
      return 0;
    }
  else 
    {
      // vars declaration
      std::ofstream fileAll;
      std::ofstream fileEach;
      std::ostringstream oss;

      // process params
      int param[numParam-1];
      for (int i=0;i<numParam-1;i++)
        {
          param[i] = atoi(argv[i+1]);
        }
      std::string filePrefix = argv[numParam];

      // prepare the output file 
      fileAll.open((filePrefix+"All.mlp").c_str());	
      oss.str("");
      oss << filePrefix << 0 << ".mlp";		
      fileEach.open( oss.str().c_str() );

      // create a main module
      oss.str("");
      createMainModule(param[0], param[1], param[2], param[3], param[4], param[5], param[6], oss);
      oss << std::endl;

      // write to output files
      fileAll << oss.str();
      fileEach << oss.str();
      fileEach.close();

      // create library modules	
      for (int i=1; i<=param[6]; i++)
	{
	  // prepare individual output files
          oss.str("");
          oss << filePrefix << i << ".mlp";		
	  fileEach.open( oss.str().c_str() );
          oss.str("");
	  createLibraryModule(i, param[0], param[1], param[2], param[3], param[4], param[5], oss);
	  oss << std::endl;
          // write to output files
          fileAll << oss.str();
          fileEach << oss.str();
          fileEach.close();
	}
      fileAll.close();      
    }
}




