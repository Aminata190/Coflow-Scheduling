#ifndef __eventlist_hxx__

#define __eventlist_hxx__




//****************************************************************//
//*                                                              *//
//*------------------- Includes SECTION -------------------------*//
//*                                                              *//
//****************************************************************//

#include <queue>
#include <map>

#include "lois.hxx"


//****************************************************************//
//*                                                              *//
//------------ Public Data Structures SECTION --------------------//
//*                                                              *//
//****************************************************************//

using namespace std;





//****************************************************************//
//*                                                              *//
//----------------- Public Classes SECTION -----------------------//
//*                                                              *//
//****************************************************************//


//----------------------------------------------------------------//
// Class :    Event                                               //
//                                                                //
//                                                                //
// Description : classe representant un evenement                 //
//                                                                //
//----------------------------------------------------------------//

class Event
{
public:
  enum EventType { ARRIVAL=1, DEPARTURE, UPDATE, END};

private:

  double     date_;
  double     insertionTime_;
  EventType  type_;
  int        flow_;
  
public:

  //----------------------------------------------------------------//
  //                 CONSTRUCTEURS ET DESTRUCTEUR                   //
  //----------------------------------------------------------------//

  ///Constructeur par defaut
  Event( double t=0.0, double insertTime=0.0, EventType e=ARRIVAL, int f=1000000):
    date_(t),
    insertionTime_(insertTime),
    type_(e),
    flow_(f)
  {}

  ///Constructeur par recopie
  Event( Event const &other ):
    date_(other.date_),
    insertionTime_(other.insertionTime_),
    type_(other.type_),
    flow_(other.flow_)
  {}
  
  //----------------------------------------------------------------//
  //                   ACCESSEURS                                   //
  //----------------------------------------------------------------//

  double    getDate() { return date_; }

  double    getInsertionTime() { return insertionTime_; }

  EventType getType() { return type_; }

  int       getFlow() { return flow_; }

  void      setDate(double d) { date_ = d; }
  
  
  //----------------------------------------------------------------//
  //                   ORDRE                                        //
  // Un evenement est d'autant plus prioritaire que sa date est     //
  // petite (inversion liee au priority_queue)                      //
  //----------------------------------------------------------------//
  
  bool operator < (Event const &right) const {
    if ( date_ > right.date_ )
      return true;
    // if ( (date_==right.date_) && (type_==Event::UPDATE) ) 
    //   return true;
    if ( (date_==right.date_) && (flow_ > right.flow_) ) 
       	return true;
    return false;
  }

};



//----------------------------------------------------------------//
// Class :   EventList                                            //
//                                                                //
//                                                                //
// Description : classe representant un echeancier                //
//                                                                //
//----------------------------------------------------------------//


class EventList
{
  //attributs
private:
  priority_queue<Event>   evtList_;        //Echeancier                           



public:

  //----------------------------------------------------------------//
  //                 CONSTRUCTEURS ET DESTRUCTEUR                   //
  //----------------------------------------------------------------//

  ///Constructeur par defaut
  EventList()
  { }

  // destructeur
  ~EventList()
  { }


  //----------------------------------------------------------------//
  //                   ACCESSEURS                                   //
  //----------------------------------------------------------------//


  //----------------------------------------------------------------//
  //               METHODES DE CALCUL                               //
  //----------------------------------------------------------------//

  Event  nextEvent();

  void   addEvent(Event e);

  double removeEvent(Event::EventType typeEvt, int flowId);

  void   reset();

  bool   empty() { return evtList_.empty(); }

  int    size() { return evtList_.size(); }

  void   update(map<int,double> & departureTimes);
};




#endif
