
#include "EventList.hxx"
#include <fstream>



Event EventList::nextEvent() { 
  Event e = evtList_.top();

  evtList_.pop();
  return e;
}

void  EventList::addEvent(Event e) {
  //  cerr << "Evenement de type " << e.getType() << " ajouté a la date " << e.getDate() << " pour le flot " << e.getFlow() << endl;
  evtList_.push( e );
}

void EventList::reset() {
  while ( !evtList_.empty() )
    evtList_.pop();
}

double EventList::removeEvent(Event::EventType typeEvt, int flowId) {
  priority_queue<Event>   updatedCopy;
  Event                   e;
  double                  result;
  
  while ( evtList_.empty() == false ) {
    
    // on recupere le prochain evenement
    e = evtList_.top();
    evtList_.pop();
    
    // on saute l'evenement a enlever
    if ( (e.getType() == typeEvt) && (e.getFlow() == flowId) ) {
      result = e.getInsertionTime();
      continue;
    }

    // on ajoute les autres a la copie
    updatedCopy.push(e);
  }

  //on recopie
  evtList_ = updatedCopy;

  return result;
}


void EventList::update(map<int,double> & departureTimes) {
  priority_queue<Event>     updatedCopy;
  map<int,double>::iterator it;
  Event                     e;
  int                       i;
  
  while ( evtList_.empty() == false ) {
    
    // on recupere le prochain evenement
    e = evtList_.top();
    evtList_.pop();

    //on verifie si la date de depart a changee
    i = e.getFlow();
    it = departureTimes.find(i);
    if ( it != departureTimes.end() )
      e.setDate( it->second );

    // on ajoute l'evenement a la copie
    updatedCopy.push(e);
  }

  //on recopie
  evtList_ = updatedCopy;
}
