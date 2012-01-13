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
 * @file   Module.cpp
 * @author Tri Kurniawan Wijaya
 * @date   Tue 05 Apr 2011 06:37:56 PM CEST 
 * 
 * @brief  Generate a random program (with some parameter settings) for benchmarking
 *         For star, line, ring, diamond, and random topology
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <iostream>
#include <stdlib.h>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <sys/time.h>


class BaseTopology
{
  protected:
    std::string outputFilePrefix;
    int numConstantMax; 
    int numPredicateMax; 
    int sizeOfHeadMax; 
    int sizeOfBodyMax;
    int notProbability;
    int numRuleMax;
    int numModules;
    int maxPredArity;
    std::vector<int> numInputPredVector; // number of input predicates for each modules
    std::vector< std::vector<int> > listInputPreds;
    std::vector< std::vector<int> > numPredArity; // number of input predicates for each modules

    int* createAtom(int idxModule, int idxPredicate, std::string content, int maxToRand, std::ostream& oss);
    void createGroundAtom(int idxModule, int idxPredicate, std::ostream& oss);
    int* createNonGroundAtom(int idxModule, int idxPredicate, std::ostream& oss);

    void createModuleHeader(int idxModule, std::ostream& oss);
    void generateFacts(int idxModule, std::ostream& oss);
    void generateRule(int idxModule, std::ostream& ossResult);
    void generateRules(int idxModule, std::ostream& oss);
    void generateModuleCall(int idxModuleSrc, int idxModuleDest, std::ostream& oss);
    virtual void createMainModule(std::ostream& oss){};
    virtual void createLibraryModule(int idxModule, std::ostream& oss){};

  public:
    void setAll(int NumConstant, int NumPredicate, int SizeOfHead, int SizeOfBody, int NotProbability, int NumRule, int NumModules, std::string& OutputFilePrefix);
    void generate();
};


class StarTopology: public BaseTopology 
{
  void createMainModule(std::ostream& oss);
  void createLibraryModule(int idxModule, std::ostream& oss);
};


class LineTopology: public BaseTopology 
{
  void createMainModule(std::ostream& oss);
  void createLibraryModule(int idxModule, std::ostream& oss);
};


class RingTopology: public BaseTopology 
{
  void createMainModule(std::ostream& oss);
  void createLibraryModule(int idxModule, std::ostream& oss);
};

class DiamondTopology: public BaseTopology 
{
  private:
    void createMainModule(std::ostream& oss);
    void createLibraryModule(int idxModule, std::ostream& oss);
};


class RandomTopology: public BaseTopology 
{
  private:
    int density;
    void createMainModule(std::ostream& oss);
    void createLibraryModule(int idxModule, std::ostream& oss);
  public:
    void setAll(int NumConstant, int NumPredicate, int SizeOfHead, int SizeOfBody, int NotProbability, int NumRules, int NumModules, std::string& OutputFilePrefix, int Density=50);
};


class TreeTopology: public BaseTopology 
{
  private:
    int branch;
    void createMainModule(std::ostream& oss);
    void createLibraryModule(int idxModule, std::ostream& oss);
  public:
    void setAll(int NumConstant, int NumPredicate, int SizeOfHead, int SizeOfBody, int NotProbability, int NumRules, int NumModules, std::string& OutputFilePrefix, int Branch=3);
};

