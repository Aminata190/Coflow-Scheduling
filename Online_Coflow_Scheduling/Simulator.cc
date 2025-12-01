//****************************************************************//
//*                                                              *//
//*------------------- Includes SECTION -------------------------*//
//*                                                              *//
//***************************************************************//

//#include <stdlib.h>
//#include <stdio.h>
#include <iomanip>
#include <math.h>
#include <set>
#include <iterator>
#include <algorithm>

#include "Simulator.hxx"





//****************************************************************//
//*                                                              *//
//*------------- Private Variables SECTION ----------------------*//
//*                                                              *//
//****************************************************************//


#define INFTY  1.0e20



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

//----------------------------------------------------------------//
// Generic function:  contains                                    //
// Find if an element of any type exists in list                  //
//----------------------------------------------------------------//

bool contains(list<int> & listOfElements, const int & element)
{
    // Find the iterator if element in list
  list<int>::iterator it = std::find(listOfElements.begin(), listOfElements.end(), element);
    //return if iterator points to end or not. It points to end then it means element
    // does not exists in list
  return it != listOfElements.end();
}



//****************************************************************//
//*                                                              *//
//*--------------- Public Methods SECTION -----------------------*//
//*                                                              *//
//****************************************************************//


//----------------------------------------------------------------//
// Public Method: number_of_active_coflows                        //
// Cette methode determine les coflots dans le systeme a partir de//
// la liste des flots pending et de celle des flots running.      //
//----------------------------------------------------------------//

void Simulator::set_of_active_coflows(list<int> & released, list<int> & running, set<int> & set_coflow_id) {
  list<int>::iterator it = released.begin();
  int                 m;
  
  for ( ; it != released.end(); it++) {
    m = flowIdMap_[*it].first;   //coflow correspondant a ce flot
    set_coflow_id.insert(m);
  }
  // it = running.begin();

  // for ( ; it != running.end(); it++) {
  //   m = flowIdMap_[*it].first;   //coflow correspondant a ce flot
  //   set_coflow_id.insert(m);
  // }  
}



//----------------------------------------------------------------//
// Public Method: clone                                           //
// Cette methode construit un objet Network a partir de l'objet   //
// Simulator courant. Les liens sont identiques. Les coflows      //
// sont ceux en cours d'execution dans la simulation.             //
//----------------------------------------------------------------//

void Simulator::clone(Network & copy, list<int> & released, list<int> & running, double t) {
  int                  n, i, j, k;
  set<int>             set_coflow_id;
  map<int,int>         nb_flows;
  set<int>::iterator   it;

  //version de DCOFLOW
  copy.setVersion(version_);
  
  //Coflows  
  set_of_active_coflows(released,running,set_coflow_id);
  n = set_coflow_id.size();
  copy.setNumberofCoflows(n);
  k = 0;
  nb_flows[k] = 0;
  it = set_coflow_id.begin();
  for ( ; it != set_coflow_id.end(); it++, k++) {
    copy.addCoflow(k,coflow_[*it],t);
    j = coflow_[*it].getNbFlow();    
    for (i=0; i<j; i++) {
      int     id = coflow_[*it].getFlowId(i);

      if ( contains(released,id) || contains(running,id) ) {
        copy.addFlow(k,nb_flows[k],id,coflow_[*it].getFlow(i), true);
        nb_flows[k]++;
      }
    }
    copy.setNumberofFlows(k, nb_flows[k]);
  }
  
  //read links
  // cerr << "in Clone nbLinks_ : " <<nbLinks_<< endl;
  copy.setNumberofLinks( nbLinks_ );
  for (i=0; i<nbLinks_; i++)
    copy.addLink(i,link_[i]);

  
  //compute time slots
  copy.compute_time_slots();
}

// New version -not complete 
// released is a list of coflow id
// void Simulator::clone(Network & copy, list<int> & released, list<int> & running, double t) {
//   int                  n, i, j, k;
//   set<int>             set_coflow_id;
//   map<int,int>         nb_flows;
//   set<int>::iterator   it;

//   //version de DCOFLOW
//   copy.setVersion(version_);
  
//   //Coflows
//   set_coflow_id.insert(released.begin(),released.end());
//   set_coflow_id.insert(running.begin(),running.end());

//   n = set_coflow_id.size();
//   copy.setNumberofCoflows(n);
//   k = 0;
//   nb_flows[k] = 0;
//   it = set_coflow_id.begin();
//   for ( ; it != set_coflow_id.end(); it++, k++) {
//     copy.addCoflow(k,coflow_[*it],t);
//     j = coflow_[*it].getNbFlow();    
//     for (i=0; i<j; i++) {
//       int     id = coflow_[*it].getFlowId(i);

//       if ( contains(released,id) || contains(running,id) ) {
//         copy.addFlow(k,nb_flows[k],id,coflow_[*it].getFlow(i), true);
//         nb_flows[k]++;
//       }
//     }
//     copy.setNumberofFlows(k, nb_flows[k]);
//   }
  
//   //read links
//   // cerr << "in Clone nbLinks_ : " <<nbLinks_<< endl;
//   copy.setNumberofLinks( nbLinks_ );
//   for (i=0; i<nbLinks_; i++)
//     copy.addLink(i,link_[i]);

//   // Problem ????????????????deeper?????????????????????????????
//   //compute time slots
//   copy.compute_time_slots();
// }



//----------------------------------------------------------------//
// Public Method:  update_residual_sizes                          //
// Methode appellee a chaque evenement pour mettre a jour la      //
// taille residuelle des flots en cours d'execution.              //
//----------------------------------------------------------------//

void Simulator::update_residual_sizes(list<int> & running, double Delta_t) {
  int                  f, k, i;
  list<int>::iterator  it = running.begin();
  double               min_capa, s;

  if ( Delta_t < 1.0e-10 )
    return;
  
  for ( ; it != running.end(); it++) {
    f = *it;
    k = flowIdMap_[f].first;
    i = flowIdMap_[f].second;
    min_capa = coflow_[k].getFlowMinCapa(i);
    s = coflow_[k].getFlowResidualSize(i,preemption_) - Delta_t*min_capa;
    coflow_[k].setFlowResidualSize(i,s,preemption_);
  }
}



//----------------------------------------------------------------//
// Public Method: remove_outdated_coflows                         //
// Methode appellee a chaque evenement pour enlever les coflows   //
// ayant depasse leur deadline de la liste pending.               //
//----------------------------------------------------------------//

void Simulator::remove_outdated_coflows(list<int> & pending, double t) {
  int                  f, k, i;
  list<int>::iterator  it = pending.begin();
  set<int>             del;
  set<int>::iterator   del_it;
  double               min_capa, s;

  for ( ; it != pending.end(); it++) {
    f = *it;
    k = flowIdMap_[f].first;
    i = flowIdMap_[f].second;
    if ( coflow_[k].getEndTime() < t ) {
      del.insert(f);
      coflow_[k].setFlowStatus(i,Flow::DONE);
      coflow_[k].updateCCT(INFTY);
    }
  }
  for ( del_it=del.begin(); del_it != del.end(); del_it++) 
    pending.remove(*del_it);
}





