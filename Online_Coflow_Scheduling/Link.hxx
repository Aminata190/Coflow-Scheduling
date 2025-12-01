#ifndef __Link_hxx__
#define __Link_hxx__



//****************************************************************//
//*                                                              *//
//*------------------- Includes SECTION -------------------------*//
//*                                                              *//
//****************************************************************//

#include <fstream>
#include <iostream>
#include <string>
#include <set>


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
// Class :      Link                                              //
//                                                                //
//                                                                //
// Description : classe representant un lien de communication     //
//                                                                //
//----------------------------------------------------------------//


class  Link
{
public:
  enum LinkType { IDLE=0, BUSY};

  // attributs prives 
private:
  int              id_;    // external id
  double           capa_;  // capacity
  double           r_;     // release date
  double           ctime_;  // completion time
  LinkType         status_; //IDLE or BUSY
  int              flow_; // id du flot utilisant le lien
  
  // methodes amies
public:
  friend   istream   &   operator>>(istream &stream, Link &);
  friend   ostream   &   operator<<( ostream &, Link const & );

  
  // methodes de classe 
public:

  // constructeurs 
  Link():
    id_(-1),
    capa_(0.0),
    r_(0.0),
    ctime_(0.0),
    status_(IDLE),
    flow_(-1)
  {}


  Link(int id, double capa): 
    id_(id),
    capa_(capa),
    r_(0.0),
    ctime_(0.0),
    status_(IDLE),
    flow_(-1)
  {}

  Link(const Link &other): 
    id_(other.id_),
    capa_(other.capa_),
    r_(other.r_),
    ctime_(other.ctime_),
    status_(other.status_),
    flow_(other.flow_)
  {}

  // accesseurs 

  int    getId() const { return id_; }

  double getCapa() const { return capa_; }

  double release_date() const { return r_; }

  double completion_time() const { return ctime_; }
  
  LinkType getStatus() const { return status_; }

  int    getFlow() const { return flow_; }
  
  void   setId(int f) { id_ = f; }

  void   setCapa(double x) { capa_ = x; }

  void   setReleaseDate(double x) { r_ = x; }

  void   setCompletionTime(double x) { ctime_ = x; }

  void   updateCompletionTime(double x) { ctime_ += x; }
  
  void   setStatus(LinkType t) { status_ = t; }

  void   setFlow(int i) { flow_ = i; }
  
  // methodes de calcul
  void   acquire(int i) { status_=BUSY; flow_=i; }
  
  void   release() { status_=IDLE; flow_=-1; }  
};


#endif