// set all params
void BaseTopology::setAll(int NumConstant, int NumPredicate, int SizeOfHead, int SizeOfBody, int NotProbability, int NumRule, int NumModules, std::string& OutputFilePrefix)
{
  outputFilePrefix = OutputFilePrefix;
  numConstantMax = NumConstant;
  numPredicateMax = NumPredicate;
  sizeOfHeadMax = SizeOfHead;
  sizeOfBodyMax = SizeOfBody;
  notProbability = NotProbability;
  numRuleMax = NumRule;
  numModules = NumModules;
  maxPredArity = 4;
  if (maxPredArity > numPredicateMax) maxPredArity = numPredicateMax;
  // list of arity per predicate
  int arrArity[22];
  for (int i= 0;i< 2;i++) arrArity[i]=0;  // 2
  for (int i= 2;i<12;i++) arrArity[i]=1;  // 10
  for (int i=12;i<17;i++) arrArity[i]=2;  // 5
  for (int i=17;i<20;i++) arrArity[i]=3;  // 3
  for (int i=20;i<22;i++) arrArity[i]=4;  // 2

  for (int i=0; i<numModules; i++)
    {
      std::vector<int> listArity;	
      for (int j=0; j<numPredicateMax; j++)
	{
          listArity.push_back( arrArity[rand()% 22] ); // random between 0-maxPredArity
//          listArity.push_back( rand()% (maxPredArity + 1) ); // random between 0-maxPredArity
	}
      numPredArity.push_back(listArity);
    }

  // maxInputPreds for modules
  int maxInputPreds = numPredicateMax/3;
  if ( maxInputPreds == 1 && numPredicateMax > 1)
    {
      maxInputPreds = 2;
    }
  if (maxInputPreds == 0) maxInputPreds = 1;

  // defining input preds per module
  for (int i=0;i<numModules;i++)
    {
      // for randomizing input preds for each module
      if ( i==0 )
	{ // for main module, input preds = 0
	  numInputPredVector.push_back(0);  
	  std::vector<int> listInputPred;
	  listInputPreds.push_back(listInputPred);
	}
      else
	{ // for library module, input preds min = 1
	  numInputPredVector.push_back( (rand()% maxInputPreds ) + 1);
	  std::vector<int> listInputPred;
	  for (int j=0; j<numInputPredVector.back(); j++) 
	    { // loop on how many input preds we have on this module
	      listInputPred.push_back( rand() % numPredicateMax ); 
	    }
	  listInputPreds.push_back( listInputPred );
	}
    }
}



int* BaseTopology::createAtom(int idxModule, int idxPredicate, std::string content, int maxToRand, std::ostream& oss)
{ 
  int* conf = new int[maxToRand];
  for (int i=0; i<maxPredArity; i++) conf[i] = 0;
  oss << "p" << idxPredicate;
  for (int i=0;i<numPredArity.at(idxModule).at(idxPredicate);i++)
    {
      if ( i == 0 ) oss << "("; else oss << ",";
      int value = rand() % maxToRand;
      conf[value] = 1;
      oss << content << value;
      if ( i == numPredArity.at(idxModule).at(idxPredicate)-1 ) oss << ")";
    }
  return conf;
}


// create p(c0, c2, c2, c3) -> constants based on listConstant
void BaseTopology::createGroundAtom(int idxModule, int idxPredicate, std::ostream& oss)
{
  int* conf = createAtom(idxModule, idxPredicate, "c", numConstantMax, oss);
  delete[] conf;
}


// create p(X0, X2, X2, X3) -> variables based on listVars
int* BaseTopology::createNonGroundAtom(int idxModule, int idxPredicate, std::ostream& oss)
{
  int* arr = 0;
  arr = createAtom(idxModule, idxPredicate, "X", maxPredArity, oss);
  return arr;
}

// create module header: #module(..., [...]).
void BaseTopology::createModuleHeader(int idxModule, std::ostream& oss)
{
  oss << "#module(mod" << idxModule << ", [";
  for (int i=0;i<numInputPredVector.at(idxModule);i++)
    {
      if ( i>0 ) oss << ", ";
      oss << "p" << listInputPreds.at(idxModule).at(i) << "/" << numPredArity.at(idxModule).at( listInputPreds.at(idxModule).at(i) );
    }
  oss << "]).";  
}


// generate facts, automatically prefix the predicate symbol to p<idxModule>p<idPred>... 
// input params: idxModule, numConstant, numPredicate, numFacts to be generated
void BaseTopology::generateFacts(int idxModule, std::ostream& oss)
{
  int idxPredicate;
  int numFacts = rand() % (numConstantMax*numPredicateMax/3);
  if (numFacts < (numConstantMax+numPredicateMax)) numFacts = numConstantMax+numPredicateMax;
  for (int i=0;i<numFacts;i++)
    {
      idxPredicate = rand() % numPredicateMax;
      createGroundAtom(idxModule, idxPredicate, oss);
      oss << ". " << std::endl;           
    }
}