//----------------------------------------------------------------//
// Public Method:    preemptFlowOnLink                            //
// Cette methode est utilisee dans la simulation pour preempter   //
// le flot s'executant sur le lien l a l'instant t.               //
// On commence par liberer les liens utilises et on en profite    //
// pour determiner la capacite minimale de ces liens. On enleve   //
// ensuite l'evenement de depart associe a ce flot de l'echeancier//
// en recuperant la date a laquelle l'evenement a ete insere (c'  //
// est la date de debut d'execution). On met ensuite a jour le    //
// volume residuel du flot. Enfin, on l'enleve de la liste running//
// et on l'ajoute dans la liste pending.                          //
//----------------------------------------------------------------//

void Simulator::preemptFlowOnLink(double t, int l, list<int> & pending, list<int> & running, EventList & evtList) {
  int    f = link_[l].getFlow();
  int    k = flowIdMap_[f].first;
  int    i = flowIdMap_[f].second; 
  int    length = coflow_[k].getLength(i);
  int    m, j;
  double insertTime;
  double c;
  double min_capa = INFTY;

  if ( debugEvent_ )
    cerr << "\t=> Flow " << f << " is preempted on link " << link_[l].getId() << endl;
  
  //on marque les ports source et destination comme libres
  for (j=0; j<length; j++) {
    m = coflow_[k].getLink(i,j);
    m = linkIdMap_[m];
    link_[m].release();
  }

  //on enleve l'evenement de depart
  insertTime = evtList.removeEvent(Event::DEPARTURE,f);
  
  
  // on passe le flow de running a pending
  running.remove(f);
  coflow_[k].setFlowStatus(i,Flow::PENDING);
  pending.push_front(f);
}

void Simulator::preemptFlowOnLink_online(double t, int l, list<int> & pending, list<int> & running) {
  int    f = link_[l].getFlow();
  int    k = flowIdMap_[f].first;
  int    i = flowIdMap_[f].second; 
  int    length = coflow_[k].getLength(i);
  int    m, j;
  double c;

  if ( debugEvent_ )
    cerr << "\t=> Flow " << f << " is preempted on link " << link_[l].getId() << endl;
  
  //on marque les ports source et destination comme libres
  for (j=0; j<length; j++) {
    m = coflow_[k].getLink(i,j);
    m = linkIdMap_[m];
    link_[m].release();
  }
  
  
  // on passe le flow de running a pending
  running.remove(f);
  coflow_[k].setFlowStatus(i,Flow::PENDING);
  pending.push_front(f);
}



//----------------------------------------------------------------//
// Public Method:    run_flow                                     //
// Cette methode est utilisee dans la simulation pour executer    //
// le flow flow_id a l'instant t. Il passe de la liste pending    //
// a la liste running et on change son status. Les liens utilises //
// sont egalement marques comme busy. Enfin on ajoute l'evenement //
// de depart du flow.                                             //
//----------------------------------------------------------------//

void Simulator::run_flow(double t, int flow_id, list<int> & pending, list<int> & running, EventList & evtList) {
  int    k = flowIdMap_[flow_id].first;
  int    i = flowIdMap_[flow_id].second;  
  int    length = coflow_[k].getLength(i); //longueur du chemin utilisé par le flow
  int    l, j;
  double duration = 0.0;
  double min_capa = INFTY;
  double c;

  // on passe le flow de pending a running
  pending.remove(flow_id);
  coflow_[k].setFlowStatus(i,Flow::RUNNING);
  cerr << "     flow " << flow_id<< " running at " << t << " on link " << coflow_[k].getLink(i,0) << " ->" <<coflow_[k].getLink(i,1)<< endl;
  running.push_front(flow_id);

  //on modifie le status des liens utilises
  for (j=0; j<length; j++) {
    l = coflow_[k].getLink(i,j);
    l = linkIdMap_[l];
    if ( link_[l].getStatus() == Link::BUSY )
      preemptFlowOnLink(t,l,pending, running,evtList);
    link_[l].acquire(flow_id);
    c = link_[l].getCapa();
    if ( c < min_capa )
      min_capa = c;
  }
  // on update le flow residual size
  double res_size = coflow_[k].getFlowResidualSize(i,preemption_) - min_capa;
  cerr << "   flow "<< flow_id<<" size: " << coflow_[k].getFlowResidualSize(i,preemption_) << " -> " << res_size << endl;

  if (res_size > 0.0){
    coflow_[k].setFlowResidualSize(i,res_size,preemption_);
    coflow_[k].AddToFlowExpectedCT(i,min_capa);
  }
  else { // flow finit son execution
    coflow_[k].AddToFlowExpectedCT(i,min_capa);
    coflow_[k].setFlowStatus(i,Flow::DONE);
    cerr << "   ***Flow " << flow_id << " fini avec cct: " << coflow_[k].getFlowExpectedCT(i) << endl;
    // it = running.erase(it); // supprime de running et avance l'iterateur
    // break;
  }
  // //on insere l'evenement de depart
  // duration = coflow_[k].getFlowResidualSize(i,preemption_)/min_capa;
  // cerr << " coflow duration: " << duration << endl;

  // evtList.addEvent( Event( t+duration, t, Event::DEPARTURE, flow_id) );
  // coflow_[k].setFlowExpectedCT(i,t+duration);
  // coflow_[k].setFlowStatus(i,Flow::DONE); // test
}



//----------------------------------------------------------------//
// Public Method:  getFlowPriorityOnlink                          //
// Cette methode prend en parametre un lien l (id interne). Elle  //
// determine le flot qui a acquis ce lien et retourne sa priorite //
//----------------------------------------------------------------//

int Simulator::getFlowPriorityOnlink(int l) {
  int f, k, i;

  // id externe du flot en cours sur le lien l
  f = link_[l].getFlow();
  if ( f == -1 ) {
    cerr << "Link " << link_[l].getId() << " is not busy => no flow has acquired it!" << endl;
    exit(-1);
  }
  
  // id interne du coflow correspondant
  k = flowIdMap_[f].first;

  // position dans le tableau flow_ de coflow_[b]
  i = flowIdMap_[f].second; 

  return coflow_[k].getFlowPriority(i);
}



//----------------------------------------------------------------//
// Public Method:  is_link_busy                                   //
// Cette methode prend en parametre un lien l et un flot identifie//
// par l'id interne k du coflow et la position i du flot dans le  //
// tableau flow_ de coflow_[k]. Si le lien est occupe et qu'il    //
// n'y a pas de preemption, elle retourne true. Si le lien est    //
// occupe mais que la premption est permise, elle ne retourne true//
// que si le flot s'executant sur le lien a un indice de priorite //
// plus petit (i.e. il est plus prioritaire). Dans tous les autres//
// cas, le lien peut etre acquis et elle retourne donc false.     //
//----------------------------------------------------------------//

