
//****************************************************************//
//*                                                              *//
//*------------------- Includes SECTION -------------------------*//
//*                                                              *//
//****************************************************************//
#include <math.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <iomanip>
#include <algorithm>
#include "Stats.hxx"





//****************************************************************//
//*                                                              *//
//*------------- Private Variables SECTION ----------------------*//
//*                                                              *//
//****************************************************************//









//****************************************************************//
//*                                                              *//
//*------------- Public Variables SECTION -----------------------*//
//*                                                              *//
//****************************************************************//




//****************************************************************//
//*                                                              *//
//*----------- Private Functions Definition SECTION -------------*//
//*                                                              *//
//****************************************************************//




//****************************************************************//
//*                                                              *//
//*--------------- Public Methods SECTION -----------------------*//
//*                                                              *//
//****************************************************************//

//----------------------------------------------------------------//
// Public method:   run_stats                                     //
//----------------------------------------------------------------//

void Stats::run_stats() {
  int    n = this->size();
  double x;

  // sort the vector
  sort(this->begin(), this->end());
  
  // compute the mean, freq, min and max
  mean_ = 0.0;
  min_ = 1.0e10;
  max_ = 0.0;
  freq_ = 0.0;
  for (int i=0; i<n; i++) {
    x = this->at(i);
    mean_ += x;
    if ( x <= 1.0 )
      freq_ += 1.0;
    if ( x < min_ )
      min_ = x;
    if ( x > max_ )
      max_ = x;
  }
  mean_ /= n;
  freq_ /= n;
  
  // compute the median
  if ( n%2 == 1 )
    median_ = this->at((n-1)/2);
  else
    median_ = (this->at((n-1)/2)+this->at((n-1)/2+1))/2.0;    

  // compute the standard deviation
  stddev_ = 0.0;
  for (int i=0; i<n; i++)
    stddev_ += pow(this->at(i)-mean_,2);
  stddev_ = sqrt(stddev_/n);
}


//----------------------------------------------------------------//
// Public method:   print                                         //
//----------------------------------------------------------------//

void Stats::print() {
  int n = this->size();

  cerr << "data: ";
  for (int i=0; i<n; i++)
    cerr << this->at(i) << ", ";
  cerr << endl;
}


//----------------------------------------------------------------//
// Public method:   print_stats                                   //
//----------------------------------------------------------------//

void Stats::print_stats() {
  cerr << "mean=" << mean_ << ", "
       << "min=" << min_ << ", "
       << "max=" << max_ << ", "
       << "median=" << median_ << ", "
       << " std dev=" << stddev_ 
       << endl;
}


//----------------------------------------------------------------//
// Public method:   print_stats                                   //
//----------------------------------------------------------------//

void Stats::write_cdf(const char * dirName, int index, string prefix) {
  int           n = this->size();
  stringstream  s;
  string        outputFileName;
  string        indexStr;
  
  //ouverture du fichier de sortie
  s << index;
  indexStr= s.str();
  outputFileName = string(dirName) + "/cdf" + prefix + "_"+ indexStr + ".txt";
  
  ofstream    outFile(outputFileName.c_str());

  // ecriture dans le fichier
  for (int i=0; i<n; i++) 
    outFile << this->at(i) << "\t" << ((double) (i+1))/((double) n) << endl;

  outFile.close();
}