void BaseTopology::generateRule(int idxModule, std::ostream& ossResult)
{
  bool safe=false;
  std::ostringstream oss;
  while (safe==false) 
    {
      oss.str("");
      // generate head
      int confHead[maxPredArity];  // list the name of vars X(0-XmaxPredArity-1)
      for (int i=0; i<maxPredArity; i++) confHead[i] = 0;
      int sizeOfHead = (rand() % sizeOfHeadMax) + 1;
      for (int j=0;j<sizeOfHead;j++)
	{
	  int idxPredicate = rand() % numPredicateMax;
	  if ( j>0 ) oss << " v ";
	  int* conf = 0;
	  conf = createNonGroundAtom(idxModule, idxPredicate, oss);
		//rmv. std::cout<<"head: " << oss.str();
	  for (int k=0;k<maxPredArity;k++) 
	    {
		//rmv. std::cout << conf[k];	
		if (conf[k]==1) confHead[k]=1;
	    }
	  //rmv. std::cout<< std::endl;
	  delete [] conf;
	} 
	
      oss << " :- ";
      // generate body
      int confBody[maxPredArity];  // list the name of vars X(0-XmaxPredArity-1)
      for (int i=0; i<maxPredArity; i++) confBody[i] = 0;
      int sizeOfBody = (rand() % sizeOfBodyMax) + 1;
      for (int j=0;j<sizeOfBody;j++)
	{
	  int idxPredicate = rand() % numPredicateMax;
	  int notProbs = rand() % 100;
	  if ( j>0 ) oss << ", ";
	  if ( notProbs < notProbability ) oss << "not ";
	  int* conf = createNonGroundAtom(idxModule, idxPredicate, oss);
	  if ( notProbs < notProbability ) 
	   { for (int k=0;k<maxPredArity;k++) if (conf[k]==1) confHead[k]=1; }
	  else 
	   { for (int k=0;k<maxPredArity;k++) if (conf[k]==1) confBody[k]=1; }
	  delete [] conf;
	} 

      oss << "." << std::endl;
/*
      oss << "% [";
      for (int k=0;k<maxPredArity;k++) oss << confHead[k];
      oss << "] [";
      for (int k=0;k<maxPredArity;k++) oss << confBody[k];
      oss << "]" << std::endl;
*/
      // check
      safe = true;
      int k=0;
      while (k<maxPredArity && safe==true)
	{
	  if (confHead[k]==1 && confBody[k]==0) safe=false;
	  k++;
	}
    }
    ossResult << oss.str();  
}


// generate rules
// input params: idxModule, numConstant, numPredicate, sizeOfHead, sizeOfBody, notProbability, numRules to be generated
////////////////// ensure rule safety here
void BaseTopology::generateRules(int idxModule, std::ostream& oss)
{
  // generate rules
  int numRule = rand() % numRuleMax;
  for (int i=0;i<numRule;i++)
    {
      generateRule(idxModule, oss);
    }
}


// generate module call from idxModuleSrc to idxModuleDest with numPredicate to create a random input predicate p<idxModuleSrc>p<random 0-numPredicate>
void BaseTopology::generateModuleCall(int idxModuleSrc, int idxModuleDest, std::ostream& oss)
{
  oss << "out" << idxModuleSrc << " :- @mod" << idxModuleDest << "[";
  int ctrNew=0;
  for (int i=0;i<numInputPredVector.at(idxModuleDest);i++) 
    {
      if (i > 0) oss <<",";
      int value = 0;
      // test for arity matching
      int ctrLoop=0;
      bool match = false;      
      while (ctrLoop<1 && match==false)
	{
	  value = rand() % numPredicateMax;
          if (numPredArity.at(idxModuleSrc).at(value)== numPredArity.at(idxModuleDest).at( listInputPreds.at(idxModuleDest).at(i) )) match = true;
	  ctrLoop++;
	}
      if (match==false)
	{
	  oss << "pnew" << ctrNew;
	  ctrNew++;
	}
      else oss << "p" << ( value );
    }
  oss << "]::out" << idxModuleDest << ".";
}