bool Simulator::is_link_busy(int l, int k, int i) {
  int   p;
  int   q = coflow_[k].getFlowPriority(i);
  int   n = coflow_[k].getFlowId(i);
  
  if ( link_[l].getStatus() == Link::BUSY ) {
        //  cerr << "\t=> Link " << link_[l].getId() << " is busy for new flow " << n << endl;

    // Si la premption n'est pas autorise
    if ( preemption_ == false )
      return true;

    // Si la premption est autorise, on compare les priorites des flots
    p = getFlowPriorityOnlink(l);
    if ( q > p ) //pas de premption
      return true;
    if ( (q==p) && (n>=link_[l].getFlow()) ) //pas de premption
      return true;
  }

  return false;
}



//----------------------------------------------------------------//
// Public Method:    is_flow_ready                                //
// Cette methode verifie si les ports source et destination du    //
// flow flow_id sont dans l'etat IDLE et retourne vrai si c'est   //
// le cas, et faux sinon.                                         //
//----------------------------------------------------------------//

bool Simulator::is_flow_ready(int flow_id) {
  int    k = flowIdMap_[flow_id].first;
  int    i = flowIdMap_[flow_id].second;  
  int    length = coflow_[k].getLength(i);
  int    l, j, a, b, c, p;
  bool   result = true;
  
  //on verifie si les ports source et destination sont libres ou preemptables
  for (j=0; j<length; j++) {
    l = coflow_[k].getLink(i,j);
    l = linkIdMap_[l];
    if ( is_link_busy(l,k,i) ) {
      // cerr <<" link "<< l <<" is busy" << endl;
	    result = false;
	  break;
    }
  }

  return result;
}


//----------------------------------------------------------------//
// Public Method:   remove_flow                                   //
// Cette methode prend en parametre le flow flow_id et indique    //
// que les ports source et destination sont a l'etat IDLE.        //
//----------------------------------------------------------------//

void Simulator::remove_flow(int flow_id) {
  int    k = flowIdMap_[flow_id].first;
  int    i = flowIdMap_[flow_id].second;  
  int    length = coflow_[k].getLength(i);
  int    l, j;
  
  //on marque les ports source et destination comme libres
  for (j=0; j<length; j++) {
    l = coflow_[k].getLink(i,j);
    l = linkIdMap_[l];
    link_[l].release();
    // cerr << " ---link " << l << " released" << endl;
  }
}



//----------------------------------------------------------------//
// Public Method:    new_flow_arrival                             //
// Cette methode est appellee par la simulation lorsque le flow   //
// flow_id arrive a l'instant t. Il est marque comme pending et   //
// ajoute a la liste des flows pending. On verifie ensuite si le  //
// flow est pret a etre execute. Si c'est le cas, on fait appel   //
// a la methode run_flow.                                         //
//----------------------------------------------------------------//

void Simulator::new_flow_arrival(double t, int flow_id, list<int> & pending, list<int> & running, EventList & evtList) {
  int    k = flowIdMap_[flow_id].first; //coflow id
  int    i = flowIdMap_[flow_id].second;  // position of flow in coflow
  double duration = 0.0;
  double min_capa = INFTY;
  int    next_flow = 0;

  //on ne considere par le flot si sa priorite est MIN_PRIORITY
  if ( coflow_[k].getFlowPriority(i) == MIN_PRIORITY )
    return;
  
  //on change le status du flow et on l'ajoute a la liste des flows pending
  coflow_[k].setFlowStatus(i,Flow::PENDING);
  pending.push_front(flow_id);

  //Determination des prochains flots a executer (if any) parmi les pending
  //Il peut y avoir ceux qui viennent d'arriver
  while ( next_flow != -1 ) {
    next_flow = choose_next_flow(pending); // next flow with the small priority
    if ( next_flow != -1 ) {
      k = flowIdMap_[next_flow].first;
      i = flowIdMap_[next_flow].second;  
      if ( debugEvent_ )
	cerr << "\t=> Starting flow " << next_flow
	     << " after arrival  of flow " << flow_id
	     << "(priority=" << coflow_[k].getFlowPriority(i) << ")"
	     << endl;
      run_flow(t, next_flow, pending, running, evtList);
    }
  }
}




//----------------------------------------------------------------//
// Public Method:    new_flow_departure                           //
// Cette methode est appele par la simulation a l'instant t quand //
// le flow flow_id quitte le systeme. On le marque comme termine  //
// et on l'enleve de la liste running. On libere aussi les ports  //
// source et destination et on met a jour le CCT du coflow. Puis  //
// parmi les flots pending, on determine tous ceux qui peuvent    //
// commencer leur execution et on les lance avec run_flow.        //
//----------------------------------------------------------------//

void Simulator::new_flow_departure(double t, int flow_id, list<int> & pending, list<int> & running, EventList & evtList) {
  int    k = flowIdMap_[flow_id].first;
  int    i = flowIdMap_[flow_id].second;  
  int    next_flow = 0;
  
  //on change le status du flow et on le supprime des flows running
  coflow_[k].setFlowStatus(i,Flow::DONE);
  running.remove(flow_id);

  //on met a jour le CCT du coflow
  coflow_[k].updateCCT(t);
  
  //on libere les ports source et destination
  remove_flow(flow_id);

  //Determination des prochains flots a executer (if any) parmi les pending
  while ( next_flow != -1 ) {
    next_flow = choose_next_flow(pending);
    if ( next_flow != -1 ) {
      k = flowIdMap_[next_flow].first;
      i = flowIdMap_[next_flow].second;  
      if ( debugEvent_ )
		cerr << "\t=> Starting flow " << next_flow
	     << " after departure  of flow " << flow_id
	     << "(priority=" << coflow_[k].getFlowPriority(i) << ")"
	     << endl;
      run_flow(t, next_flow, pending, running, evtList);
    }
  }
}

//----------------------------------------------------------------//
// Public Method:    flow_departure                           //
// Cette methode est appele par la simulation a l'instant t quand //
// le flow flow_id quitte le systeme. On le marque comme termine  //
// et on l'enleve de la liste running. On libere aussi les ports  //
// source et destination et on met a jour le CCT du coflow.       //
//----------------------------------------------------------------//

