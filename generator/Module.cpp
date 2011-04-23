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
    std::vector<int> numConstantVector; // for each modules
    std::vector<int> numPredicateVector; // for each modules
    std::vector<int> numRuleVector; // for each modules
    std::vector<int> numInputPredsVector; // number of input predicates for each modules

    void createModuleHeader(int idxModule, std::ostream& oss);
    void generateFacts(int idxModule, std::ostream& oss);
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
  public:
    void setAll(int NumConstant, int NumPredicate, int SizeOfHead, int SizeOfBody, int NotProbability, int NumRules, int NumModules, std::string& OutputFilePrefix);
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
  int maxInputPreds = numPredicateMax/3;
  if ( maxInputPreds <= 1 && numPredicateMax > 1)
    {
      maxInputPreds = 2;
    }
  for (int i=0;i<numModules;i++)
    {
      numConstantVector.push_back( (rand()%numConstantMax) + 1);
      numPredicateVector.push_back( (rand()%numPredicateMax) + 1);
      numRuleVector.push_back( (rand()%numPredicateMax) + 1);
      // for randomizing input preds for each module
      if ( i==0 )
	{ // for main module, input preds = 0
	  numInputPredsVector.push_back(0);  
	}
      else
	{ // for library module, input preds min = 1
	  numInputPredsVector.push_back( (rand()% maxInputPreds ) + 1);
	  if (numInputPredsVector.back() > numPredicateVector.back() ) 
	    numInputPredsVector.back() = numPredicateVector.back();
	}
    }
}


// create module header: #module(..., [...]).
void BaseTopology::createModuleHeader(int idxModule, std::ostream& oss)
{
  oss << "#module(mod" << idxModule << ", [";
  for (int i=0;i<numInputPredsVector.at(idxModule);i++)
    {
      if (i>0) oss << ", ";
      oss << "p" << idxModule << "p" << i << "/1";
    }
  oss << "]).";  
}


// generate facts, automatically prefix the predicate symbol to p<idxModule>p<idPred>... 
// input params: idxModule, numConstant, numPredicate, numFacts to be generated
void BaseTopology::generateFacts(int idxModule, std::ostream& oss)
{
  int constant;
  int predicate;
  int numFacts = numConstantVector.at(idxModule)*numPredicateVector.at(idxModule)*2/3;
  for (int i=0;i<numFacts;i++)
    {
      predicate = rand() % numPredicateVector.at(idxModule);
      constant = rand() % numConstantVector.at(idxModule);
      oss << "p" << idxModule << "p" << predicate << "(c" << constant << "). ";           
    }
}


// generate rules
// input params: idxModule, numConstant, numPredicate, sizeOfHead, sizeOfBody, notProbability, numRules to be generated
void BaseTopology::generateRules(int idxModule, std::ostream& oss)
{
  int constant;
  int predicate;
  int notProbs;
  char vars;
  // generate rules
  int numRule = rand() % numRuleMax;
  for (int i=0;i<numRule;i++)
    {
      // generate head
      int sizeOfHead = (rand() % sizeOfHeadMax) + 1;
      for (int j=0;j<sizeOfHead;j++)
	{
	  predicate = rand() % numPredicateVector.at(idxModule);
	  constant = rand() % numConstantVector.at(idxModule);
	  if ( j>0 ) oss << " v ";
          vars = j+65;  // generate variable
          oss << "p" << idxModule << "p" << predicate << "(" << vars << ")";           
	} 
      oss << " :- ";
      // generate body
      for (int j=0;j<sizeOfHead;j++)
	{
	  predicate = rand() % numPredicateVector.at(idxModule);
	  vars = (j) + 65; // generate variables
	  notProbs = rand() % 100;
	  if ( j>0 ) oss << ", ";
          oss << "p" << idxModule <<"p" << predicate << "(" << vars << ")";           
	} 
      int sizeOfBody = (rand() % sizeOfBodyMax) + 1;
      for (int j=sizeOfHead;j<sizeOfBody;j++)
	{
	  predicate = rand() % numPredicateVector.at(idxModule);
	  vars = (rand() % sizeOfHead) + 65; // generate variables
	  notProbs = rand() % 100;
	  if ( j>0 ) oss << ", ";
	  if ( notProbs < notProbability ) oss << "not ";
          oss << "p" << idxModule << "p" << predicate << "(" << vars << ")";           
	} 
      oss << "." << std::endl;
    }
}


// generate module call from idxModuleSrc to idxModuleDest with numPredicate to create a random input predicate p<idxModuleSrc>p<random 0-numPredicate>
void BaseTopology::generateModuleCall(int idxModuleSrc, int idxModuleDest, std::ostream& oss)
{
  oss << "out" << idxModuleSrc << " :- @mod" << idxModuleDest << "[";
  for (int i=0;i<numInputPredsVector.at(idxModuleDest);i++) 
    {
      if (i > 0) oss <<",";
      oss << "p" << idxModuleSrc << "p" << (rand()%numPredicateVector.at(idxModuleSrc));
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
// set all params
void DiamondTopology::setAll(int NumConstant, int NumPredicate, int SizeOfHead, int SizeOfBody, int NotProbability, int NumRules, int NumModules, std::string& OutputFilePrefix)
{
  BaseTopology::setAll(NumConstant, NumPredicate, SizeOfHead, SizeOfBody, NotProbability, NumRules, NumModules*3+1, OutputFilePrefix);
}

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
  else if ( (idxModule+2)%3 == 0 ) generateModuleCall(idxModule, idxModule+2, oss);
  else if ( idxModule%3 == 0 )
    {
      generateModuleCall(idxModule, idxModule+1, oss);
      oss << std::endl;
      generateModuleCall(idxModule, idxModule+2, oss);
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
      std::cerr << "Module diamond <numConstant> <numPredicate> <sizeOfHead> <sizeOfBody> <notProbability> <numRules> <numDiamond> <outputFilePrefix>" << std::endl;
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