void BaseTopology::generate()
{
      // prepare the output file 
      std::ofstream fileEach;
      std::ostringstream oss;
      oss.str("");
      oss << outputFilePrefix << 0 << ".mlp";		
      fileEach.open( oss.str().c_str() );
      // create a main module
      oss.str("");
      createMainModule(oss);
      oss << std::endl;
      // write to output files
      fileEach << oss.str();
      fileEach.close();

      // create library modules	
      for (int i=1; i<numModules; i++)
	{
	  // prepare individual output files
          oss.str("");
          oss << outputFilePrefix << i << ".mlp";		
	  fileEach.open( oss.str().c_str() );
          oss.str("");
	  createLibraryModule(i, oss);
	  oss << std::endl;
          // write to output files
          fileEach << oss.str();
          fileEach.close();
	}
}



/******************* 
 * For star topology
 *******************/
void StarTopology::createMainModule(std::ostream& oss)
{
  // generate fact as many as the rules
  createModuleHeader(0, oss);
  oss << std::endl;
  // generate facts
  generateFacts(0, oss);
  oss << std::endl;

  // generate rules
  generateRules(0, oss);

  // generate module calls 
  for (int i=1;i<numModules;i++)
    {
      generateModuleCall(0, i, oss);
      oss << std::endl;
    }
} 


void StarTopology::createLibraryModule(int idxModule, std::ostream& oss)
{
  // generate fact as many as the rules
  createModuleHeader(idxModule, oss);
  oss << std::endl;

  // generate facts
  generateFacts(idxModule, oss);
  oss << std::endl;

  // generate rules
  generateRules(idxModule, oss);

  // generate module calls 
  // the number of input preds is according to numInputPredsVector
  generateModuleCall(idxModule, idxModule, oss);
}



/******************* 
 * For Line topology
 *******************/

void LineTopology::createMainModule(std::ostream& oss)
{
  // generate fact as many as the rules
  createModuleHeader(0, oss);
  oss << std::endl;
  // generate facts
  generateFacts(0, oss);
  oss << std::endl;

  // generate rules
  generateRules(0, oss);

  if ( numModules > 1 ) generateModuleCall(0, 1, oss);
} 


void LineTopology::createLibraryModule(int idxModule, std::ostream& oss)
{
  // generate fact as many as the rules
  createModuleHeader(idxModule, oss);
  oss << std::endl;

  // generate facts
  generateFacts(idxModule, oss);
  oss << std::endl;

  // generate rules
  generateRules(idxModule, oss);

  // generate module calls 
  if ( idxModule == numModules-1 )
    generateModuleCall(idxModule, idxModule, oss);
  else 
    generateModuleCall(idxModule, idxModule+1, oss);
}



/******************* 
 * For Ring topology
 *******************/

void RingTopology::createMainModule(std::ostream& oss)
{
  // generate fact as many as the rules
  createModuleHeader(0, oss);
  oss << std::endl;
  // generate facts
  generateFacts(0, oss);
  oss << std::endl;

  // generate rules
  generateRules(0, oss);

  if ( numModules > 1 ) generateModuleCall(0, 1, oss);
} 


void RingTopology::createLibraryModule(int idxModule, std::ostream& oss)
{
  // generate fact as many as the rules
  createModuleHeader(idxModule, oss);
  oss << std::endl;

  // generate facts
  generateFacts(idxModule, oss);
  oss << std::endl;

  // generate rules
  generateRules(idxModule, oss);

  // generate module calls 
  if ( idxModule == numModules-1 )
    generateModuleCall(idxModule, 0, oss);
  else 
    generateModuleCall(idxModule, idxModule+1, oss);
}



/********************** 
 * For Diamond topology
 **********************/
void DiamondTopology::createMainModule(std::ostream& oss)
{
  // generate fact as many as the rules
  createModuleHeader(0, oss);
  oss << std::endl;
  // generate facts
  generateFacts(0, oss);
  oss << std::endl;

  // generate rules
  generateRules(0, oss);

  if ( numModules > 1 ) 
    {
      generateModuleCall(0, 1, oss);
      oss << std::endl;
    }
  if ( numModules > 2 ) 
    {
      generateModuleCall(0, 2, oss);
      oss << std::endl;
    }
} 


