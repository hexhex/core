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
 *         For star, line, ring, and fully connected topology
 */

#include <iostream>
#include <stdlib.h>
#include <sstream>
#include <fstream>
#include <string>
#include <sys/time.h>


class BaseExample
{
  protected:
    std::string outputFilePrefix;
    int numConstant; 
    int numPredicate; 
    int sizeOfHead; 
    int sizeOfBody;
    int notProbability;
    int numFacts;
    int numRules;
    int numModules;
    void createModuleHeader(int idxModule, int numParam, std::ostream& oss);
    void generateFacts(int idxModule, std::ostream& oss);
    void generateRules(int idxModule, std::ostream& oss);
    void generateModuleCall(int idxModuleSrc, int idxModuleDest, int numInputPreds, std::ostream& oss);
    virtual void createMainModule(std::ostream& oss){};
    virtual void createLibraryModule(int idxModule, std::ostream& oss){};

  public:
    void setAll(int NumConstant, int NumPredicate, int SizeOfHead, int SizeOfBody, int NotProbability, int NumRules, int NumModules, std::string& OutputFilePrefix);
    void generate();
};


class StarExample: public BaseExample 
{
  void createMainModule(std::ostream& oss);
  void createLibraryModule(int idxModule, std::ostream& oss);
};


class LineExample: public BaseExample 
{
  void createMainModule(std::ostream& oss);
  void createLibraryModule(int idxModule, std::ostream& oss);
};


class RingExample: public BaseExample 
{
  void createMainModule(std::ostream& oss);
  void createLibraryModule(int idxModule, std::ostream& oss);
};

/*
class FullyConnectedExample: public BaseExample 
{
  void createMainModule(std::ostream& oss);
  void createLibraryModule(int idxModule, std::ostream& oss);
};
*/

class DiamondExample: public BaseExample 
{
  private:
    void createMainModule(std::ostream& oss);
    void createLibraryModule(int idxModule, std::ostream& oss);
  public:
    void setAll(int NumConstant, int NumPredicate, int SizeOfHead, int SizeOfBody, int NotProbability, int NumRules, int NumModules, std::string& OutputFilePrefix);
};


class RandomExample: public BaseExample 
{
  private:
    int density;
    void createMainModule(std::ostream& oss);
    void createLibraryModule(int idxModule, std::ostream& oss);
  public:
    void setAll(int NumConstant, int NumPredicate, int SizeOfHead, int SizeOfBody, int NotProbability, int NumRules, int NumModules, std::string& OutputFilePrefix, int Density=50);
};

// set all params
void BaseExample::setAll(int NumConstant, int NumPredicate, int SizeOfHead, int SizeOfBody, int NotProbability, int NumRules, int NumModules, std::string& OutputFilePrefix)
{
  outputFilePrefix = OutputFilePrefix;
  numConstant = NumConstant;
  numPredicate = NumPredicate;
  sizeOfHead = SizeOfHead;
  sizeOfBody = SizeOfBody;
  notProbability = NotProbability;
  numRules = NumRules;
  numModules = NumModules;
}


// create module header: #module(..., [...]).
void BaseExample::createModuleHeader(int idxModule, int numParam, std::ostream& oss)
{
  oss << "#module(mod" << idxModule << ", [";
  for (int i=0;i<numParam;i++)
    {
      if (i>0) oss << ", ";
      oss << "p" << idxModule << "p" << i << "/1";
    }
  oss << "]).";  
}


// generate facts, automatically prefix the predicate symbol to p<idxModule>p<idPred>... 
// input params: idxModule, numConstant, numPredicate, numFacts to be generated
void BaseExample::generateFacts(int idxModule, std::ostream& oss)
{
  int constant;
  int predicate;
  for (int i=0;i<numFacts;i++)
    {
      predicate = rand() % numPredicate;
      constant = rand() % numConstant;
      oss << "p" << idxModule << "p" << predicate << "(c" << constant << "). ";           
    }
}


