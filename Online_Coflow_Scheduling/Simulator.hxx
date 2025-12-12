#ifndef __Simulator_hxx__
#define __Simulator_hxx__


//****************************************************************//
//*                                                              *//
//*------------------- Includes SECTION -------------------------*//
//*                                                              *//
//****************************************************************//

#include "Network.hxx"
#include "EventList.hxx"


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
// Class :    Simulator                                           //
//                                                                //
//                                                                //
// Description : classe derivant de la classe Network.            //
// Elle ajoute les methodes de simulation pour l'ordonnancement   //
// online.                                                        //
//                                                                //
//----------------------------------------------------------------//


class Simulator: public Network
{
private:
  bool                     preemption_;     // true if preemption is allowed (greedy transport layer)
  bool                     debugEvent_;    // set to true for displaying events
  Algorithm                algo_;          // algorithme d'ordonnancement des coflows
  double                   lambda_;        // ponderation greedy (1.0) et RR (0.0)
  
public :

  // constructeurs

  Simulator( const char * inputFileName, bool debug, bool preempt, bool debugEvent, int version=2, int coeffWeight=1 ) :
    Network(inputFileName, debug, version, coeffWeight),
    preemption_(preempt),
    debugEvent_(debugEvent)
  {}
  

  // destructeur
  ~Simulator()
  {}


  // accesseurs aux attributs

  void     setPreemption(bool b) { preemption_=b; }

  void     setDebugEvent(bool b) { debugEvent_ = b; }

  void     setLambda(double l) { lambda_ = l; }
  
  
  // methodes de lecture et d'ecriture

  void      set_of_active_coflows(list<int> & pending, list<int> & running, set<int> & set_coflow_id);

    
  // methodes de calcul

  void     clone(Network & copy, list<int> & pending, list<int> & running, double t=0.0);

  void     update_residual_sizes(list<int> & running, double Delta_t);
  
  void     preemptFlowOnLink(double t, int l, list<int> & pending, list<int> & running, EventList & evtList);
  
  void     preemptFlowOnLink_online(double t, int l, list<int> & pending, list<int> & running);

  int      getFlowPriorityOnlink(int l);

  bool     is_link_busy(int l, int k, int i);

  int      choose_next_flow(list<int> & pending);

  void     addToRunning(double t, int flow_id, list<int> & pending, list<int> & running);
  
  bool     is_flow_ready(int flow_id);

  void     remove_flow(int flow_id);
  
  void     run_flow(double t, int flow_id, list<int> & pending, list<int> & running, EventList & evtList);
  
  void     run_flows(double t,int loop, list<int> & selected,list<int> & running,list<int> & flow_list);
  
  void     new_flow_arrival(double t, int flow_id, list<int> & pending, list<int> & running, EventList & evtList);

  void     new_flow_departure(double t, int flow_id, list<int> & pending, list<int> & running, EventList & evtList);
  
  void     flow_departure(double t, list<int> & running,list<int> & flow_list); //online 

  void     selector(double t,int alpha,double mu_max,int deadline, list<int> & pending,list<int> & selected, bool RealFlowSizes) ; //online 

  void     update_priorities(double t,double deadline,list<int> & selected , list<int> & running, Algorithm alg, bool type);
  
  void     print_link_table(double t);
  
  void     simulation(Algorithm alg=Network::DCOFLOW, bool online_priorities=true, double slot_size=0.0, bool type="real");

  void     remove_outdated_coflows(list<int> & pending, double t);

  void     addArrivalTime();

  void     online_simulation(Algorithm alg, bool online_priorities, double slot_size, bool type );

  void     online_simulation_greedy(Algorithm alg, double slot_size, bool type );

  void     online_simulation_greedy_anticipate(Algorithm alg, double slot_size, bool type );
  // methodes pour la simulation offline
  //  void     greedy_allocation(list<int> & runningFlows, map<int,double> & rates, bool *busy);

  void     round_robin_allocation(list<int> & runningFlows, map<int,double> & rates);
  
  void     greedy_allocation(list<int> & runningFlows, map<int,double> & rates, bool *busy,
			     set<int> & priorities, map<int, list<int> > &priority_map);
  
  void     offline_simulation(Network::Algorithm algo);

  void     allocation(int start_epoch,int alpha,double mu_max,list<int> & selected,list<int> & flow_list);

  void     allocation_Anticipate(int start_epoch,int alpha,double mu_max,list<int> & selected,list<int> & flow_list, int & global_time);

};


// methodes d'affichage 

ostream & operator<< (ostream &, Network const &);


#endif