void DiamondTopology::createLibraryModule(int idxModule, std::ostream& oss)
{
  // generate fact as many as the rules
  createModuleHeader(idxModule, oss);
  oss << std::endl;

  // generate facts
  generateFacts(idxModule, oss);
  oss << std::endl;

  // generate rules
  generateRules(idxModule, oss);

  // generate module calls 
  if ( idxModule == numModules-1) generateModuleCall(idxModule, idxModule, oss);
  else if ( (idxModule+1)%3 == 0 ) generateModuleCall(idxModule, idxModule+1, oss);
  else 
    if ( (idxModule+2)%3 == 0 ) 
      {
        if ( idxModule+2 < (numModules) ) generateModuleCall(idxModule, idxModule+2, oss);
        else generateModuleCall(idxModule, idxModule, oss);
      }
  else 
    if ( idxModule%3 == 0 )
      {
        generateModuleCall(idxModule, idxModule+1, oss);
        oss << std::endl;
        if (idxModule+2 < numModules) generateModuleCall(idxModule, idxModule+2, oss);
        oss << std::endl;
      }
}


/********************* 
 * For Random topology
 *********************/
// set all params
void RandomTopology::setAll(int NumConstant, int NumPredicate, int SizeOfHead, int SizeOfBody, int NotProbability, int NumRules, int NumModules, std::string& OutputFilePrefix, int Density)
{
  BaseTopology::setAll(NumConstant, NumPredicate, SizeOfHead, SizeOfBody, NotProbability, NumRules, NumModules, OutputFilePrefix);
  density = Density;
  //rmv. std::cerr << "density: " << density << std::endl;
}

void RandomTopology::createMainModule(std::ostream& oss)
{
  // generate fact as many as the rules
  createModuleHeader(0, oss);
  oss << std::endl;
  // generate facts
  generateFacts(0, oss);
  oss << std::endl;

  // generate rules
  generateRules(0, oss);

  // generate module calls
  bool moduleCall = false;
  for (int i=1;i<numModules;i++)
    {
      if ( rand() % 100 < density ) 
        {
          generateModuleCall(0, i, oss);
          oss << std::endl;
	  moduleCall = true;
        }
    }
  if ( moduleCall == false )
    {
      oss << "out0.";
    }
} 


void RandomTopology::createLibraryModule(int idxModule, std::ostream& oss)
{
  // generate fact as many as the rules
  createModuleHeader(idxModule, oss);
  oss << std::endl;

  // generate facts
  generateFacts(idxModule, oss);
  oss << std::endl;

  // generate rules
  generateRules(idxModule, oss);

  // generate module calls
  bool moduleCall = false;
  if ( rand() % 100 < density ) 
    {
      generateModuleCall(idxModule, 0, oss);
      oss << std::endl;
      moduleCall = true;
    }
  for (int i=1;i<numModules;i++)
    {
      if ( rand() % 100 < density ) 
        {
          generateModuleCall(idxModule, i, oss);
          oss << std::endl;
          moduleCall = true;
        }
    }
  if ( moduleCall == false )
    {
      oss << "out" << idxModule << ".";
    }
}


/********************* 
 * For Tree topology
 *********************/
// set all params
void TreeTopology::setAll(int NumConstant, int NumPredicate, int SizeOfHead, int SizeOfBody, int NotProbability, int NumRules, int NumModules, std::string& OutputFilePrefix, int Branch)
{
  BaseTopology::setAll(NumConstant, NumPredicate, SizeOfHead, SizeOfBody, NotProbability, NumRules, NumModules, OutputFilePrefix);
  branch = Branch;
}

void TreeTopology::createMainModule(std::ostream& oss)
{
  // generate fact as many as the rules
  createModuleHeader(0, oss);
  oss << std::endl;
  // generate facts
  generateFacts(0, oss);
  oss << std::endl;

  // generate rules
  generateRules(0, oss);

  // generate module calls
  int numCall = branch;
  if (numModules-1 < branch) numCall = numModules-1;
  for (int i=1;i<=numCall;i++)
    {
          generateModuleCall(0, i, oss);
          oss << std::endl;
    }
} 