void Simulator::flow_departure(double t,list<int> & running,list<int> & flow_list) {
  int             k ,i; 
  pair<int,int>   x;
  
  auto it = running.begin();
  while (it != running.end()) {
      int flow_id = *it;
      x = flowIdMap_[flow_id];
      k = x.first;
      i = x.second;

      if (coflow_[k].getFlowStatus(i) == Flow::DONE) {
          cerr << "     flow " << flow_id << " finished" << endl;

          // Met à jour le CCT
          coflow_[k].updateCCT(t);

          // Libère les ports
          remove_flow(i);

          // Supprime flow_id des listes en sécurité
          it = running.erase(it);       // efface et avance
          flow_list.remove(flow_id);    // ok : pas en cours d'itération
          // it = flow_list.erase(it); 
      } else {
          ++it; // avance normalement
      }
  }

  
  
}





//----------------------------------------------------------------//
// Public Method:  selector                              //
// Cette methode est appellee par la simulation pour selectionner //
// les coflows à l'intervalle D et pouvant s'executer dans        //
// l'intervalle Alpha*D à partir de la methode MUWP               //
//              (Minimum Unscheduled weight Problem)              //
//----------------------------------------------------------------//

void Simulator::selector(double t,int alpha,double mu_max,int deadline, list<int> & released,list<int> & selected, bool RealFlowSizes) { // drop released
  Network               instance;
  list<int>             released_flows;
  list<int>             selected_coflows;
  int                   n, k, i;
  pair<int,int>         x;
  int                   next_flow = 0;
  
  if ( released.empty() )
    return;
  
  // Construire la liste des flots à partir des coflows "released"
  for (int coflow_id : released) {
      int nb_flows = coflow_[coflow_id].getNbFlow();
      for (int i = 0; i < nb_flows; ++i) {
          int flow_id = coflow_[coflow_id].getFlowId(i);
          released_flows.push_back(flow_id);
          // cerr <<"released flow_id " <<flow_id << endl;
      }
  }

  // cerr << " released size: " << released.size() << endl;
  //Creation de l'instance
  list<int> cp_running ; //= NULL;
  clone(instance, released_flows, cp_running,t);


  //  MUWP
  instance.selection(alpha, mu_max,deadline,selected_coflows, RealFlowSizes); 

  list<int>::iterator  it = selected_coflows.begin();
  int flowId;
  for ( ; it != selected_coflows.end(); it++) {
    // cerr << "           selected coflow n°: " << *it << endl;
    released.remove(*it); // drop from released the selected coflows
    for(int j=0; j< coflow_[*it].getNbFlow(); j++){
      flowId = coflow_[*it].getFlowId(j);
      // cerr << "\tSelected Flow id: " << flowId << "size " << coflow_[*it].getFlowSize(j)<< endl;
      selected.push_back(flowId);
    }    
  }
  // cerr <<"  Released size: " <<released.size() << endl;
}


//----------------------------------------------------------------//
// Public Method:  update_priorities                              //
// Cette methode est appellee par la simulation lorsque un        //
// evenement de type UPDATE se produit. Elle permet de mettre a   //
// jour la priorite des coflows presents dans la fabrique en      //
// resolvant un probleme d'optimisation.                          //
//----------------------------------------------------------------//

void Simulator::update_priorities(double t,double deadline,list<int> & selected , list<int> & running, Algorithm alg, bool type) {
  Network        first_instance;
  Network        instance;
  double         nb_admitted = 0.0;
  int            n, k, i;
  pair<int,int>  x;
  int            next_flow = 0;
  
  if ( selected.empty() )
    return;

  clone(instance, selected, running,t); // new instance
  // calcul des priorites
  instance.schedule(nb_admitted,alg, type); // ----------------Ordonancement

  // ----------Checking 
  // debugEvent_ = true;
  if ( debugEvent_ )
    instance.printSigma();
  // debugEvent_ = false;

  //mise a jour des priorites
  list<int>::iterator  it = selected.begin();
  for ( ; it != running.end(); it++) {
    if ( it == selected.end() )
      it = running.begin();
    x=flowIdMap_[*it];
    k = x.first;
    i = x.second;
    n = instance.getFlowPriority(*it);
    coflow_[k].setFlowPriority(i,n);
  }

}


//----------------------------------------------------------------//
// Public Method:   choose_next_flow                              //
// Cette methode, parmi les flots pending, determine celui qui    //
// peut commencer son execution et a la plus grande priorite.     //
//----------------------------------------------------------------//

int Simulator::choose_next_flow(list<int> & pending) {
  int                 m, n, p;
  int                 p_min=1000000;
  int                 next_flow = -1;
  list<int>::iterator it = pending.begin();

    for ( ; it != pending.end(); it++) {
    m = flowIdMap_[*it].first;   //coflow correspondant a ce flot
    n = flowIdMap_[*it].second;  //position dans le tableau des flots du coflow
    p = coflow_[m].getFlowPriority(n);
    // if (is_flow_ready(*it)) 
    //   cerr << " flow " << *it<< " is ready "<< endl;
    // else cerr << " flow " << *it<< " not ready "<< endl;
    if ( is_flow_ready(*it) && (p < p_min) ) {
      p_min = p;
      next_flow = *it;
    }
  }
  
  return next_flow;
}


//----------------------------------------------------------------//
// Public Method:    addToRunning                                 //
// Cette methode est utilisee pour ajouter le flow dans la liste //
// running                                                        //
//----------------------------------------------------------------//

void Simulator::addToRunning(double t, int flow_id, list<int> & selected, list<int> & running) {
  int    k = flowIdMap_[flow_id].first;
  int    i = flowIdMap_[flow_id].second;  

  // on passe le flow de selected a running
  selected.remove(flow_id);
  coflow_[k].setFlowStatus(i,Flow::RUNNING);

  
  coflow_[k].updateCCT(t); //adding start time to the cct of each flow at the moment it will run --To check
  coflow_[k].setFlowExpectedCT(i,t); //adding start time to the expected cct
  // cerr << "     flow " << flow_id<< " will run at " << t << " on link " << coflow_[k].getLink(i,0) << " ->" <<coflow_[k].getLink(i,1)<< endl;
  running.push_back(flow_id);

}
//----------------------------------------------------------------//
// Public Method:    run_flows                             //
// Cette methode est utilisee dans la simulation pour mettre à   //
// jour l'utilisation des liens des differents flows en cours de //
// d'execution                                                   //
//---------------------------------------------------------------//