// generate rules
// input params: idxModule, numConstant, numPredicate, sizeOfHead, sizeOfBody, notProbability, numRules to be generated
void BaseExample::generateRules(int idxModule, std::ostream& oss)
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
	  predicate = rand() % numPredicate;
	  constant = rand() % numConstant;
	  if ( j>0 ) oss << " v ";
          vars = j+65;  // generate variable
          oss << "p" << idxModule << "p" << predicate << "(" << vars << ")";           
	} 
      oss << " :- ";
      // generate body
      for (int j=0;j<sizeOfHead;j++)
	{
	  predicate = rand() % numPredicate;
	  vars = (j) + 65; // generate variables
	  notProbs = rand() % 100;
	  if ( j>0 ) oss << ", ";
          oss << "p" << idxModule <<"p" << predicate << "(" << vars << ")";           
	} 
      for (int j=sizeOfHead;j<sizeOfBody;j++)
	{
	  predicate = rand() % numPredicate;
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
void BaseExample::generateModuleCall(int idxModuleSrc, int idxModuleDest, int numInputPreds, std::ostream& oss)
{
  oss << "out" << idxModuleSrc << " :- @mod" << idxModuleDest << "[";
  for (int i=0;i<numInputPreds;i++) 
    {
      if (i > 0) oss <<",";
      oss << "p" << idxModuleSrc << "p" << rand()%numPredicate;
    }
  oss << "]::out" << idxModuleDest << ".";
}


void BaseExample::generate()
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
void StarExample::createMainModule(std::ostream& oss)
{
  // generate fact as many as the rules
  createModuleHeader(0, 0, oss);
  oss << std::endl;
  // generate facts
  numFacts = numConstant*numPredicate*2/3;
  generateFacts(0, oss);
  oss << std::endl;

  // generate rules
  generateRules(0, oss);

  // generate module calls 
  for (int i=1;i<numModules;i++)
    {
      generateModuleCall(0, i, 1, oss);
      oss << std::endl;
    }
} 


void StarExample::createLibraryModule(int idxModule, std::ostream& oss)
{
  // generate fact as many as the rules
  createModuleHeader(idxModule, 1, oss);
  oss << std::endl;

  // generate facts
  numFacts = numConstant*numPredicate*2/3;
  generateFacts(idxModule, oss);
  oss << std::endl;

  // generate rules
  generateRules(idxModule, oss);

  // generate module calls 
  generateModuleCall(idxModule, idxModule, 1, oss);
}



/******************* 
 * For Line topology
 *******************/
void LineExample::createMainModule(std::ostream& oss)
{
  // generate fact as many as the rules
  createModuleHeader(0, 0, oss);
  oss << std::endl;
  // generate facts
  numFacts = numConstant*numPredicate*2/3;
  generateFacts(0, oss);
  oss << std::endl;

  // generate rules
  generateRules(0, oss);

  if ( numModules > 1 ) generateModuleCall(0, 1, 1, oss);
} 


void LineExample::createLibraryModule(int idxModule, std::ostream& oss)
{
  // generate fact as many as the rules
  createModuleHeader(idxModule, 1, oss);
  oss << std::endl;

  // generate facts
  numFacts = numConstant*numPredicate*2/3;
  generateFacts(idxModule, oss);
  oss << std::endl;

  // generate rules
  generateRules(idxModule, oss);

  // generate module calls 
  if ( idxModule == numModules-1 )
    generateModuleCall(idxModule, idxModule, 1, oss);
  else 
    generateModuleCall(idxModule, idxModule+1, 1, oss);
}



/******************* 
 * For Ring topology
 *******************/
void RingExample::createMainModule(std::ostream& oss)
{
  // generate fact as many as the rules
  createModuleHeader(0, 0, oss);
  oss << std::endl;
  // generate facts
  numFacts = numConstant*numPredicate*2/3;
  generateFacts(0, oss);
  oss << std::endl;

  // generate rules
  generateRules(0, oss);

  if ( numModules > 1 ) generateModuleCall(0, 1, 1, oss);
} 


void RingExample::createLibraryModule(int idxModule, std::ostream& oss)
{
  // generate fact as many as the rules
  createModuleHeader(idxModule, 1, oss);
  oss << std::endl;

  // generate facts
  numFacts = numConstant*numPredicate*2/3;
  generateFacts(idxModule, oss);
  oss << std::endl;

  // generate rules
  generateRules(idxModule, oss);

  // generate module calls 
  if ( idxModule == numModules-1 )
    generateModuleCall(idxModule, 0, 0, oss);
  else 
    generateModuleCall(idxModule, idxModule+1, 1, oss);
}



/****************************** 
 * For Fully Connected topology
 ******************************/
/*
void FullyConnectedExample::createMainModule(std::ostream& oss)
{
  // generate fact as many as the rules
  createModuleHeader(0, 0, oss);
  oss << std::endl;
  // generate facts
  numFacts = numConstant*numPredicate*2/3;
  generateFacts(0, oss);
  oss << std::endl;

  // generate rules
  generateRules(0, oss);
 
  for (int i=1;i<numModules;i++)
    {
      generateModuleCall(0, i, 1, oss);
      oss << std::endl;
    }
} 


void FullyConnectedExample::createLibraryModule(int idxModule, std::ostream& oss)
{
  // generate fact as many as the rules
  createModuleHeader(idxModule, 1, oss);
  oss << std::endl;

  // generate facts
  numFacts = numConstant*numPredicate*2/3;
  generateFacts(idxModule, oss);
  oss << std::endl;

  // generate rules
  generateRules(idxModule, oss);

  // generate module calls 
  if (idxModule != 0)
    {
      generateModuleCall(idxModule, 0, 0, oss);
      oss << std::endl;
    }
  for (int i=1;i<numModules;i++)
    {
      if ( i!=idxModule ) 
	{
	  generateModuleCall(idxModule, i, 1, oss);
      	  oss << std::endl;
	}
    }
}
*/


/********************** 
 * For Diamond topology
 **********************/
// set all params
void DiamondExample::setAll(int NumConstant, int NumPredicate, int SizeOfHead, int SizeOfBody, int NotProbability, int NumRules, int NumModules, std::string& OutputFilePrefix)
{
  outputFilePrefix = OutputFilePrefix;
  numConstant = NumConstant;
  numPredicate = NumPredicate;
  sizeOfHead = SizeOfHead;
  sizeOfBody = SizeOfBody;
  notProbability = NotProbability;
  numRules = NumRules;
  numModules = NumModules*3+1;
}

void DiamondExample::createMainModule(std::ostream& oss)
{
  // generate fact as many as the rules
  createModuleHeader(0, 0, oss);
  oss << std::endl;
  // generate facts
  numFacts = numConstant*numPredicate*2/3;
  generateFacts(0, oss);
  oss << std::endl;

  // generate rules
  generateRules(0, oss);

  if ( numModules > 1 ) 
    {
      generateModuleCall(0, 1, 1, oss);
      oss << std::endl;
      generateModuleCall(0, 2, 1, oss);
      oss << std::endl;
    }
} 


void DiamondExample::createLibraryModule(int idxModule, std::ostream& oss)
{
  // generate fact as many as the rules
  createModuleHeader(idxModule, 1, oss);
  oss << std::endl;

  // generate facts
  numFacts = numConstant*numPredicate*2/3;
  generateFacts(idxModule, oss);
  oss << std::endl;

  // generate rules
  generateRules(idxModule, oss);

  // generate module calls 
  if ( idxModule == numModules-1) generateModuleCall(idxModule, idxModule, 1, oss);
  else if ( (idxModule+1)%3 == 0 ) generateModuleCall(idxModule, idxModule+1, 1, oss);
  else if ( (idxModule+2)%3 == 0 ) generateModuleCall(idxModule, idxModule+2, 1, oss);
  else if ( idxModule%3 == 0 )
    {
      generateModuleCall(idxModule, idxModule+1, 1, oss);
      oss << std::endl;
      generateModuleCall(idxModule, idxModule+2, 1, oss);
      oss << std::endl;
    }

}


/********************* 
 * For Random topology
 *********************/
// set all params
void RandomExample::setAll(int NumConstant, int NumPredicate, int SizeOfHead, int SizeOfBody, int NotProbability, int NumRules, int NumModules, std::string& OutputFilePrefix, int Density)
{
  outputFilePrefix = OutputFilePrefix;
  numConstant = NumConstant;
  numPredicate = NumPredicate;
  sizeOfHead = SizeOfHead;
  sizeOfBody = SizeOfBody;
  notProbability = NotProbability;
  numRules = NumRules;
  numModules = NumModules;
  density = Density;
  //rmv. std::cerr << "density: " << density << std::endl;
}

void RandomExample::createMainModule(std::ostream& oss)
{
  // generate fact as many as the rules
  createModuleHeader(0, 0, oss);
  oss << std::endl;
  // generate facts
  numFacts = numConstant*numPredicate*2/3;
  generateFacts(0, oss);
  oss << std::endl;

  // generate rules
  generateRules(0, oss);
  bool moduleCall = false;
  for (int i=1;i<numModules;i++)
    {
      if ( rand() % 100 < density ) 
        {
          generateModuleCall(0, i, 1, oss);
          oss << std::endl;
	  moduleCall = true;
        }
    }
  if ( moduleCall == false )
    {
      oss << "out0.";
    }
} 


void RandomExample::createLibraryModule(int idxModule, std::ostream& oss)
{
  // generate fact as many as the rules
  createModuleHeader(idxModule, 1, oss);
  oss << std::endl;

  // generate facts
  numFacts = numConstant*numPredicate*2/3;
  generateFacts(idxModule, oss);
  oss << std::endl;

  // generate rules
  generateRules(idxModule, oss);

  // generate module calls
  bool moduleCall = false;
  if ( rand() % 100 < density ) 
    {
      generateModuleCall(idxModule, 0, 0, oss);
      oss << std::endl;
      moduleCall = true;
    }
  for (int i=1;i<numModules;i++)
    {
      if ( rand() % 100 < density ) 
        {
          generateModuleCall(idxModule, i, 1, oss);
          oss << std::endl;
          moduleCall = true;
        }
    }
  if ( moduleCall == false )
    {
      oss << "out" << idxModule << ".";
    }
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
	  StarExample example;
	  example.setAll(param[0], param[1], param[2], param[3], param[4], param[5], param[6], outputFilePrefix);
	  example.generate();
	}
      else if (topology == "line")
	{
	  LineExample example;
	  example.setAll(param[0], param[1], param[2], param[3], param[4], param[5], param[6], outputFilePrefix);
	  example.generate();
	}
      else if (topology == "ring")
	{
	  RingExample example;
	  example.setAll(param[0], param[1], param[2], param[3], param[4], param[5], param[6], outputFilePrefix);
	  example.generate();
	}
/*
      else if (topology == "fully")
	{
	  FullyConnectedExample example;
	  example.setAll(param[0], param[1], param[2], param[3], param[4], param[5], param[6], outputFilePrefix);
	  example.generate();
	}
*/
      else if (topology == "diamond")
	{
	  DiamondExample example;
	  example.setAll(param[0], param[1], param[2], param[3], param[4], param[5], param[6], outputFilePrefix);
	  example.generate();
	}
      else if (topology == "random")
	{
	  RandomExample example;
	  if (argc == numParam + 2)
	    {
	      example.setAll(param[0], param[1], param[2], param[3], param[4], param[5], param[6], outputFilePrefix, atoi(argv[numParam+1]));
	      //rmv. std::cerr << "with density: " << argv[numParam+1] << std::endl;
	    }
          else 
	    {
	      example.setAll(param[0], param[1], param[2], param[3], param[4], param[5], param[6], outputFilePrefix);
	      //rmv. std::cerr << "with empty density" << std::endl;
	    }
	  example.generate();
	}
    }


}