void TreeTopology::createLibraryModule(int idxModule, std::ostream& oss)
{
  // generate fact as many as the rules
  createModuleHeader(idxModule, oss);
  oss << std::endl;

  // generate facts
  generateFacts(idxModule, oss);
  oss << std::endl;

  // generate rules
  generateRules(idxModule, oss);

  // generate module calls
  int lowerbound = (idxModule*branch) + 1;
  int upperbound = (idxModule+1) * branch;
  bool moduleCall = false;
  for (int i=lowerbound;i<=upperbound;i++)
    {
      if (i < numModules) 
	{
          generateModuleCall(idxModule, i, oss);
          oss << std::endl;
	  moduleCall = true;
	}
    }
  if ( moduleCall == false ) generateModuleCall(idxModule, idxModule, oss);
}



int main(int argc, char *argv[])
{

  // set the random seed	
  struct timeval tv;
  gettimeofday(&tv, NULL);
  srand(tv.tv_sec + tv.tv_usec);

  // read params
  int numParam = 9;
  if (argc <= numParam) 
    {
      std::cerr << "Usage: " << std::endl;	
      std::cerr << "Module star <numConstant> <numPredicate> <sizeOfHead> <sizeOfBody> <notProbability> <numRules> <numModules> <outputFilePrefix>" << std::endl;
      std::cerr << "Module line <numConstant> <numPredicate> <sizeOfHead> <sizeOfBody> <notProbability> <numRules> <numModules> <outputFilePrefix>" << std::endl;
      std::cerr << "Module ring <numConstant> <numPredicate> <sizeOfHead> <sizeOfBody> <notProbability> <numRules> <numModules> <outputFilePrefix>" << std::endl;
      std::cerr << "Module diamond <numConstant> <numPredicate> <sizeOfHead> <sizeOfBody> <notProbability> <numRules> <numModules> <outputFilePrefix>" << std::endl;
      std::cerr << "Module random <numConstant> <numPredicate> <sizeOfHead> <sizeOfBody> <notProbability> <numRules> <numModules> <outputFilePrefix> [density]" << std::endl;
      std::cerr << "Module tree <numConstant> <numPredicate> <sizeOfHead> <sizeOfBody> <notProbability> <numRules> <numModules> <outputFilePrefix> [branch]" << std::endl;

      return 0;
    }
  else 
    {
      int param[numParam-2]; // -2 for topology and outputFilePrefix
      for (int i=0;i<numParam-2;i++)
        {
          param[i] = atoi(argv[i+2]);  //+2 because [0] is for fileExecution, [1] is for topology
        }
      std::string topology = argv[1];
      std::string outputFilePrefix = argv[numParam];

      if (topology == "star")
	{
	  StarTopology example;
	  example.setAll(param[0], param[1], param[2], param[3], param[4], param[5], param[6], outputFilePrefix);
	  example.generate();
	}
      else if (topology == "line")
	{
	  LineTopology example;
	  example.setAll(param[0], param[1], param[2], param[3], param[4], param[5], param[6], outputFilePrefix);
	  example.generate();
	}
      else if (topology == "ring")
	{
	  RingTopology example;
	  example.setAll(param[0], param[1], param[2], param[3], param[4], param[5], param[6], outputFilePrefix);
	  example.generate();
	}
      else if (topology == "diamond")
	{
	  DiamondTopology example;
	  example.setAll(param[0], param[1], param[2], param[3], param[4], param[5], param[6], outputFilePrefix);
	  example.generate();
	}
      else if (topology == "random")
	{
	  RandomTopology example;
	  if (argc == numParam + 2)
	    {
	      example.setAll(param[0], param[1], param[2], param[3], param[4], param[5], param[6], outputFilePrefix, atoi(argv[numParam+1]));
	    }
          else 
	    {
	      example.setAll(param[0], param[1], param[2], param[3], param[4], param[5], param[6], outputFilePrefix);
	    }
	  example.generate();
	}
      else if (topology == "tree")
	{
	  TreeTopology example;
	  if (argc == numParam + 2)
	    {
	      example.setAll(param[0], param[1], param[2], param[3], param[4], param[5], param[6], outputFilePrefix, atoi(argv[numParam+1]));
	    }
          else 
	    {
	      example.setAll(param[0], param[1], param[2], param[3], param[4], param[5], param[6], outputFilePrefix);
	    }
	  example.generate();
	}
    }
}