void Simulator::run_flows(double t,int loop, list<int> & selected,list<int> & running,list<int> & flow_list) { 
  list<int>::iterator  it = running.begin();
  int    k,i, j, l;
  int    flowId;
  int    length ;
  double min_capa = INFTY;
  double c;

  // cerr << "   loop: " << loop <<endl; //" ;runnig size: " << running.size() << endl;
  for ( ; it != running.end(); it++) {
    flowId = *it;
    k = flowIdMap_[flowId].first;
    i = flowIdMap_[flowId].second;
    // cerr << " flow "<<flowId<< endl;
    if (coflow_[k].getFlowStatus(i) == Flow::DONE) continue;
    length =  coflow_[k].getLength(i); //longueur du chemin utilisé par le flow

  //   // Obtenir la capacité minimale sur les deux ports ou mettre en mode preemptif
  
    for (j=0; j<length; j++) {
      l = coflow_[k].getLink(i,j);
      l = linkIdMap_[l];
      if ( link_[l].getStatus() == Link::BUSY && link_[l].getFlow()!= flowId ){
          // debugEvent_ = true;
          preemptFlowOnLink_online(t,l,selected, running);
          // debugEvent_ = false;
          // continue;
      }
      link_[l].acquire(flowId);
      c = link_[l].getCapa();
      if ( c < min_capa )
        min_capa = c;
    }

    // on update le flow residual size
    double res_size = coflow_[k].getFlowResidualSize(i,preemption_) - min_capa;
    // cerr << "           -flow "<< flowId<<" size: " << coflow_[k].getFlowResidualSize(i,preemption_) << " -> " << res_size << endl;

    if (res_size > 0.0){
      coflow_[k].setFlowResidualSize(i,res_size,preemption_);
      coflow_[k].AddToFlowExpectedCT(i,min_capa);
    }
    else { // flow finit son execution

      //---- Flow departure 

      coflow_[k].AddToFlowExpectedCT(i,coflow_[k].getFlowResidualSize(i,preemption_));
      coflow_[k].setFlowStatus(i,Flow::DONE);
      // cerr << "         Departure: Flow " << flowId << " fini avec cct: " << coflow_[k].getFlowExpectedCT(i) << endl << endl;
      
      // Met à jour le CCT
      coflow_[k].updateCCT(coflow_[k].getFlowExpectedCT(i)); 
        
      // Liberation des ports
      remove_flow(flowId);

      it = running.erase(it); // supprime de running et avance l'iterateur
      flow_list.remove(flowId);
      
      // cerr << "start " << t << endl;
      // cerr << "flow " << flowId <<" size=" << coflow_[k].getFlowSize(i) << endl;
      // cerr <<"CCT COFLOW " << k << " is " << coflow_[k].getCCT() << endl << endl;
    }
  }

}

//----------------------------------------------------------------//
// Public Method:     allocation                                  //
// Simulation combinant allcotaion Greedy et RR pour online       //
//----------------------------------------------------------------//

void Simulator::allocation(int start_epoch,int alpha,double mu_max,list<int> & selected,list<int> & flow_list) {
  list<int>              runningFlows;
  list<int>              finishedFlows;
  map<int, double>       rr_rate;
  map<int, double>       greedy_rate;
  map<int, double>       rate;
  map<int,double>        residual_size;
  map<int, double>       cct;
  set<int>               priorities;
  set<int>::iterator     prio_it;
  map<int, list<int> >   priority_map;
  list<int>::iterator    it;
  bool                 * busy = new bool[nbLinks_];
  int                    i, j, k, f, p;
  double                 t, global_time;

  //initialisation
  
  global_time = ceil(alpha *mu_max * start_epoch); // all are integer 
  int loop = global_time ;

  auto it_s = selected.begin();
  for(; it_s != selected.end(); it_s++){
    f = *it_s ;
    k = flowIdMap_[f].first;
    i= flowIdMap_[f].second;
    coflow_[k].setCCT(0.0);
    // f = coflow_[k].getFlowId(i);
    p = coflow_[k].getFlowPriority(i);
    runningFlows.push_back(f);
    residual_size[f] = coflow_[k].getFlowSize(i);
    priorities.insert(p);

  }

  for (prio_it = priorities.begin(); prio_it != priorities.end(); prio_it++)
    priority_map[*prio_it] = list<int>();
  for (it=runningFlows.begin(); it != runningFlows.end(); it++) {
    f = *it;
    k = flowIdMap_[f].first;
    i = flowIdMap_[f].second; 
    p = coflow_[k].getFlowPriority(i);
    priority_map[p].push_back(f);
  }

  //boucle principale
  while ( runningFlows.empty() == false ) {

    // compute Greedy allocation
    greedy_allocation(runningFlows, greedy_rate, busy, priorities, priority_map);
    
    // determine next departure time
    t = INFTY;
    for (auto it=runningFlows.begin(); it != runningFlows.end(); it++) {
      f = *it;
     
      if ( lambda_ < 1.0e-4 )
        rate[f] = rr_rate[f];
      if ( lambda_ > 0.9999 )
        rate[f] = greedy_rate[f];
      else
        rate[f] = lambda_*greedy_rate[f] + (1.0-lambda_)*rr_rate[f];

      if ( rate[f] > 0.0 ) {
        k = flowIdMap_[f].first;
        i = flowIdMap_[f].second; 
        p = coflow_[k].getFlowPriority(i);
        	// cerr << "Rate for flow " << i << " of coflow " << k << " (id=" << f << ", size=" << coflow_[k].getFlowSize(i) << ")" << " = " << rate[f] << endl;
        // cerr << "flow " << f << " runs on " <<coflow_[k].getFlowSource(i) << "  and " << coflow_[k].getFlowDestination(i) << endl;
        cct[f] = residual_size[f]/rate[f];
        if ( cct[f] < t ) 
          t = cct[f];
        }
    }
    global_time += t;
    loop += ceil(t);
    
    // determine the next flows to depart and update residual sizes for the others
    for (auto it=runningFlows.begin(); it != runningFlows.end(); it++) {
      f = *it;
      k = flowIdMap_[f].first;
      i = flowIdMap_[f].second; 
      p = coflow_[k].getFlowPriority(i);
      if ( fabs(cct[f]-t) < 1e-8 ) {
	      finishedFlows.push_back(f);
		    // cerr << "Flow " << i << " of coflow " << k << " leaves the system at time " << global_time << endl;
      }
      else {
	      residual_size[f] -= rate[f]*t;
        // cerr << "flow " << f<<" res size " <<residual_size[f] << endl;
	// if ( rate[f] > 0.0 )
	//   cerr << "Flow " << i << " of coflow " << k << " : residual size=" << residual_size[f] << endl;
      }
    }
    // cerr <<"start " << start_epoch << endl;
    //remove finished flows
    for (auto it=finishedFlows.begin(); it != finishedFlows.end(); it++) {
      f = *it;
      k = flowIdMap_[f].first;
      i = flowIdMap_[f].second; 
      p = coflow_[k].getFlowPriority(i);
      // cerr << "Flow " << i << " of coflow " << k << " (id=" << f << ", size=" << coflow_[k].getFlowSize(i) << ")" << " leaves the system at time " << global_time << endl;
      
      runningFlows.remove(f);
      priority_map[p].remove(f);
      residual_size.erase(f);
      rr_rate.erase(f);
      greedy_rate.erase(f);
      rate.erase(f);
      cct.erase(f);
      coflow_[k].updateCCT(global_time);
      // cerr <<"        Time elapse : " << loop << endl;
      // cerr << "flow " << f<<" size=" << coflow_[k].getFlowSize(i) << endl;
      // cerr <<"\t\tCCT COFLOW " << k << " is " << coflow_[k].getCCT() << " after flow " << f << " of size " <<coflow_[k].getFlowSize(i)<< " finished"<< endl << endl;
      selected.remove(f); //remove from selected list
      flow_list.remove(f); //remove from flow_list
    }
    finishedFlows.clear();

  } // fin while

  delete [] busy;
}
//----------------------------------------------------------------//
// Public Method:  print_link_table                               //
// Cette methode affiche les flots s'executant sur chacun des     //
// liens                                                          //
//----------------------------------------------------------------//

