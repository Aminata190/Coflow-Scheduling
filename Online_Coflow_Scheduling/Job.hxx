#ifndef __Job_hxx__
#define __Job_hxx__



//****************************************************************//
//*                                                              *//
//*------------------- Includes SECTION -------------------------*//
//*                                                              *//
//****************************************************************//

#include <fstream>
#include <iostream>
#include <string>


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
// Class :      Job                                               //
//                                                                //
//                                                                //
// Description : classe representant un job a scheduler           //
//                                                                //
//----------------------------------------------------------------//


class  Job
{
  // attributs prives 
private:
  int              id_; 
  double           p_; // processing time
  double           d_; // due date
  double           c_; // completion time
  
  // methodes de classe 
public:

  // constructeurs 
  Job() {}


  Job(int id, double p, double d): 
    id_(id),
    p_(p),
    d_(d),
    c_(0.0)
  { }

  Job(const Job &other): 
    id_(other.id_),
    p_(other.p_),
    d_(other.d_),
    c_(other.c_)
  {}

  // accesseurs 

  int    id() const { return id_; }

  double processingTime() const { return p_; }

  double dueDate() const { return d_; }

  double completionTime() const { return c_; }

  void   setId(int j) { id_ = j; }

  void   setProcessingTime(double p) { p_ = p; }

  void   setDueDate(double d) { d_ = d; }

  void   setCompletionTime(double c) { c_ = c; }


  // comparison operators
  friend bool operator< (const Job& lhs, const Job& rhs){ return lhs.d_ < rhs.d_; }
  friend bool operator> (const Job& lhs, const Job& rhs){ return rhs < lhs; }
  friend bool operator<=(const Job& lhs, const Job& rhs){ return !(lhs > rhs); }
  friend bool operator>=(const Job& lhs, const Job& rhs){ return !(lhs < rhs); }

  friend bool operator==(const Job& lhs, const Job& rhs){ return (lhs.id_==rhs.id_) && (lhs.p_==rhs.p_) && (lhs.d_==rhs.d_); }
  friend bool operator!=(const Job& lhs, const Job& rhs){ return !(lhs == rhs); }

  
  // methodes de calcul
};


#endif
