#ifndef __Stats_hxx__
#define __Stats_hxx__



//****************************************************************//
//*                                                              *//
//*------------------- Includes SECTION -------------------------*//
//*                                                              *//
//****************************************************************//

#include <fstream>
#include <iostream>
#include <vector>


using namespace std;




//****************************************************************//
//*                                                              *//
//------------ Public Data Structures SECTION --------------------//
//*                                                              *//
//****************************************************************//





//****************************************************************//
//*                                                              *//
//----------------- Public Classes SECTION -----------------------//
//*                                                              *//
//****************************************************************//



//----------------------------------------------------------------//
// Class :      Stats                                             //
//                                                                //
//                                                                //
// Description : classe pour faire des statistiques               //
//                                                                //
//----------------------------------------------------------------//


class  Stats: public vector<double>
{
  // attributs prives 
private:
  double           mean_;   // mean
  double           min_;    // minimum
  double           max_;    // maximum
  double           median_; // median
  double           stddev_; // standard deviation
  double           freq_;   // frequency of values smaller than 1

  // methodes de classe 
public:

  // constructeurs 
  Stats():
    mean_(0.0),
    min_(0.0),
    max_(1.0e3),
    median_(0.0),
    stddev_(0.0),
    freq_(0.0)
  {}

  Stats(const Stats &other): 
    mean_(other.mean_),
    min_(other.min_),
    max_(other.max_),
    median_(other.median_),
    stddev_(other.stddev_),
    freq_(other.freq_)
  {}

  // accesseurs 
  double   mean() { return mean_; }

  double   minimum() { return min_; }

  double   maximum() { return max_; }

  double   median() { return median_; }

  double   stddev() { return stddev_; }

  double   frequency() { return freq_; }
  
  void     run_stats();

  void     print();

  void     print_stats();

  void     write_cdf(const char * dirName, int index, string prefix);
};


#endif