void Simulator::print_link_table(double t) {
  int l;
  int n = MAX(5,nbLinks_);
  
  cerr << "t=" << std::setprecision(3) << t << "\t| ";
  for (l=0; l<n; l++) {
    if ( link_[l].getStatus() == Link::BUSY ) {
      int    f = link_[l].getFlow();
      int    k = flowIdMap_[f].first;
      int    i = flowIdMap_[f].second; 

      cerr << "F" << coflow_[k].getFlowId(i) << " (t=" << std::setprecision(3) << coflow_[k].getFlowExpectedCT(i) << ")\t| ";
    }
    else
      cerr << "------------\t| ";
  }
  cerr << endl;
}

//----------------------------------------------------------------//
// Public Method:     Coflow arrival time                        //
// Adding a method to get coflows arring over time.                //
//----------------------------------------------------------------//
void Simulator::addArrivalTime(){
  int gen = 5;
  for (int k=0; k<nbCoflows_; k++) {
    int t = rand()%gen;
    coflow_[k].setStartTime(t);
  }
  cerr << "Coflows arriving time" << endl;
  for (int k=0; k<nbCoflows_; k++) {
    cerr << coflow_[k].getStartTime() << endl;
  }
}


//----------------------------------------------------------------//
// Public Method:     simulation                                  //
// Simulation evenementielle au niveau flot du systeme.           //
// L'echeancier est initialise avec les evenements d'arrivee de   //
// tous les flots. La boucle principale recupere a chaque fois    //
// le prochain evenement, se positionne a la date de cet evene-   //
// ment, puis appelle new_flow_arrival si c'est une arrivee, et   //
// new_flow_departure si c'est un depart.                         //
//----------------------------------------------------------------//

void Simulator::simulation(Algorithm alg, bool online_priorities, double slot_size, bool type ) { // type is if real flow size
  EventList  evtList;
  Event      e;
  list<int>  pending;
  list<int>  selecting;
  list<int>  running;
  double     d;
  double     max_deadline = 0.0;
  double     t = 0.0;
  double     t_prev = 0.0;
  int        k, i, j, n;

  // on indique l'algorithme utilise
  algo_ = alg;
  // debugEvent_ = true ;
  
  addArrivalTime();
  

  // on initialise l'echeancier avec les evenements d'arrivee et d'update
  for (k=0; k<nbCoflows_; k++) {
    t = coflow_[k].getStartTime();
    d = coflow_[k].getEndTime();
    cerr <<"Arrive time "<< t << endl;
    if ( d > max_deadline )
      max_deadline = d;
    for (i=0; i<coflow_[k].getNbFlow(); i++) {
      j = coflow_[k].getFlowId(i);
      evtList.addEvent( Event(t, 0.0, Event::ARRIVAL, j) );
    }
    if ( online_priorities && (slot_size==0.0) )
      evtList.addEvent( Event(t, 0.0, Event::UPDATE) );
  }
  // Avoir pour le slot_size
  // if ( online_priorities && (slot_size>0.0) ) {
  //   for (t=0.0; t<max_deadline; t += slot_size)
  //     {cerr << t << endl;
  //     evtList.addEvent( Event(t, 0.0, Event::UPDATE) );}
  // }
  
  // int to, k = 0, 0;
  // for (int pen : pending)  cerr << "pending "<<pen << "\n";
  //affichage
  if ( debugEvent_ ) {
    cerr << "\n\nTime\t| ";
    for (int l=0; l<nbLinks_; l++)
      cerr << "LINK " << link_[l].getId() << "\t| ";
    cerr << endl;
  }
  
  //boucle principale de simulation
  t = 0.0;
  while ( evtList.empty() == false ) {
    
    // on recupere le prochain evenement
    e = evtList.nextEvent();

    // on se place a la date de l'evenement
    t_prev = t;
    t = e.getDate();

    // on met a jour les tailles residuelles des flots
    update_residual_sizes(running, t-t_prev);
    //    remove_outdated_coflows(pending, t);
    
    // on traite l'evenement
    switch( e.getType() ) {
      
    case Event::ARRIVAL :
      if ( debugEvent_ )
       	cerr << "ARRIVAL EVENT: flow " << e.getFlow() << " at time " << t << endl;
      new_flow_arrival(t, e.getFlow(), pending, running, evtList);
      break;
      
    case Event::DEPARTURE :
      if ( debugEvent_ )
       	cerr << "DEPARTURE EVENT: flow " << e.getFlow() << " at time " << t << endl;
      new_flow_departure(t, e.getFlow(), pending, running, evtList);
      break;      

    case Event::UPDATE :
      if ( debugEvent_ )
	cerr << "UPDATE EVENT at time " << t << endl;
      update_priorities(t,t, pending, running, alg,type);
      break;      
      
      
    default :
      cerr << "Unknown event " << endl;
      exit(-1);
    }
    
    if ( debugEvent_ ) {
      if ( e.getType() != Event::UPDATE )
     	print_link_table(t);
    }
  }
}

//----------------------------------------------------------------//
// Public Method:     online_simulation                           //
//----------------------------------------------------------------//
void Simulator::online_simulation(Algorithm alg, bool online_priorities, double slot_size, bool type ) { // type is if real flow size
  EventList  evtList;
  list<int>  flow_list;
  list<int>  released;
  list<int>  selected;
  list<int>  running;
  double     t = 0.0;
  int        k,p, i, j; 

  // addArrivalTime();  // adding arrival time -from a function here  

  p = 1;
  t = 0.0;  

  int epoch_start = 0;
  int epoch_end = 1 ;

  for (k=0; k<nbCoflows_; k++) { 
    for (i = 0; i < coflow_[k].getNbFlow(); i++) {
      int j = coflow_[k].getFlowId(i);
      flow_list.push_back(j);
    }
  }

  while (!flow_list.empty()) {
    //1. Ajouter les coflows arrivés dans l'epoch courant  
    for (k = 0; k < nbCoflows_; k++) {
      t = coflow_[k].getStartTime();        
      if ( t>= epoch_start && t < epoch_end) {
        released.push_back(k);      
      }
    }

    if (online_priorities){
      // 2. Selectionner les coflows à executer dans l'intervalle D à partir de MUWP
      int deadline = epoch_end - epoch_start;
      double mu_max = getMU_MAX(type);
      int alpha = 4;
      selector(epoch_start,alpha, mu_max,deadline, released,selected,type); // epoch_start because all cofloww allready released , will be process at this time epoch
      
      // 3. Mettre à jour les priorités sur la selection 
      update_priorities(epoch_start,deadline,selected, running, alg, type); // epoch_start because all cofloww allready released , will be process at this time epoch

     
      }
    

    // 4. Exécuter les flots selectionnés dans l'intervalle [epoch_start, epoch_end] et enlever ceux qui finnissent
    for (int loop = epoch_start ; loop < epoch_end ; loop ++){ 
      if(selected.size() == 0 && running.size() == 0) break;  
      int next = choose_next_flow(selected);

      if (next != -1){
        addToRunning(epoch_start, next, selected, running);
      }
      run_flows(epoch_start,loop, selected, running,flow_list);
    }
    
    // 5. Avancer le epoch
    p +=1;
    epoch_start = epoch_end;
    epoch_end = pow(2 ,(p-1));
   
  }

}

//----------------------------------------------------------------//
// Public Method:     online_simulation with greedy               //
//----------------------------------------------------------------//
void Simulator::online_simulation_greedy(Algorithm alg, double slot_size, bool type ) { // type is if real flow size
  EventList  evtList;
  list<int>  flow_list;
  list<int>  released;
  list<int>  selected;
  list<int>  running;
  double     t = 0.0;
  int        k,p, i, j; 

  // addArrivalTime();  // adding arrival time -from a function here  

  p = 1;
  t = 0.0;  

  int epoch_start = 0;
  int epoch_end = 1 ;

  for (k=0; k<nbCoflows_; k++) { 
    for (i = 0; i < coflow_[k].getNbFlow(); i++) {
      int j = coflow_[k].getFlowId(i);
      flow_list.push_back(j);
    }
  }

  while (!flow_list.empty()) {
    // cerr <<"New epoch [" << epoch_start << ", " << epoch_end << " ]" << endl; 
    //1. Ajouter les coflows arrivés avant l'epoch start  

    for (k = 0; k < nbCoflows_; k++) {
      t = coflow_[k].getStartTime();        
      if ( t>= int(epoch_start/2) && t <= epoch_start) {
        released.push_back(k);  
        // cerr <<"Coflow " << k << "released at " << t << endl;    
      }
    }

    
      // 2. Selectionner les coflows à partir de epoch_start à executer dans l'intervalle alpha*D( alpha*mu_max*D) à partir de MUWP
      int deadline = epoch_end - epoch_start;
      double mu_max;
      if (!type)
        mu_max = getMu_max();
      else
        mu_max = 1.0;

      int alpha = 4;
      // if (epoch_start == 256) exit(-1);
      // cerr << "2. Selection dans [" << mu_max*epoch_start << " , "<< mu_max*epoch_end << " ] " << endl;
      // cerr << ". Execution dans [" << alpha *mu_max*epoch_start << " , "<< alpha *mu_max*epoch_end << " ] " << endl;
      selector(epoch_start,alpha, mu_max,deadline, released,selected,type); // epoch_start because all cofloww allready released , will be process at this time epoch
      

      // 3. Mettre à jour les priorités sur la selection 
      update_priorities(epoch_start,deadline,selected, running, alg, type); // epoch_start because all cofloww allready released , will be process at this time epoch
   

    // 4. Exécuter les flots selectionnés dans l'intervalle [alpha *epoch_start, alpha * epoch_end]
    // cerr << "4. Execution dans [" << alpha *mu_max*epoch_start << " , "<< alpha *mu_max*epoch_end << " ] " << " selected flows size: " << selected.size() << endl;
    
    if (!selected.empty())
      allocation(epoch_start,alpha, mu_max, selected, flow_list);
    
    // 5. Avancer le epoch
    p +=1;
    epoch_start = epoch_end;
    epoch_end = pow(2 ,(p-1));
    // cerr <<"___________________________________________"<<endl;
   
  }

}



//----------------------------------------------------------------//
// Public Method:     offline_simulation                          //
// Simulation combinant allcotaion Greedy et RR                   //
//----------------------------------------------------------------//

void Simulator::offline_simulation(Network::Algorithm algo) {
  list<int>              runningFlows;
  list<int>              finishedFlows;
  map<int, double>       rr_rate;
  map<int, double>       greedy_rate;
  map<int, double>       rate;
  map<int,double>        residual_size;
  map<int, double>       cct;
  set<int>               priorities;
  set<int>::iterator     prio_it;
  map<int, list<int> >   priority_map;
  list<int>::iterator    it;
  bool                 * busy = new bool[nbLinks_];
  int                    i, j, k, f, p;
  double                 t, global_time, ct;

  //initialisation
  //  cerr << "Offline simulation of " << nbFlows_ << " flows..." << endl;
  global_time = 0.0;
  for (k=0; k<nbCoflows_; k++) {
    // coflow_[k].setCCT(0.0); //to check 
    coflow_[k].setCCT(coflow_[k].getStartTime()); //to check 
    for (i=0; i<coflow_[k].getNbFlow(); i++) {
      f = coflow_[k].getFlowId(i);
      p = coflow_[k].getFlowPriority(i);
      runningFlows.push_back(f);
      residual_size[f] = coflow_[k].getFlowSize(i);
      priorities.insert(p);
    }
  }

  for (prio_it = priorities.begin(); prio_it != priorities.end(); prio_it++)
    priority_map[*prio_it] = list<int>();
  for (it=runningFlows.begin(); it != runningFlows.end(); it++) {
    f = *it;
    k = flowIdMap_[f].first;
    i = flowIdMap_[f].second; 
    p = coflow_[k].getFlowPriority(i);
    priority_map[p].push_back(f);
  }

  //boucle principale
  while ( runningFlows.empty() == false ) {

    // compute Greedy allocation
    if ( lambda_<0.9999 )
      round_robin_allocation(runningFlows, rr_rate);

    if ( lambda_ > 1.0e-4 )
      greedy_allocation(runningFlows, greedy_rate, busy, priorities, priority_map);
    
    // determine next departure time
    t = INFTY;
    for (auto it=runningFlows.begin(); it != runningFlows.end(); it++) {
      f = *it;
     
      if ( lambda_ < 1.0e-4 )
	rate[f] = rr_rate[f];
      else if ( lambda_ > 0.9999 )
	rate[f] = greedy_rate[f];
      else
	rate[f] = lambda_*greedy_rate[f] + (1.0-lambda_)*rr_rate[f];
      if ( rate[f] > 0.0 ) {
	k = flowIdMap_[f].first;
	i = flowIdMap_[f].second; 
	p = coflow_[k].getFlowPriority(i);
	//	cerr << "Rate for flow " << i << " of coflow " << k << " (id=" << f << ", size=" << coflow_[k].getFlowSize(i) << ")" << " = " << rate[f] << endl;
	cct[f] = residual_size[f]/rate[f];
	if ( cct[f] < t ) 
	  t = cct[f];
      }
    }
    global_time += t;
    
    // determine the next flows to depart and update residual sizes for the others
    for (auto it=runningFlows.begin(); it != runningFlows.end(); it++) {
      f = *it;
      k = flowIdMap_[f].first;
      i = flowIdMap_[f].second; 
      p = coflow_[k].getFlowPriority(i);
      if ( fabs(cct[f]-t) < 1e-8 ) {
	finishedFlows.push_back(f);
	//	cerr << "Flow " << i << " of coflow " << k << " leaves the system at time " << global_time << endl;
      }
      else {
	residual_size[f] -= rate[f]*t;
	// if ( rate[f] > 0.0 )
	//   cerr << "Flow " << i << " of coflow " << k << " : residual size=" << residual_size[f] << endl;
      }
    }

    //remove finished flows
    for (auto it=finishedFlows.begin(); it != finishedFlows.end(); it++) {
      f = *it;
      k = flowIdMap_[f].first;
      i = flowIdMap_[f].second; 
      p = coflow_[k].getFlowPriority(i);
      // cerr << "Flow " << i << " of coflow " << k << " (id=" << f << ", size=" << coflow_[k].getFlowSize(i) << ")" << " leaves the system at time " << global_time << endl ;
      
      runningFlows.remove(f);
      priority_map[p].remove(f);
      residual_size.erase(f);
      rr_rate.erase(f);
      greedy_rate.erase(f);
      rate.erase(f);
      cct.erase(f);
      // coflow_[k].AddToFlowExpectedCT(i,global_time);
      if (algo == Network::SINCRONIARelease){
        ct = coflow_[k].getStartTime() + global_time;
        // cerr << "coflow " << k << " cct= " << ct << endl<< endl;
      }
      else {
        ct = global_time;
        // cerr << "coflow " << k << " cct= " << ct << endl<< endl;
      }
      coflow_[k].updateCCT(ct); // start_time + global_time
    }
    finishedFlows.clear();

  } // fin while

  delete [] busy;
}




//----------------------------------------------------------------//
// Public Method:  round_robin_allocation                         //
// Calcule l'allocation RR de chaque flot running                 //
//----------------------------------------------------------------//

void Simulator::round_robin_allocation(list<int> & runningFlows, map<int,double> & rates) {
  int                  * nb_flows_on_link = new int[nbLinks_];
  list<int>::iterator    it;
  int                    i, j, k, f, l, length;
  double                 r, r_min;
  
  //initialisation
  for (j=0; j<nbLinks_; j++)
    nb_flows_on_link[j] = 0;

  // on compte le nombre de flots sur chaque lien
  for (it=runningFlows.begin(); it != runningFlows.end(); it++) {
    f = *it;
    k = flowIdMap_[f].first;
    i = flowIdMap_[f].second; 
    length = coflow_[k].getLength(i);
    for (j=0; j<length; j++) {
      l = coflow_[k].getLink(i,j);
      l = linkIdMap_[l];
      nb_flows_on_link[l] += 1;
    }
  }

  // on calcule le debit RR de chaque flot
  for (it=runningFlows.begin(); it != runningFlows.end(); it++) {
    f = *it;
    k = flowIdMap_[f].first;
    i = flowIdMap_[f].second; 
    length = coflow_[k].getLength(i);
    r_min = 1.0e10;
    //    cerr << "RR: Flow " << i << " of coflow " << k << " :";
    for (j=0; j<length; j++) {
      l = coflow_[k].getLink(i,j);
      l = linkIdMap_[l];
      r = link_[l].getCapa()/nb_flows_on_link[l];
      //      cerr << " rate(" << l << ")=" << link_[l].getCapa() << "/" << nb_flows_on_link[l] << "=" << r << " | ";
      if ( r < r_min )
	r_min = r;
    }
    //    cerr << " => rr rate = " << r_min << endl;
    rates[f] = r_min;
  }
  
  delete [] nb_flows_on_link;
}




//----------------------------------------------------------------//
// Public Method:  greedy_allocation                              //
// Calcule l'allocation greedy de chaque flot running             //
//----------------------------------------------------------------//

void Simulator::greedy_allocation(list<int> & runningFlows, map<int,double> & rates, bool *busy,
				  set<int> & priorities, map<int, list<int> > &priority_map) {
  map<int, list<int> >::iterator map_it;
  set<int>::iterator             prio_it;
  list<int>::iterator            it;
  int                            i, j, k, f, l, p, length;
  int                            nb_busy_link = 0;
  double                         r, r_min;

  //initialisation
  for (j=0; j<nbLinks_; j++)
    busy[j] = false;

  // on determine les priorites encore actives
  for (map_it = priority_map.begin(); map_it != priority_map.end(); map_it++) {
    p = map_it->first;
    if ( map_it->second.empty() )
      priorities.erase(p);
  }
  
  //Allocation de debit greedy aux flots par priorite croissante
  for (prio_it = priorities.begin(); prio_it != priorities.end(); prio_it++) {

    // parcourt des flots en sautant ceux qui n'ont pas la bonne priorite
    for (it=priority_map[*prio_it].begin(); it != priority_map[*prio_it].end(); it++) {
      f = *it;
      k = flowIdMap_[f].first;
      i = flowIdMap_[f].second; 
      p = coflow_[k].getFlowPriority(i);
      if ( p != *prio_it ) continue;
      length = coflow_[k].getLength(i);
      r_min = 1.0e10;
      for (j=0; j<length; j++) {
      l = coflow_[k].getLink(i,j);
      l = linkIdMap_[l];
      if ( busy[l] )
        r = 0.0;
      else
        r = link_[l].getCapa();
      if ( r < r_min )
        r_min = r;
          }
      rates[f] = r_min;
      // if ( r_min > 0.0 ) {
      //  	int src = coflow_[k].getFlowSource(i);
      //  	int dst = coflow_[k].getFlowDestination(i);
      //  	double size = coflow_[k].getFlowSize(i);
	
      // 	cerr << "GREEDY: Flow " << i << " of coflow " << k  << " => src=" << src << ", dst=" << dst << ", rate=" << r_min << " (prio=" << p << ", size=" << size << ")" << endl;
      // }
      if ( r_min > 0.0 ) 
      for (j=0; j<length; j++) {
        l = coflow_[k].getLink(i,j);
        l = linkIdMap_[l];
        busy[l] = true;
        nb_busy_link++;
      }
    } // fin boucle sur les flots
  } // fin boucle sur les priorites

}
