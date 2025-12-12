//****************************************************************//
//*                                                              *//
//*------------------- Includes SECTION -------------------------*//
//*                                                              *//
//***************************************************************//

//#include <stdlib.h>
//#include <stdio.h>
#include <iomanip>
#include <math.h>
#include <cmath>
#include <set>
#include <iterator>
#include <algorithm>
#include <random>

#include "ClpSimplex.hpp"

#include <cstdio>
#include <iostream>
#include <unistd.h>

#include "Network.hxx"
#include "common.hxx"
#include "Job.hxx"





//****************************************************************//
//*                                                              *//
//*------------- Private Variables SECTION ----------------------*//
//*                                                              *//
//****************************************************************//

#define OPTIMAL_SOLUTION      0
#define CENTRALIZED_SOLUTION  1
#define DISTRIBUTED_SOLUTION  2

#define BIG_M  10000000
#define BIG_M2 50000
#define EPS    1.0e-6
#define INFTY  1.0e20

#define SCALING_FACTOR   1.0

#define GRB_TIME_LIMIT   900.0


random_device               rd;
mt19937                     gen(rd());



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
// Private Function: gcd                                          //
//----------------------------------------------------------------//

int gcd(int a, int b) {
  while (b != 0)  {
    int t = b;
    b = a % b;
    a = t;
  }
  return a;
}

//----------------------------------------------------------------//
// Private Function: alpha_fair                                   //
//----------------------------------------------------------------//

double alpha_fair(double x, int alpha) {

  if ( alpha == 1 )
    return log(x);
  else
    return pow(x,1-alpha)/(1-alpha);
}



//----------------------------------------------------------------//
// Private Function:   operator<<                                 //
//----------------------------------------------------------------//

ostream & operator<< (ostream &stream, Network const &g)
{

  return stream;
}



//----------------------------------------------------------------//
// Private function:   find_argmax_proc_time                      //
//----------------------------------------------------------------//

void find_argmax_proc_time(set<Job> my_jobs, int n, Job & rejected_job) {
  set<Job>::iterator  it;
  double              p;
  double              max_p = 0.0;

  it = my_jobs.begin();
  for ( ; it != my_jobs.end(); it++) {
    p = it->processingTime();
    if ( p > max_p ) {
      max_p = it->processingTime();
      rejected_job = *it;
    }
    if ( it->id() == n )
      break;
  }
}


//****************************************************************//
//*                                                              *//
//*--------------- Public Methods SECTION -----------------------*//
//*                                                              *//
//****************************************************************//

//----------------------------------------------------------------//
// Public Method:     setNumberofCoflows                          //
//----------------------------------------------------------------//

void  Network::setNumberofCoflows(int nb) {
  nbCoflows_ = nb;
  coflow_ = new Coflow[nbCoflows_];
}



//----------------------------------------------------------------//
// Public Method:     setNumberofLinks                            //
//----------------------------------------------------------------//

void  Network::setNumberofLinks(int nb) {
  nbLinks_ = nb;
  link_ = new Link[nbLinks_];
}

//----------------------------------------------------------------//
// Public Method:     setWeights                                  //
//----------------------------------------------------------------//

void  Network::setWeights(int *weight) {
  int k, c;
  
  for (k=0; k<nbCoflows_; k++) {
    c = coflow_[k].getClassId();
    if ( c == 1 )
      coflow_[k].setWeight(weight[0]);
    else
      coflow_[k].setWeight(weight[1]);	    
  }
}


//----------------------------------------------------------------//
// Public Method:     readCoflows                                 //
//----------------------------------------------------------------//

void Network::readCoflows(ifstream &inFile) {
  int               k, i, j,  n, w;
  Coflow            cf;

  for (k=0; k<nbCoflows_; k++) {
    inFile >> cf;
    w= cf.getWeight();
    cf.setClassId(w);
    if ( w>1 )
      cf.setWeight(w*coeffWeight_);
    addCoflow(k,cf.getId(), cf.getWeight(), cf.getStartTime(), cf.getEndTime(), cf.getNbFlow());
    n = cf.getId()*1000;
    j = coflow_[k].getNbFlow();
    for (i=0; i<j; i++) {
      Flow  myFlow;

      //      n = readFlow(inFile, myFlow);
      readFlow(inFile, myFlow);
      n++;
      myFlow.setId(n);
      addFlow(k,i,n,myFlow);
    }

    // affichage
    coflow_[k].scale(SCALING_FACTOR);
    if ( debug_ ) {
      cerr << endl << coflow_[k] << endl;
      for (i=0; i<coflow_[k].getNbFlow(); i++)
	coflow_[k].printFlow(i);
    }
  }
  
  if ( debug_ )
    cerr << endl;
}


//----------------------------------------------------------------//
// Public Method:     addCoflow                                   //
//----------------------------------------------------------------//

void Network::addCoflow(int pos, int id, int w, double a, double d, int nb_flows, double t) {
  double t_start = ((t>0.0)?0.0:a);
  double t_end = d-t;

  coflow_[pos].setId(id);
  coflow_[pos].setClassId(w);  
  coflow_[pos].setWeight(w);    
  coflow_[pos].setStartTime(t_start);
  coflow_[pos].setDeadline(t_end);
  coflow_[pos].setNbFlows(nb_flows);
  coflow_[pos].allocate();
  idMap_[id]=pos;
}


//----------------------------------------------------------------//
// Public Method:     addLink                                     //
//----------------------------------------------------------------//

void Network::addLink(int pos, int id, double capa) {
  link_[pos].setId(id);
  link_[pos].setCapa(capa);
  link_[pos].setReleaseDate(0.0);
  link_[pos].setCompletionTime(0.0);
  //  link_[pos].release();
  linkIdMap_[id] = pos;
  if ( debug_ )
    cerr << link_[pos] << endl;
}


//----------------------------------------------------------------//
// Public Method:      readFlow                                   //
//----------------------------------------------------------------//

int Network::readFlow(ifstream & inFile, Flow & flow) {
  int k, l;
  
  inFile >> flow;
  flow.allocate();
  for (k=0; k<flow.getLength(); k++) {
    inFile >> l;
    flow.setLink(k,l);
  }

  return flow.getId();
}


//----------------------------------------------------------------//
// Public Method:      addFlow                                    //
//----------------------------------------------------------------//

void Network::addFlow(int k, int i, int n, Flow & flow, bool dynamic) {
  coflow_[k].setFlow(i, flow, dynamic);
  coflow_[k].setFlowPath(i, flow.getPath());
  flowMap_[nbFlows_] = make_pair(k,i);
  flowIdMap_[n] = make_pair(k,i);
  nbFlows_++;
}




//----------------------------------------------------------------//
// Public Method:    compute_time_slots                           //
//----------------------------------------------------------------//

void Network::compute_time_slots() {
  set<int>     S;
  int          k, b;
  int          n, i, j;
  double       max_end_time, Delta, t;
  
  
  
  // initialisation
  for (k=0; k<nbCoflows_; k++)
    S.insert(coflow_[k].getId());

  // cerr << "In compute_time_slots -nbCoflows_: " <<nbCoflows_<< endl;
  // cerr << "In compute_time_slots -nbLinks_: " <<nbLinks_<< endl;

    //????Problem here , there is no link
  //calcul bottleneck et temps de terminaison
  b = bottleneck_link(S, max_end_time, true);
  // cerr <<"Not executed........"<< endl;
  //taille des slots
  nbSlots_ = 100;
  Delta = max_end_time/nbSlots_;
  // debug_ = true;
  if ( debug_ )
    cerr << "Compute time slots: T=" << max_end_time << " and Delta=" << Delta << endl;
  
  //create slots
  slotSize_ = new double[nbSlots_];
  slotStart_ = new double[nbSlots_];
  slotEnd_ = new double[nbSlots_];
  if ( debug_ )
    cerr << "TIME SLOTS : ";
  t = 0.0;
  for (k=0; k<nbSlots_; k++) {
    if ( debug_ )
      cerr << "[" << t << "," << t+Delta << "], ";
    slotStart_[k] = t;
    slotEnd_[k] = t+Delta;
    slotSize_[k] = Delta;
    t += Delta;
  }
  if ( debug_ )
    cerr << endl;
}


//----------------------------------------------------------------//
// Public Method:      read_Sincronia_file                        //
//----------------------------------------------------------------//

void Network::read_Sincronia_file(ifstream & inFile) {
  int    k, n, i;
  int    cf_id, cf_num_flows, cf_num_src, cf_num_dst;
  int    f_src, f_dst;
  double cf_arrival_time, f_size;
  
  //read links
  inFile >> n;
  setNumberofLinks(n);
  if ( debug_ ) {
    cerr << "Number of links is " << nbLinks_ << endl << endl;
    cerr << "LINKS" << endl << "---------------------" << endl;
  }
  for (i=0; i<nbLinks_; i++) {
    Link l(i,1.0);
    addLink(i,l);
  }
  if ( debug_ )
    cerr << endl;

  //read coflows
  inFile >> n;
  setNumberofCoflows(n);
  if ( debug_ ) {
    cerr << "Number of coflows is " << nbCoflows_ << endl << endl;
    cerr << "COFLOWS" << endl << "---------------------" << endl;
  }
  for (k=0; k<nbCoflows_; k++) {
    inFile >> cf_id
	   >> cf_arrival_time
	   >> cf_num_flows
	   >> cf_num_src
	   >> cf_num_dst;
    addCoflow(k,cf_id, 1, cf_arrival_time, 1.0e10, cf_num_flows);
    n = cf_id*1000;
    for (i=0; i<cf_num_flows; i++) {
      inFile >> f_src
	     >> f_dst
	     >> f_size;
      n++;
      Flow  myFlow(n, f_size, 2, f_size);

      myFlow.allocate();
      myFlow.setLink(0,f_src);
      myFlow.setLink(1,f_dst);
      addFlow(k,i,n,myFlow);
    }
   
    // affichage
    if ( debug_ ) {
      cerr << endl << coflow_[k] << endl;
      for (i=0; i<coflow_[k].getNbFlow(); i++)
	coflow_[k].printFlow(i);
    }
  }
  if ( debug_ )
    cerr << endl;
}





//----------------------------------------------------------------//
// Public Method:       Constructor                               //
//----------------------------------------------------------------//

Network::Network( const char * inputFileName, bool debug, int version,  bool Sincronia_format  ):
  nbFlows_(0),
  nbSlots_(0),
  slotSize_(0),
  slotStart_(0),
  slotEnd_(0),
  debug_(debug),
  version_(version),
  coeffWeight_(1.0)
{
  ifstream          inFile(inputFileName);
  int               i, n;
  Link              l;
  
  if ( !inFile ) { cerr << "Cannot open input file\n";  exit(-1);  }

  if ( Sincronia_format ) {
    read_Sincronia_file(inFile);
  }
  else {
 
    //read max time
    inFile >> maxTime_;
    if ( debug_ )
      cerr << "Number of time slots is " << maxTime_ << endl << endl;
    maxTime_ = 0.0;
  
    //read coflows
    inFile >> n;
    setNumberofCoflows(n);
    if ( debug_ ) {
      cerr << "Number of coflows is " << nbCoflows_ << endl << endl;
      cerr << "COFLOWS" << endl << "---------------------" << endl;
    }
    readCoflows(inFile);
    if ( debug_ )
      cerr << endl;
    
    //read links
    inFile >> n;
    setNumberofLinks(n);
    if ( debug_ ) {
      cerr << "Number of links is " << nbLinks_ << endl << endl;
      cerr << "LINKS" << endl << "---------------------" << endl;
    }
    for (i=0; i<nbLinks_; i++) {
      inFile >> l;
      addLink(i,l);
    }
    if ( debug_ )
      cerr << endl;
  }
  
  //compute min capa over paths
  min_capa_on_path();
  
  // fermeture du fichier
  inFile.close();
}



//----------------------------------------------------------------//
// Public Method:     print                                       //
//----------------------------------------------------------------//

void Network::print() {
  int i, k;

  cerr << "Number of coflows is " << nbCoflows_ << endl << endl;
  cerr << "COFLOWS" << endl << "---------------------" << endl;

  for (k=0; k<nbCoflows_; k++) {
    cerr << endl << coflow_[k] << endl;
    for (i=0; i<coflow_[k].getNbFlow(); i++)
      coflow_[k].printFlow(i);
  }
  cerr << endl << endl;
  cerr << "Number of links is " << nbLinks_ << endl << endl;
  cerr << "LINKS" << endl << "---------------------" << endl;
  for (i=0; i<nbLinks_; i++)
    cerr << link_[i] << endl;
  cerr << endl;
}


//----------------------------------------------------------------//
// Public Method: getCoflowParameters                             //
// Cette methode determine l'id interne du coflow, sa deadline et //
// son poids a partir de l'id externe                             //
//----------------------------------------------------------------//

void Network::getCoflowParameters(int id, int &k, double &d, int &c, int & w) {
  k = idMap_[id];
  d = coflow_[k].getEndTime();
  c = coflow_[k].getClassId();
  w = coflow_[k].getWeight();
}


//----------------------------------------------------------------//
// Public Method:  min_capa_on_path                               //
// Cette methode calcule la capacite minimale sur le chemin de    //
// chaque flot.                                                   //
//----------------------------------------------------------------//

void Network::min_capa_on_path() {
  int    length;
  int    k, i, m, j, n;
  double c;
  double min_capa = INFTY;

  for (k=0; k<nbCoflows_; k++) {
    n = coflow_[k].getNbFlow();
    for (i=0; i<n; i++) {
      min_capa = INFTY;
      length = coflow_[k].getLength(i);
      for (j=0; j<length; j++) {
	m = coflow_[k].getLink(i,j);
	m = linkIdMap_[m];
	c = link_[m].getCapa();
	if ( c < min_capa )
	  min_capa = c;
      }
      coflow_[k].setFlowMinCapa(i,min_capa);
    }
  }
}



//----------------------------------------------------------------//
// Public Method:    next_release_time                            //
// Methode calculant le workload total (temps de terminaison) du  //
// lien link_id (id externe) etant donne l'ensemble S des         //
// coflows a executer                                             //
// Cette methode est utilisee par dcoflow dans la fonction        //
// bottleneck_link pour trouver le lien goulot d'etranglement     //
//----------------------------------------------------------------//

double Network::next_release_time(int link_id, set<int> & S, bool RealFlowSizes) {
  double              x = 0.0;
  double              p;
  set<int>::iterator  it = S.begin();
  int                 i = link_id;
  int                 j=link_[i].getId();
  int                 k;
  
  // initialisation du temps de terminaison
  link_[i].setCompletionTime(0.0);
  //  x = link_[i].release_date();
  if ( debug_ )
    cerr << "\tLink " << j << ": release date = " << x << endl;
  
  //Parcourt de tous les coflows
  for (; it != S.end(); it++) {
    k = idMap_[*it];
    p = total_transmission_time(i,k,RealFlowSizes) ;
    if ( debug_ )
      if ( p > 0.0 )
	cerr << "\tLink " << j << ": coflow " << *it << " adds " << p << " to workload" << endl;
    x += p;
  }

  if ( debug_ )
    cerr << "\tLink " << j << ": Total workload is " << x  << endl;
  return x;
}


//----------------------------------------------------------------//
// Public Method:    bottleneck_link                              //
// Etant donne l'ensemble S des coflows, cette methode calcule    //
// le temps de terminaison de tous les liens et retourne l'id     //
// externe de celui qui termine en dernier. Elle est utilisee dans//
// dcoflow.                                                       //
//----------------------------------------------------------------//

int Network::bottleneck_link(set<int> & S, double & max_end_time, bool RealFlowSizes ) {
  double              x;
  int                 k, i, j;
  int                 arg_max = 0;


  //initialisation
  max_end_time = 0.0;
  // cerr << "In bottleneck_link -nbCoflows_: " <<nbCoflows_<< endl;
  // cerr << "In bottleneck_link -nbLinks_: " <<nbLinks_<< endl;
  //Parcourt des liens
  for (i=0; i<nbLinks_; i++) {

    // calcul du temps de terminaison pour ce lien
    x = next_release_time(i, S, RealFlowSizes);
    link_[i].setCompletionTime(x);
    if ( debug_ )
      cerr << "Completion time is " << x << " on link " << link_[i].getId() << endl;

    // mise a jour de max et arg_max
    if ( x >= max_end_time ) {
      max_end_time = x;
      arg_max = i;
    }
  }

  max_end_time=next_release_time(arg_max, S, RealFlowSizes ) ;

  return arg_max;
}


//----------------------------------------------------------------//
// Public Method:    bottleneck_coflows                           //
// Cette methode prend en parametre le lien bottleneck b et son   //
// temps de terminaison max_end_time, ainsi que l'ensemble S des  //
// coflows. Elle calcule l'ensemble S_b des coflows utilisant le  //
// lien bottleneck et determine l'id du coflow schedulable en     //
// dernier sur b avec la plus grande deadline (-1 si aucun)       //
//----------------------------------------------------------------//

int Network::bottleneck_coflows(int b, double max_end_time, set<int> & S, set<int> & S_b) {

  set<int>::iterator  it = S.begin();
  double              max_deadline, d;
  int                 k, arg_max, w, c;

  // initialisation
  S_b.clear();
  max_deadline = max_end_time;
  arg_max = -1;

  // Parcourt des coflows
  for (; it != S.end(); it++) {
    getCoflowParameters(*it, k, d, c, w);

    // on saute les coflows n'utilisant pas le bottleneck
    if ( !coflow_[k].useLink(b) ) continue;

    //on insere tous les autres coflows dans S_b
    S_b.insert( *it );

    //on determine le coflow schedulable en dernier avec le plus grand deadline
    if ( d > max_deadline ) {
      max_deadline = d;
      arg_max = *it;
    } // end if
  } // end for

  return arg_max;
}



//----------------------------------------------------------------//
// Public Method:   sumOfWeights                                  //
// Cette methode retourne la somme des poids des coflows de S_b   //
//----------------------------------------------------------------//

int Network::sumOfWeights(set<int> S_b) {
  int                 n = 0;
  int                 k, w, c;
  double              d;
  set<int>::iterator  it = S_b.begin();

  for ( ; it != S_b.end(); it++) {
    getCoflowParameters(*it,k,d,c,w);
    n += w;
  }
  return n;
}



//----------------------------------------------------------------//
// Public Method:   sort_EDD_order                                //
// Cette methode trie une ensemble de coflows dans l'ordre EDD    //
// S_b est l'ensemble des coflows utilisant le lien b.            //
//----------------------------------------------------------------//

void Network::sort_EDD_order(set<int> &S_b, list<int> &EDD_order) {
  int                 m = S_b.size();
  int                 n = 0;
  set<int>::iterator  it;
  double              min_deadline, d;
  int                 k, j, w, c;
  
  if ( debug_ )
    cerr << "EDD order : ";
  while ( n < m ) {
    min_deadline = INFTY;
    it = S_b.begin();
    for ( ; it != S_b.end(); it++) {
      getCoflowParameters(*it,k,d,c,w);
      if ( d < min_deadline ) {
	min_deadline = d;
	j = *it;
      }
    } // fin boucle sur les coflows
    EDD_order.push_back(j);
    S_b.erase(j);
    if ( debug_ )
      cerr << "C" << j << "\t";
    n++;
  }
  if ( debug_ )
    cerr << endl;
}


//----------------------------------------------------------------//
// Public Method:    DP                                           //
// Cette methode implemente l'algorithme de programmation         //
// dynamique resolvant le probleme 1|\sum w_j U_j                 //
// A l'appel, S_b est l'ensemble des coflows utilisant le lien b. //
// Au retour, S_b ne contient plus que les coflows supprimés par  //
// l'algorithme de DP.                                            //
//----------------------------------------------------------------//

void Network::DP(int b, set<int> &S_b) {
  list<int>           EDD_order;
  list<int>::iterator EDD_it;
  set<int>            All = S_b;
  int                 i = linkIdMap_[b];
  int                 W =sumOfWeights(S_b);
  int                 k, c, w, w_k, sum_w, iter;
  double             *P = new double[W+1];
  double             *P_prev = new double[W+1];
  set<int>           *selected = new set<int>[W+1];
  set<int>           *selected_prev = new set<int>[W+1];
  set<int>::iterator  select_it;
  double              d, p, cct;
  
  //Initialisation
  P_prev[0] = 0.0;
  for (w=1; w<=W; w++)
    P_prev[w] = INFTY;
  
  //Tri de S_b dans l'ordre EDD
  sort_EDD_order(All, EDD_order);
  
  //Algorithme de programmation dynamique
  sum_w = 0;
  iter = 1;
  if ( debug_ )
    cerr << "Dynamic programming : W =" << W << endl;
  EDD_it = EDD_order.begin();
  for ( ; EDD_it != EDD_order.end(); EDD_it++, iter++) {

    //recuperation des valeurs de d_k, w_k et p_k
    getCoflowParameters(*EDD_it,k,d,c,w_k);
    p = coflow_[k].loadOnLink(b)/link_[i].getCapa();
    sum_w += w_k;
    
    //equation de programmation dynamique
    for (w=0; w<=W; w++) {
      if ( w>sum_w ) {
       	P[w] = INFTY;
       	selected[w].clear();
       	continue;
      }
      if ( (w>=w_k) && (P_prev[w-w_k]+p<=d) ) {
	cct = P_prev[w-w_k]+p;
	if ( P_prev[w] <= cct ) {
	  selected[w] = selected_prev[w];
	  P[w] = P_prev[w];
	}
	else {
	  P[w] = cct;
	  selected[w] = selected_prev[w-w_k];
	  selected[w].insert(*EDD_it);
	}
      }
      else {
	P[w] = P_prev[w];
	selected[w] = selected_prev[w];
      }
    }
    
    //mise a jour de P_prev et selected_prev
    for (w=0; w<=W; w++) {
      P_prev[w] = P[w];
      selected_prev[w] = selected[w];
    }

    //affichage
    if ( debug_ ) {
      cerr << "\nIteration " << iter << " de la programmation dynamique (coflow " << *EDD_it << ")" << endl;
      cerr << "\tw=";
      for (w=0; w<=W; w++)
	cerr << w << "\t";
      cerr << endl;
      cerr << "\tP=";
      for (w=0; w<=W; w++)
	cerr << P[w] << "\t";
      cerr << endl;
      cerr << "\tS=";
      for (w=0; w<=W; w++) {
	select_it = selected[w].begin();
	cerr << "{";
	for ( ; select_it != selected[w].end(); select_it++)
	  cerr << *select_it << " ";
	cerr << "}\t";
      }
      cerr << endl;
    }
  }
  
  //Recherche de la plus grande valeur de W finie
  for (w=W; w>=0; w--)
    if ( P[w] < INFTY ) {
      select_it = selected[w].begin();
      for ( ; select_it != selected[w].end(); select_it++) 
	S_b.erase(*select_it);
      if ( debug_ ) {
	select_it = selected[w].begin();
	cerr << "Max weight = " << w << endl;
	cerr << "Selected coflows: {";
	for ( ; select_it != selected[w].end(); select_it++) 
	  cerr << *select_it << " ";
	cerr << "}" << endl;
      }
      break;
    }

  
  // affichage des coflows rejetes
  if ( debug_ ) {
    set<int>::iterator  rejected_it = S_b.begin();
    cerr << "Liste des coflots rejetes : " ;
    for ( ; rejected_it != S_b.end(); rejected_it++)
      cerr << *rejected_it << "  ";
    cerr << endl;
  }
  
  //Liberation memoire
  delete [] P;
  delete [] P_prev;
  delete [] selected;
  delete [] selected_prev;
}



//----------------------------------------------------------------//
// Public Method:    Moore_hogdson                                //
// Cette methode implemente l'algorithme de Moore Hogdson. A      //
// l'appel est l'ensemble des coflows utilisant le lien b. En     //
// retour, S_b ne contient plus que les coflows supprimés par     //
// l'algorithme de Moore Hogdson.                                 //
//----------------------------------------------------------------//

void Network::Moore_hogdson(int b, set<int> &S_b) {
  set<int>            E;
  list<int>           EDD_order;
  list<int>::iterator EDD_it, EDD_it2;
  int                 i = linkIdMap_[b];
  int                 argmax;
  int                 k, j, n, w, c;
  double              max_proc_time;
  double              d, cct, p;
  
  //Tri de S_b dans l'ordre EDD
  sort_EDD_order(S_b, EDD_order);
  
  //Algorithme de Moore Hogdson
  S_b.clear();
  EDD_it = EDD_order.begin();
  cct = 0.0;
  for ( ; EDD_it != EDD_order.end(); ) {
    getCoflowParameters(*EDD_it,k,d,c,w);
    p = coflow_[k].loadOnLink(b)/link_[i].getCapa();
    cct += p;
    // Si cct > d
    if ( cct > d ) {
      if ( debug_ )
	cerr << "Coflow "
	     << *EDD_it
	     << " does not meet its deadline (cct="
	     << cct
	     << "> d="
	     << d
	     << ")" << endl;

      // boucle pour rechercher le coflow j <_EDD k avec max proc time
      EDD_it2 = EDD_order.begin();
      max_proc_time = 0;
      for ( ; EDD_it2 != EDD_order.end(); EDD_it2++) {
	j = *EDD_it2;
	n = idMap_[j];
	p = coflow_[n].loadOnLink(b)/link_[i].getCapa();
	if ( p > max_proc_time ) {
	  max_proc_time = p;
	  argmax = j;
	}
	if ( EDD_it == EDD_it2  )
	  break;
      } // fin boucle pour argmax
      S_b.insert(argmax);
      EDD_order.remove(argmax);
      if ( debug_ ) {
	n = idMap_[argmax];
	p = coflow_[n].loadOnLink(b)/link_[i].getCapa();
	cerr << "\t=> Coflow " << argmax << " is removed (p=" << p << ")." << endl;
      }
      EDD_it = EDD_order.begin();
      cct = 0.0;
    } // fin if cct > d
    else
      EDD_it++;
  } // fin algorithme de Moore Hogdson

  //affichage pour debug
  if ( debug_ ) {
    set<int>::iterator  it;
    
    cerr << "S_b=[";
    it = S_b.begin();
    for ( ; it != S_b.end(); it++)
      cerr << *it << " ";
    cerr << "]" << endl;
  }
}



//----------------------------------------------------------------//
// Public Method:     dcoflow_v1                                  //
// Cette methode est appelle par dcoflow_reject. Elle implemente  //
// la version 1 de DCOFLOW pour mettre a jour l'index.            //
//----------------------------------------------------------------//

void Network::dcoflow_v1(int k, int i, double &index) {
  double d = coflow_[k].getEndTime();
  int    w = coflow_[k].getWeight();
  int    j = link_[i].getId();
  double x, p;

  if ( !coflow_[k].useLink(j) ) return;
  x = d-link_[i].completion_time();
  if ( x >= 0.0 ) return;
  p = coflow_[k].loadOnLink(j)/link_[i].getCapa();
  if ( p <= 0.0 ) return;
  x *= p/w;
  if ( debug_ )
    cerr << "\tCompletion time on link "
	 << j
	 << " is "
	 << link_[i].completion_time()
	 << " => sum += "
	 << p
	 << "*("
	 << d
	 << "-"
	 << link_[i].completion_time()
	 << ")="
	 << x
	 << endl;

  if ( x < index )
    index = x;
}


//----------------------------------------------------------------//
// Public Method:     dcoflow_v2                                  //
// Cette methode est appelle par dcoflow_reject. Elle implemente  //
// la version 2 de DCOFLOW pour mettre a jour l'index.            //
//----------------------------------------------------------------//

void Network::dcoflow_v2(int k, int i, double &index) {
  double d = coflow_[k].getEndTime();
  int    w = coflow_[k].getWeight();
  int    j = link_[i].getId();
  double x, p;

  if ( !coflow_[k].useLink(j) ) return;
  x = d-link_[i].completion_time();
  p = coflow_[k].loadOnLink(j)/link_[i].getCapa();
  x *= p/w;
  if ( x >= 0.0 ) return;
  if ( debug_ )
    cerr << "\tCompletion time on link "
	 << j
	 << " is "
	 << link_[i].completion_time()
	 << " => sum += "
	 << p
	 << "*("
	 << d
	 << "-"
	 << link_[i].completion_time()
	 << ")="
	 << x
	 << endl;
  
  index += x;
}


//----------------------------------------------------------------//
// Public Method:     dcoflow_v3                                  //
// Cette methode est appelle par dcoflow_reject. Elle implemente  //
// la version 3 de DCOFLOW pour mettre a jour l'index.            //
//----------------------------------------------------------------//

void Network::dcoflow_v3(int k, int i, double max_end_time, double &index) {
  double d = coflow_[k].getEndTime();
  int    w = coflow_[k].getWeight();
  int    j = link_[i].getId();
  double x, p;

  if ( !coflow_[k].useLink(j) ) return;
  if ( link_[i].completion_time() < GAMMA*max_end_time ) return;
  x = d-link_[i].completion_time();
  p = coflow_[k].loadOnLink(j)/link_[i].getCapa();
  x *= p/w;
  if ( debug_ )
    cerr << "\tCompletion time on link "
	 << j
	 << " is "
	 << link_[i].completion_time()
	 << " => sum += "
	 << p
	 << "*("
	 << d
	 << "-"
	 << link_[i].completion_time()
	 << ")="
	 << x
	 << endl;
  
  index += x;
}



//----------------------------------------------------------------//
// Public Method:     dcoflow_reject                              //
// Cette methode prend en parametre l'ensemble S_b des coflows    //
// utilisant le lien bottleneck et calcule un indice pour chacun. //
// Elle retourne ensuite l'id externe du coflow avec le plus      //
// petit indice k_star.                                           //
//----------------------------------------------------------------//

int Network::dcoflow_reject(int b, set<int> & S_b, double max_end_time) {
  int                 k, i, j, k_star;
  set<int>::iterator  it;
  double              index, min;

  //initialisation
  min = INFTY;
  k_star = -1;
  
  // Parcourt de tous les coflows dans S_b
  if ( S_b.empty() ) {
    cerr << "No coflow on bottleneck link" << endl;
    exit(-1);
  }

  //appel de l'algorithme de DP
  if ( version_ < 4 )
    DP(b, S_b);
  //Moore_hogdson(b, S_b);
  
  for (it = S_b.begin(); it != S_b.end(); it++) {

    //initialisation interne
    k = idMap_[*it];
    if ( version_ == 1 )
      index = INFTY;
    else
      index = 0.0;

    if ( debug_ )
      cerr << "RejectCoflow: considering coflow " << *it
	   << " with deadline " << coflow_[k].getEndTime() << endl;
    
    // boucle sur les liens
    for (i=0; i<nbLinks_; i++) {
      if ( version_ == 1 )
	dcoflow_v1(k,i,index);
      else if ( (version_ == 2) || (version_ == 4) )
	dcoflow_v2(k,i,index);
      else if ( version_ == 3 )
	dcoflow_v3(k,i,max_end_time, index);
      else {
	cerr << "Unknown version of dcoflow" << endl;
	exit(-1);
      }
    } // fin boucle sur les liens

    if ( debug_ )
      cerr << "\tindex = " << index << endl;
    // mise a jour du minimum
    if ( index < min ) {
      min = index;
      k_star = *it;
    }
  } // fin boucle sur les coflows

  return k_star;
}



//----------------------------------------------------------------//
// Public Method:     evalCCT                                     //
// Cette methode evalue le CCT du coflow coflowId lorsque les     //
// coflows sont executes dans l'ordre sigma. Pour chaque lien,    //
// on fait la somme des temps d'execution de tous les coflows     //
// dans l'ordre jusqu'a coflowId. Le CCT de coflowId est calcule  //
// comme etant le max de ces temps de terminaison par lien.       //
//----------------------------------------------------------------//

double Network::evalCCT(int coflowId) {
  int                  k = idMap_[coflowId];
  int                  c, j, l;
  list<int>::iterator  it = sigma_.begin();
  double               x;
  double               max = 0.0;;

  //initialisation
  coflow_[k].reset();
  
  // Calcul completion times
  for (l=0; l<nbLinks_; l++) {

    //initialisation
    link_[l].setCompletionTime(0.0);
    j = link_[l].getId();

    //on ne considere que les liens utilises par k
    if ( coflow_[k].useLink(j) == false )
      continue;
    
    //boucle sur les coflows schedules jusqu'a coflowId
    it = sigma_.begin();  
    for ( ; it != sigma_.end(); it++) {
      c = idMap_[*it];
      x = coflow_[c].loadOnLink(j);
      if ( x > 0.0 ) {
	x /= link_[l].getCapa();
	link_[l].updateCompletionTime(x);
      }
      if ( c == k )
	break;
    }

    //mise a jour CCT de k
    coflow_[k].updateCCT(link_[l].completion_time());
  }

  if ( debug_ )
    cerr << "\tCoflow " << coflowId << ": CCT=" << coflow_[k].getCCT() << ", deadline=" << coflow_[k].getEndTime() << endl;

  return coflow_[k].getCT();
}




//----------------------------------------------------------------//
// Public Method:    removeLateCoflows                            //
//----------------------------------------------------------------//

void Network::removeLateCoflows(list<int> & sigma_star) {
  int    coflowId, k, w, c;
  double cct, d;
  
  while ( sigma_star.empty() == false ) {
    coflowId = sigma_star.front();
    getCoflowParameters(coflowId,k,d,c,w);
    if ( debug_ )
      cerr << "\n\n----------- RemoveLateCoflow: eval CCT of coflow " << coflowId << "----------------\n" << endl;
    cct = evalCCT(coflowId);
    if ( cct > d ) {
      sigma_.remove(coflowId);
      coflow_[k].reject();
      if ( debug_ )
	cerr << "\tCoflow " << coflowId << " is definitely rejected" << endl;
    }
    sigma_star.pop_front();
  }
}


//----------------------------------------------------------------//
// Public Method:     dcoflow                                     //
//----------------------------------------------------------------//


int Network::dcoflow(double & cost) {
  set<int>     S, S_b, R;
  list<int>    sigma_star;
  int          k, b, max_deadline_coflow;
  int          n, i, j;
  double       max_end_time;
  
  // initialisation
  sigma_.clear();
  for (k=0; k<nbCoflows_; k++) {
    S.insert(coflow_[k].getId());
    //    cerr << "Insert " << coflow_[k].getId() << " into S" << endl;
  }
  //boucle principale
  while ( !S.empty() ) {

    //determination du lien bottleneck
    b = bottleneck_link(S, max_end_time, true);
    if ( debug_ )
      cerr << "Bottleneck link is " << b << " with completion time " << max_end_time << endl;

    if ( max_end_time < 1.0e-20 ) {
      cerr << "Maximum end time is " << max_end_time << " , size of S is " << S.size() << endl;
      set<int>::iterator S_it = S.begin();

      cerr <<"\nS = ";
      for ( ; S_it != S.end(); S_it++)
	cerr << *S_it <<", ";
      cerr << endl;
      exit(-1);
    }

    
    //determination des coflows utilisant le bottleneck => ensemble S_b
    //determination du coflow schedulable en dernier avec le plus grand deadline (-1 sinon)
    max_deadline_coflow = bottleneck_coflows(b, max_end_time, S, S_b);

    //est-ce qu'on peut scheduler un coflow en dernier ?
    if ( max_deadline_coflow != -1 ) {
      // Si oui, on l'ajoute en premier a l'ordre sigma
      if ( debug_ )
       	cerr << "Coflow " << max_deadline_coflow << " can be scheduled as the last one on link " << b << endl;
      sigma_.push_front(max_deadline_coflow);
      S.erase(max_deadline_coflow);
    }
    else {
      // Sinon il faut enlever un coflow de S
      if ( debug_ )
       	cerr << "No coflow can be scheduled as the last one on link " << b << endl;
      k = dcoflow_reject(b, S_b, max_end_time);
      if ( debug_ )
       	cerr << "Coflow " << k << " is rejected among those using link " << b << endl;
      sigma_.push_front(k);
      sigma_star.push_front(k);
      S.erase(k);
    }
    if ( debug_ ) 
      cerr << "\n\n---------------------- Size of set S is " << S.size() << " ----------------------\n\n" << endl;
  } // fin boucle principale

  //affichage
  list<int>::iterator it;
  if ( debug_ ) {
    cerr << "sigma=[";
    it=sigma_.begin();
    for ( ; it != sigma_.end(); ) {
      cerr << *it;
      it++;
      if ( it != sigma_.end() )
	cerr << ", ";
      else
	cerr << "]" << endl;
    }
    cerr << "sigma*=[";
    it=sigma_star.begin();
    for ( ; it != sigma_star.end(); ) {
      cerr << *it;
      it++;
      if ( it != sigma_star.end() )
	cerr << ", ";
      else
	cerr << "]" << endl;
    }
  }
  
  //appel de reset
  reset();
  
  //appel de removeLateCoflows
  removeLateCoflows(sigma_star);

  // if ( debug_ )
  //   printSigma();

  //mise a jour des priorites
  it=sigma_.begin();
  n = 0;
  for ( ; it != sigma_.end(); it++) {
    k = idMap_[*it];
    
    //boucle sur les flots
    for (i=0; i<coflow_[k].getNbFlow(); i++) 
      coflow_[k].setFlowPriority(i,n);
    n++;
  }
  
  //affectation du cout
  cost = sigma_.size();

  return 1;
}


//----------------------------------------------------------------//
// Public Method:     printSigma                                  //
//----------------------------------------------------------------//

void Network::printSigma() {
  list<int>::iterator  it = sigma_.begin();
  
  cerr << "sigma=[";
  it=sigma_.begin();
  for ( ; it != sigma_.end(); ) {
    cerr << *it;
    it++;
    if ( it != sigma_.end() )
      cerr << ", ";
    else
      cerr << "]" << endl;
  }
}



//----------------------------------------------------------------//
// Public Method:     printCCT                                    //
//----------------------------------------------------------------//

void Network::printCCT() {
  list<int>::iterator  it = sigma_.begin();
  int                  k;
  
  //  if ( debug_ ) {
  cerr << "ccts=[" << std::setprecision(2) << fixed;
  it=sigma_.begin();
  for ( ; it != sigma_.end(); ) {
    k = idMap_[*it];
    cerr << coflow_[k].getCCT();
    it++;
    if ( it != sigma_.end() )
      cerr << ", ";
    else
      cerr << "]" << endl;
  }
}


//----------------------------------------------------------------//
// Public Method:   normalized_CCT                                //
//----------------------------------------------------------------//

double Network::normalized_CCT(int k) {
  double b, x;
  int    i,j;
  
  b = 0.0;
  for (i=0; i<nbLinks_; i++) {
    j = link_[i].getId();
    x = coflow_[k].loadOnLink(j)/link_[i].getCapa();
    if ( x > b )
      b = x;
  }

  return b;
}



//----------------------------------------------------------------//
// Public Method:    getCDSrate                                   //
//----------------------------------------------------------------//

double Network::getMeanCCT(bool online, double & avg_cct, double &avg_cct_norm,
			   double & mu_min, double & mu_max, bool RealFlowSizes) {
  int                  k, l, j;
  double               x, y;

  //initialisation
  avg_cct = 0.0;
  avg_cct_norm = 0.0;
  mu_min = 1.0e3;
  mu_max = 0.0;
  // debug_ = true;
  //calcul mu_min et mu_max
  for (l=0; l<nbLinks_; l++) {
    j = link_[l].getId();
    for (k=0; k<nbCoflows_; k++) {
      if ( !coflow_[k].useLink(j) ) continue;
      if ( debug_ )
	cerr << "getMeanCCT: coflow "
	     << coflow_[k].getId()
	     << " on link "
	     << j
	     << " - real proc. time="
	     << total_transmission_time(l, k, true)
	     << " and predicted proc. time="
	     << total_transmission_time(l, k, RealFlowSizes)
	     << endl;
      y = total_transmission_time(l, k, RealFlowSizes);
      y /= total_transmission_time(l, k, true);
      if ( y > mu_max )
	mu_max = y;
      if ( y < mu_min )
	mu_min = y;
    }
  }
  if ( debug_ )
    cerr << "mu_min=" << mu_min << ", mu_max=" << mu_max << endl;
  
  // drop after
  // cerr << "mu_min=" << mu_min << ", mu_max=" << mu_max << " ,nu= " << mu_max/ mu_min << endl;

  //calcul des CCTS
  if ( debug_ )
    cerr << "\nCCT=[";
  for (k=0; k<nbCoflows_; k++) {
    x = coflow_[k].getCCT();
    if ( debug_ )
      cerr << x << " ";
    if ( x <= coflow_[k].getEndTime() ) {
      avg_cct += x;
      avg_cct_norm += x/normalized_CCT(k);
    }
  }
  if ( debug_ )
    cerr << "]" << endl;
  avg_cct /= nbCoflows_;
  avg_cct_norm /= nbCoflows_;
  // debug_ = false;
  return avg_cct;
}


double Network::getNU(bool RealFlowSizes){
  
  int                  k, l, j;
  double               y;
  double mu_min = 1.0e3;
  double mu_max = 0.0;
  // debug_ = true;
  //calcul mu_min et mu_max
  for (l=0; l<nbLinks_; l++) {
    j = link_[l].getId();
    for (k=0; k<nbCoflows_; k++) {
      if ( !coflow_[k].useLink(j) ) continue;
	
      y = total_transmission_time(l, k, RealFlowSizes);
      y /= total_transmission_time(l, k, true);
      if ( y > mu_max )
        mu_max = y;
      if ( y < mu_min )
        mu_min = y;
    }
  }
  return mu_max/mu_min ;
}

double Network::getMU_MAX(bool RealFlowSizes){
  
  int                  k, l, j;
  double               y;
  double mu_min = 1.0e3;
  double mu_max = 0.0;
  // debug_ = true;
  //calcul mu_min et mu_max
  for (l=0; l<nbLinks_; l++) {
    j = link_[l].getId();
    for (k=0; k<nbCoflows_; k++) {
      if ( !coflow_[k].useLink(j) ) continue;
	
      y = total_transmission_time(l, k, RealFlowSizes);
      y /= total_transmission_time(l, k, true);
      if ( y > mu_max )
        mu_max = y;
    }
  }
  return mu_max ;
}
//----------------------------------------------------------------//
// Public Method: printNumberOfVariables                          //
//----------------------------------------------------------------//

void Network::printNumberOfVariables() {
  int                  k, l, j;
  int                  nb=0;

  //calcul du nombre de p_{l,k} non nuls
  for (l=0; l<nbLinks_; l++) {
    j = link_[l].getId();
    for (k=0; k<nbCoflows_; k++) {
      if ( coflow_[k].useLink(j) )
	nb++;
    }
  }
  cerr << "Number of non-zero p_{l,k} is " << nb << endl;
}




//----------------------------------------------------------------//
// Public Method:     cs_dp                                       //
//----------------------------------------------------------------//

int Network::cs_dp(double & cost) {
  set<int>            S; // set of all coflows
  set<int>            R; // rejected coflows
  set<int>            A; // accepted coflows
  set<int>::iterator  it;
  int                 k, i, j, w, n, c;
  double              d, b;

  // initialisation
  sigma_.clear();
  for (k=0; k<nbCoflows_; k++) 
    S.insert(coflow_[k].getId());

  
  // boucle sur les liens
  for (i=0; i<nbLinks_; i++) {
    int          link_id = link_[i].getId();
    set<int>     S_link;

    //construction de l'ensemble des coflows utilisant le port i
    for (it = S.begin(); it != S.end(); it++) {
      getCoflowParameters(*it, k, d, c, w);
      if ( coflow_[k].useLink(link_id) ) 
	S_link.insert( *it );
    } // end for

    //algorithme de programmation dynamique
    DP(i, S_link);

    //on marque les coflows rejetes sur ce port comme definitivement rejetes
    for (it = S_link.begin(); it != S_link.end(); it++)
      R.insert( *it );
  } // fin boucle sur les ports
  

  // calcul des coflows acceptes
  std::set_difference(S.begin(), S.end(),
		      R.begin(), R.end(),
		      std::inserter(A, A.end()));
  
  //affichage
  if ( debug_ ) {
    
    cerr << "List of accepted coflows : ";
    for ( it=A.begin(); it != A.end(); it++) 
      cerr << *it << "  ";
    cerr << endl;
    cerr << "List of rejected coflows : ";
    for (it = R.begin(); it != R.end(); it++) 
      cerr << *it << "  ";
    cerr << endl;
  }

  // Construction de l'ordre final
  set<Job>           final_order;
  set<Job>::iterator final_it;
  double             max_deadline = 0.0;
  double             x;
  for (it=A.begin(); it != A.end(); it++) {
    if ( debug_ )
    cerr << "Next accepted coflow is " << *it << endl;
    getCoflowParameters(*it,k,d,c,w);
    if ( d > max_deadline )
      max_deadline = d;
    final_order.insert( Job(*it,1.0,d) );
  }
  for (it=R.begin(); it != R.end(); it++) {
    getCoflowParameters(*it,k,d,c,w);
    b = 0.0;
    for (i=0; i<nbLinks_; i++) {
      j = link_[i].getId();
      x = coflow_[k].loadOnLink(j)/link_[i].getCapa();
      if ( x > b )
	b = x;
    }
    if ( debug_ )
      cerr << "Next rejected coflow is " << *it << ": max g_i^k=" << b  << ", deadline=" << d << ", w=" << w << endl;
    final_order.insert( Job(*it,1.0,max_deadline+b/(w*(d+EPS))) );
  }

  //appel de reset
  reset();

  if ( debug_ )
    cerr << "Final order: ";
  final_it = final_order.begin();
  sigma_.clear();
  n = 0;
  for ( ; final_it != final_order.end(); final_it++) {
    k = idMap_[final_it->id()];
    if ( debug_ )
      cerr << final_it->id() << " (w=" << final_it->dueDate() << ")  ";

    //recopie dans la liste sigma_ contenant l'ordre
    sigma_.push_back(final_it->id());

    //mise a jour des priorites
    for (i=0; i<coflow_[k].getNbFlow(); i++) 
      coflow_[k].setFlowPriority(i,n);
    n++;
  }

  cost = sigma_.size();

  return 0;
}



//----------------------------------------------------------------//
// Public Method:     cs_mha                                      //
//----------------------------------------------------------------//

int Network::cs_mha(double & cost) {
  double     **p = new double *[nbCoflows_];
  double       b, d;
  set<int>     all_coflows, selected_coflows, rejected_coflows;
  int          k, i, j, n, w, c;

  // allocation memoire et initialisation
  for (k=0; k<nbCoflows_; k++) {
    p[k] = new double[nbLinks_]; 
    for (i=0; i<nbLinks_; i++) {
      j = link_[i].getId();
      p[k][i] = coflow_[k].loadOnLink(j)/link_[i].getCapa();
      //      cerr << "p(" << k << "," << i << ") = " << p[k][i] << endl;
    }
    all_coflows.insert(coflow_[k].getId());
  }
  
  // boucle sur les liens
  for (i=0; i<nbLinks_; i++) {

    set<Job>            my_jobs;
    set<int>            S; // accepted jobs
    set<int>            E; // rejected jobs
    set<Job>::iterator  it;
    Job                 rejected_job;
    double              comp_time_of_last_job = 0.0;
    
    // construction du set de jobs utilisant ce lien
    for (k=0; k<nbCoflows_; k++) {
      b = p[k][i];
      d = coflow_[k].getEndTime();
      j = coflow_[k].getId();
      if ( b > 0.0 )
	my_jobs.insert( Job(j,b,d) );
    }

    if ( my_jobs.empty() ) {
      if ( debug_ )
	cerr << "No coflow on port " << link_[i].getId() << "!" << endl;
      continue;
    }
    
    //affichage
    if ( debug_ ) {
      it = my_jobs.begin();
      cerr << "List of coflows using port " << link_[i].getId() << ":" << endl;
      for ( ; it != my_jobs.end(); it++) {
     	cerr << "\tCoflow " << it->id() << " : p=" << it->processingTime() << ", d=" << it->dueDate() << endl;
      }
      cerr << endl;
    }

    // Algorithme de Moore-Hogdson
    it = my_jobs.begin();
    for ( ; it != my_jobs.end(); ) {
      S.insert(it->id());
      comp_time_of_last_job += it->processingTime();
      if ( comp_time_of_last_job > it->dueDate() ) {
	if ( debug_ )
	  cerr << "NOK - Job (" << it->id() << "," << it->processingTime() << "," << it->dueDate() << ") has completion time " << comp_time_of_last_job << endl; 
	find_argmax_proc_time(my_jobs, it->id(),rejected_job);
	if ( debug_ )
	  cerr << "\tRejected job is (" << rejected_job.id() << "," << rejected_job.processingTime() << "," << rejected_job.dueDate() << ")" << endl; 
	S.erase(rejected_job.id());
	E.insert(rejected_job.id());
	my_jobs.erase(rejected_job);
	it = my_jobs.begin();
	comp_time_of_last_job = 0.0;
      }
      else {
	if ( debug_ )
	  cerr << "OK - Job (" << it->id() << "," << it->processingTime() << "," << it->dueDate() << ") has completion time " << comp_time_of_last_job << endl;
	it++;
      }
    } // fin algorithm Moore-Hogdson

    //affichage
    if ( debug_ ) {
      set<int>::iterator coflow_it = S.begin();
      
      cerr << "List of accepted coflows on port " << link_[i].getId() << ": ";
      for ( ; coflow_it != S.end(); coflow_it++) 
     	cerr << *coflow_it << "  ";
      cerr << endl;
      coflow_it = E.begin();
      cerr << "List of rejected coflows on port " << link_[i].getId() << ": ";
      for ( ; coflow_it != E.end(); coflow_it++) 
     	cerr << *coflow_it << "  ";
      cerr << endl;
    }

    // mise a jour des coflows exclus
    rejected_coflows.insert(E.begin(), E.end());
  } // fin boucle sur les ports
  

  // calcul des coflows acceptes
  std::set_difference(all_coflows.begin(), all_coflows.end(), rejected_coflows.begin(), rejected_coflows.end(),
		      std::inserter(selected_coflows, selected_coflows.end()));
  
  //affichage
  if ( debug_ ) {
    set<int>::iterator coflow_it = selected_coflows.begin();
    
    cerr << "List of accepted coflows : ";
    for ( ; coflow_it != selected_coflows.end(); coflow_it++) 
      cerr << *coflow_it << "  ";
    cerr << endl;
    coflow_it = rejected_coflows.begin();
    cerr << "List of rejected coflows : ";
    for ( ; coflow_it != rejected_coflows.end(); coflow_it++) 
      cerr << *coflow_it << "  ";
    cerr << endl;
  }

  // Construction de l'ordre final
  set<Job>           final_order;
  set<Job>::iterator final_it;
  set<int>::iterator coflow_it = selected_coflows.begin();
  double             max_deadline = 0.0;
  for ( ; coflow_it != selected_coflows.end(); coflow_it++) {
    if ( debug_ )
      cerr << "Next accepted coflow is " << *coflow_it << endl;
    getCoflowParameters(*coflow_it,k,d,c,w);
    if ( d > max_deadline )
      max_deadline = d;
    final_order.insert( Job(*coflow_it,1.0,d) );
  }
  coflow_it = rejected_coflows.begin();
  for ( ; coflow_it != rejected_coflows.end(); coflow_it++) {
    getCoflowParameters(*coflow_it,k,d,c,w);
    b = 0.0;
    for (i=0; i<nbLinks_; i++)
      if ( p[k][i] > b )
	b = p[k][i];
    if ( debug_ )
      cerr << "Next rejected coflow is " << *coflow_it << ": max g_i^k=" << b  << ", deadline=" << d << endl;
    final_order.insert( Job(*coflow_it,1.0,max_deadline+b/(d+EPS)) );
  }

  //appel de reset
  reset();

  if ( debug_ )
    cerr << "Final order: ";
  final_it = final_order.begin();
  sigma_.clear();
  n = 0;
  for ( ; final_it != final_order.end(); final_it++) {
    k = idMap_[final_it->id()];
    if ( debug_ )
      cerr << final_it->id() << " (w=" << final_it->dueDate() << ")  ";

    //recopie dans la liste sigma_ contenant l'ordre
    sigma_.push_back(final_it->id());

    //mise a jour des priorites
    for (i=0; i<coflow_[k].getNbFlow(); i++) 
      coflow_[k].setFlowPriority(i,n);
    n++;
  }

  cost = sigma_.size();
  
  // liberation memoire
  for (k=0; k<nbCoflows_; k++)
    delete [] p[k];
  delete [] p;
  
  return 0;
}







//----------------------------------------------------------------//
// Public Method:     optimal_solution                            //
//----------------------------------------------------------------//


int Network::optimal_solution(double & cost) {

  #ifdef GUROBI
  compute_time_slots();
  return solve(cost, false);
  #else
  return 0;
  #endif
}





#ifdef GUROBI



//----------------------------------------------------------------//
// Public Method:       solve                                     //
//----------------------------------------------------------------//

int Network::solve(double & cost, bool approx) {

  int                 i, j, k, t;
  double              result = 0.0;
  int                 status;

  //initialisation du cout
  cost = 0.0;
  //  debug_ = true;
  
  try {
    GRBEnv env = GRBEnv("/tmp/x.log");
    GRBModel model = GRBModel(env);

    //     create variable
    GRBVar     **f = new GRBVar *[nbFlows_];
    GRBVar     **y = new GRBVar *[nbCoflows_];
    GRBVar     **gamma = new GRBVar *[nbCoflows_];
    GRBVar      *C = new GRBVar[nbCoflows_];
    GRBConstr * cstr = 0;
    
    // allocation memoire
    for (i=0; i<nbFlows_; i++) 
      f[i] = new GRBVar[nbSlots_];
    for (k=0; k<nbCoflows_; k++) {
      y[k] = new GRBVar[nbSlots_];
      gamma[k] = new GRBVar[nbSlots_];
    }
    
    // declaration des variables d'optimisation a Gurobi
    write_flow_var(model, f, y, gamma, C);
    if ( debug_ )
      cerr << "Variables d'optimisations declarees" << endl;
    
    //limites de temps et sur la precision pour la resolution du modele
    model.getEnv().set(GRB_IntParam_OutputFlag, 0);
    // model.getEnv().set(GRB_DoubleParam_TimeLimit, GRB_TIME_LIMIT);
    model.getEnv().set(GRB_IntParam_MIPFocus, 1);
    //    model.getEnv().set(GRB_DoubleParam_MIPGap, 0.000001);
    
    //pas d'output
    model.getEnv().set(GRB_IntParam_OutputFlag, 0);
    
    //Mise a jour du modele
    model.update();
    if ( debug_ )
      cerr << "Modele Gurobi mis a jour" << endl;
    
    //Fonction objectif
    write_objective_function(model, C);
    
    //Ajout des contraintes de conservation des flots
    write_flow_cstr(model, f, y, gamma, C);
    write_capa_cstr(model, f, y, gamma, C);
    if ( debug_ )
      cerr << "Contraintes ecrites" << endl;
    
    //Mise a jour du modele
    model.update();
    if ( debug_ )
      cerr << "Modele Gurobi mis a jour" << endl;
    
    //Ecriture du modele dans un fichier
    if (debug_ )
      model.write("/tmp/prob.lp");
    
    //Optimisation du modele
    //    cerr << "Lancement de l'optimisation avec Gurobi" << endl;
    model.optimize();
    status = model.get(GRB_IntAttr_Status);
    if (status == GRB_UNBOUNDED)
      {
   	cerr << "The model cannot be solved "
   	     << "because it is unbounded" << endl;
   	result = -1;
      }
    else if (status == GRB_INFEASIBLE)
      {
   	cerr << "The model cannot be solved "
   	     << "because there is no feasible solution" << endl;

   	// do IIS
   	cout << "The model is infeasible; computing IIS" << endl;
   	model.computeIIS();
   	cout << "\nThe following constraint(s) "
   	     << "cannot be satisfied:" << endl;
   	cstr = model.getConstrs();
   	for (int i = 0; i < model.get(GRB_IntAttr_NumConstrs); ++i)
   	  {
   	    if (cstr[i].get(GRB_IntAttr_IISConstr) == 1)
   	      {
   		cout << cstr[i].get(GRB_StringAttr_ConstrName) << endl;
   	      }
   	  }
	
   	result = -2;
      }
    else
      {
   	result = 0;
	if ( debug_ ) {
	  cost = model.get(GRB_DoubleAttr_ObjVal);
   	  cerr << "GUROBI: sum of CCT = " << cost << endl << endl;
   	  cerr << "Runtime : " << model.get(GRB_DoubleAttr_Runtime) << endl;
	}
   	cost = 0.0;
	
   	pair<int,int>  my_pair;
   	double         tmp, s;
	
   	for (k=0; k<nbCoflows_; k++) {
   	  tmp = C[k].get(GRB_DoubleAttr_X);
	  if ( debug_ )
	    cerr << "Coflow " << k << " completes at time " << tmp << endl;
	  cost+=tmp;
	  coflow_[k].setCCT(tmp);	  
   	}
	
   	if ( debug_ ) {	  

	  cerr << endl << "VARIABLES y_k(t) : " << endl;
	  for (k=0; k <nbCoflows_; k++) {
	    for (t=0; t<nbSlots_; t++) {
   	      tmp = y[k][t].get(GRB_DoubleAttr_X);
	      cerr << "y[" << k << "](" << t << ")=" << tmp << "  ";
	    }
	    cerr << endl;
	  }

	  cerr << endl << "SCHEDULING OVER TIME SLOTS : " << endl;
	  for (t=0; t<nbSlots_; t++) {
   	    cerr << endl << "SCHEDULING OVER SLOT [" << slotStart_[t] << "," << slotEnd_[t] << "]"  << endl << "----------------------------" << endl;
   	    for (j=0; j<nbFlows_; j++) {
   	      tmp = f[j][t].get(GRB_DoubleAttr_X);
   	      my_pair = flowMap_[j];
   	      k = my_pair.first;
   	      s = coflow_[k].getFlowSize( my_pair.second );
   	      i = coflow_[k].getFlowId( my_pair.second );
   	      if ( tmp > 1.0e-4 ) {
   		cerr << "f_" 
   		     << i
   		     << "("
   		     << t
   		     <<") = "
   		     << tmp
   		     << endl;
   	      }
   	    }
   	  }
	}
      }
    
    //liberation memoire
    for (i=0; i<nbFlows_; i++) {
      delete [] f[i];
    }
    for (k=0; k<nbCoflows_; k++) {
      delete [] y[k];
      delete [] gamma[k];
    }
    
    delete [] f;
    delete [] y;
    delete [] gamma;
    delete [] C;
  } catch(GRBException e) {
    cerr << "Error code = " << e.getErrorCode() << endl;
    cerr << e.getMessage() << endl;
  } catch(...) {
    cerr << "Exception during optimization" << endl;
  }
  //  debug_ = false;
  
  return result;

}



//----------------------------------------------------------------//
// Public Method:  write_objective_function                       //
//----------------------------------------------------------------//

 void Network::write_objective_function(GRBModel & model, GRBVar *C)
 {
   int          k;
   double       w;
   GRBLinExpr   obj=0;

   for (k=0; k<nbCoflows_; k++) {
     w = coflow_[k].getWeight();
     obj += w*C[k];
   }
   model.setObjective(obj,GRB_MINIMIZE);

   if ( debug_ )
     cerr << "Objectif d'optimisation fixe" << endl;
 }



//----------------------------------------------------------------//
// Public Method:  write_flow_var                                 //
//----------------------------------------------------------------//

void Network::write_flow_var(GRBModel & model, GRBVar **f, GRBVar **y, GRBVar **gamma, GRBVar *C)
 {
   int            j, k, i, t;
   pair<int,int>  my_pair;
   int            cmpt = 0;
  
   // variables f
   for (j=0; j <nbFlows_; j++) {
     my_pair = flowMap_[j];
     k = my_pair.first;
     i = coflow_[k].getFlowId( my_pair.second );
     string f_name = itos(i);
     for (t=0; t<nbSlots_; t++) {
       f[j][t] = model.addVar(0.0, 1.0, 0.0, GRB_CONTINUOUS, "f_"+f_name+"("+itos(t)+")");
       cmpt++;
     }
   }

   // variables y
   for (k=0; k <nbCoflows_; k++) {
     string cof_name = itos(k);
     for (t=0; t<nbSlots_; t++) {
       y[k][t] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY, "y_"+cof_name+"("+itos(t)+")");
       cmpt++;
     }
   }

   // variables gamma
   for (k=0; k <nbCoflows_; k++) {
     string cof_name = itos(k);
     for (t=0; t<nbSlots_; t++) {
       gamma[k][t] = model.addVar(0.0, 1.0, 0.0, GRB_CONTINUOUS, "gamma_"+cof_name+"("+itos(t)+")");
       cmpt++;
     }
   }

   // variables C
   for (k=0; k <nbCoflows_; k++) {
     string cof_name = itos(k);
     C[k] = model.addVar(0.0, GRB_INFINITY, 0.0, GRB_CONTINUOUS, "C_"+cof_name);
     cmpt++;
   }
   //   cerr << "Nombre de variables: " << cmpt << endl;
 }



//----------------------------------------------------------------//
// Public Method: write_flow_cstr                                 //
//----------------------------------------------------------------//

void Network::write_flow_cstr(GRBModel & model, GRBVar **f, GRBVar **y, GRBVar **gamma, GRBVar *C)
 {
    GRBLinExpr               lhsExpr=0;
    int                      flow, t, k, i, j;
    pair<int,int>            my_pair;

    //contrainte definissant les variables C
    for (k=0; k<nbCoflows_; k++) {
      lhsExpr = C[k];
      for (t=0; t<nbSlots_; t++)
	lhsExpr -= slotSize_[t]*((t+1)*y[k][t]-gamma[k][t]);
      model.addConstr(lhsExpr, GRB_EQUAL, 0.0,  "cstr_C_"+itos(k));
    }

    //contrainte de conservation sur les y
    for (k=0; k<nbCoflows_; k++) {
      lhsExpr = 0;
      for (t=0; t<nbSlots_; t++)
	lhsExpr += y[k][t];
      model.addConstr(lhsExpr, GRB_EQUAL, 1.0,  "cstr_y_"+itos(k));
    }

    //contrainte de conservation sur les f
    for (flow=0; flow <nbFlows_; flow++) {
      my_pair = flowMap_[flow];
      k = my_pair.first;
      i = my_pair.second;
      lhsExpr = 0;
      j = coflow_[k].getFlowId( i );
      for (t=0; t<nbSlots_; t++) 
	lhsExpr += f[flow][t];
      model.addConstr(lhsExpr, GRB_EQUAL, 1.0,  "consv_"+itos(j));
    }
 }
   

//----------------------------------------------------------------//
// Public Method: write_capa_cstr                                 //
//----------------------------------------------------------------//

void Network::write_capa_cstr(GRBModel & model, GRBVar **f, GRBVar **y, GRBVar **gamma, GRBVar *C)
 {
    GRBLinExpr               lhsExpr=0;
    int                      l, j, k, i, t, t2, link_id;
    double                   size, Delta;
    int                      cmpt, n;
    pair<int,int>            my_pair;
    map<pair<int,int>, int>  reverseMap;

    //construction map inverse
    for (j=0; j<nbFlows_; j++) {
      my_pair = flowMap_[j];
      reverseMap[my_pair] = j;
    }
    
    // contrainte gamma <= y
    for (k=0; k<nbCoflows_; k++) {
      for (t=0; t<nbSlots_; t++) {
	lhsExpr = gamma[k][t] - y[k][t];
 	 model.addConstr(lhsExpr, GRB_LESS_EQUAL, 0.0,  "gamma_"+itos(k)+"("+itos(t)+")"); 
      }
    }

    // contrainte de capacite des liens
    for (l=0; l<nbLinks_; l++) {
      link_id = link_[l].getId();
      for (t=0; t<nbSlots_; t++) {
        lhsExpr = 0;
        cmpt = 0;
	Delta = slotSize_[t];
        for (j=0; j<nbFlows_; j++) {
 	 my_pair = flowMap_[j];
 	 k = my_pair.first;
 	 i = my_pair.second;
 	 if ( coflow_[k].useLink(i,link_id) == false )
 	   continue;
	 size = coflow_[k].getFlowSize(i);
 	 lhsExpr += size*f[j][t];
 	 cmpt++;
        }
        if ( cmpt>0 )
 	 model.addConstr(lhsExpr, GRB_LESS_EQUAL, Delta*link_[l].getCapa(),  "capa_"+itos(link_id)+"("+itos(t)+")"); 
      }
    }

    // contrainte definissant les y
    for (j=0; j<nbFlows_; j++) {
      my_pair = flowMap_[j];
      k = my_pair.first;
      i = my_pair.second;
      for (t=0; t<nbSlots_; t++) {
        lhsExpr = 0;
	cmpt = 0;
	for (t2=t; t2<nbSlots_; t2++) {
	  lhsExpr += f[j][t2]-y[k][t2];
	  cmpt++;
	}
	if ( cmpt>0 )
	  model.addConstr(lhsExpr, GRB_LESS_EQUAL, 0.0,  "fy_"+itos(j)+"("+itos(t)+")"); 
      }
    }

    // contrainte definissant les gamma
    for (k=0; k<nbCoflows_; k++) {
      n = coflow_[k].getNbFlow();
      for (l=0; l<nbLinks_; l++) {
	link_id = link_[l].getId();
	for (t=0; t<nbSlots_; t++) {
	  Delta = slotSize_[t];
	  lhsExpr = 0;
	  cmpt = 0;
	  for (i=0; i<n; i++) {
	    if ( coflow_[k].useLink(i,link_id) == false )
	      continue;
	    size = coflow_[k].getFlowSize(i);
	    my_pair = make_pair(k,i);
	    j = reverseMap[my_pair];
	    lhsExpr += size*f[j][t];
	    cmpt++;
	  }
	  if ( cmpt > 0 ) {
	    lhsExpr -= (1.0-gamma[k][t])*Delta*link_[l].getCapa();
	    model.addConstr(lhsExpr, GRB_LESS_EQUAL, 0.0,  "residu_"+itos(k)+"_"+itos(link_id)+"("+itos(t)+")"); 
	  }
	} // fin for t
      } // fin for l
    } // fin for k
    
 }





//----------------------------------------------------------------//
// Public Method:  write_utility_var                              //
//----------------------------------------------------------------//

 void Network::write_utility_var(GRBModel & model, GRBVar *u)
 {
   u[0] = model.addVar(0.0, 1.0, 0.0, GRB_CONTINUOUS, "U_1");
   u[1] = model.addVar(0.0, 1.0, 0.0, GRB_CONTINUOUS, "U_2");
 }



//----------------------------------------------------------------//
// Public Method:  write_objective_utilities                      //
//----------------------------------------------------------------//

void Network::write_objective_utilities(GRBModel & model, GRBVar *u,
					int nb_points, double *pwl_x, double *pwl_y)
 {
   model.setPWLObj(u[0], nb_points+1, pwl_x, pwl_y);
   model.setPWLObj(u[1], nb_points+1, pwl_x, pwl_y);

   if ( debug_ )
     cerr << "Utilites fixees" << endl;
 }


//----------------------------------------------------------------//
// Public Method: write_utility_cstr                              //
//----------------------------------------------------------------//

void Network::write_utility_cstr(GRBModel & model, GRBVar *u, GRBVar *z, int n1, int n2)
 {
    GRBLinExpr               lhsExpr1=0;
    GRBLinExpr               lhsExpr2=0;
    int                      c;
    int                      k;

    //Contrainte sur l'utilite de la classe 1
    for (k=0; k<nbCoflows_; k++) {
      c = coflow_[k].getClassId();
      if ( c != 1 ) continue;
      lhsExpr1 += z[k];
    }
    if ( n1 > 0 )
      lhsExpr1 /= n1;
    lhsExpr1 -= u[0];
    model.addConstr(lhsExpr1, GRB_EQUAL, 0,  "CSTR_U1");

    //Contrainte sur l'utilite de la classe 2
    for (k=0; k<nbCoflows_; k++) {
      c = coflow_[k].getClassId();
      if ( c == 1 ) continue;
      lhsExpr2 += z[k];
    }
    if ( n2 > 0 )
      lhsExpr2 /= n2;
    lhsExpr2 -= u[1];
    model.addConstr(lhsExpr2, GRB_EQUAL, 0,  "CSTR_U2");
    
 }
   


#endif








//----------------------------------------------------------------//
// Public Method:      total_transmission_time                    //
//----------------------------------------------------------------//

double Network::total_transmission_time(int l, int k , bool RealFlowSizes ) {
    
    int j=link_[l].getId();
    
    double u ;

   if ( RealFlowSizes )
      u= coflow_[k].loadOnLink(j)/link_[l].getCapa();
   else 
     {
       u= coflow_[k].loadOnLinkPred(j)/link_[l].getCapa();
     }
  

  
  return u ; 
}

//----------------------------------------------------------------//
// Public Method:      largest_wpt                                //
//----------------------------------------------------------------//

int Network::largest_wpt(int b, set<int> &S, set<int> & S_b, map<int, double> & weights, bool RealFlowSizes ) {

  set<int>::iterator  it = S.begin();
  int                 k, arg_min;
  double              min_wpt = INFTY;
  double              weighted_proc_time, w , p;
  
  // initialisation                                                                                                                                                                                        
  S_b.clear();
  arg_min = -1;

  // Parcourt des coflows                                                                                                                                                                                  
  for (; it != S.end(); it++) {
     k = idMap_[*it];

     // on saute les coflows n'utilisant pas le bottleneck                                                                                                                                                  
     if ( !coflow_[k].useLink(link_[b].getId() ) ) continue;

     //on insere tous les autres coflows dans S_b                                                                                                                                                           
     S_b.insert( *it );

     //  on calcule  weighted proc.time pour coflow k 
     w = weights[k];
     p = total_transmission_time(b, k, RealFlowSizes);
     weighted_proc_time= w/p;
     if (debug_ )
       cerr << "\tCoflow " << *it << " : w=" << w << ", p=" << p
	    << " =>  Weighted proc. time on link " << link_[b].getId()
	    << " is " << weighted_proc_time << endl;
     
    //on determine  le  Coflow with largest weighted proc. time                                                                                                                          
     if ( weighted_proc_time < min_wpt ) {
      min_wpt = weighted_proc_time;
      arg_min =k ;
      } // end if                                                                                                                                                                                            
  } // end for                                                                                                                                                                                             

  return arg_min;
}

//----------------------------------------------------------------//
// Public Method:      primal_variable                            //
//----------------------------------------------------------------//

double Network::primal_variable(int b, set<int> & S) {

  set<int>::iterator  it = S.begin();
  int                 k;
  double              sum; 
 
  // initialisation                                                                                                                                                                                        
  sum=0;
  
  // Parcourt des coflows                                                                                                                                                                                  
  for (; it != S.end(); it++) {
     k = idMap_[*it];
   
    //  sum of total_transmission_time
    sum+=total_transmission_time(b,k,1);
  } // end for                                                                                                                                                                                             

  return sum;
}


//----------------------------------------------------------------//
// Public Method:     update_sincroniaCoflows_weights             //
//----------------------------------------------------------------//



void Network::update_sincroniaCoflows_weights(set<int> S, map<int, double>  &weights , int b, int IK , bool RealFlowSizes)
{

    set<int>::iterator  it = S.begin();
    int                 k; 
    int                 id_bottleneck=link_[b].getId();
    double              p_star = total_transmission_time(b, IK, RealFlowSizes);
    double              p;

    // Parcourt des coflows 
    if ( debug_ )
      cerr << "Sincronia: update weights - ";
    for (; it != S.end(); it++) {
      k = idMap_[*it];
      if ( !coflow_[k].useLink(id_bottleneck) ) continue;
      if (k != IK) {
	p = total_transmission_time(b, k, RealFlowSizes);
        weights[k]= weights[k]- weights[IK]*p/p_star;
	if ( debug_ )
	  cerr << "w[" << *it << "]=" << weights[k] << "  ";
      }
    }
    weights[IK]=0.0;
    if ( debug_ )
      cerr << endl;
}      
       
       

//----------------------------------------------------------------//
// Public Method:     Sincronia                       //
//----------------------------------------------------------------//


void Network::Sincronia(double & cost, bool RealFlowSizes ) {  

  set<int>         S;
  double           max_end_time=0;
  map<int, double> weights;
  int              K=0, b,t, k; 
  set<int>         S_b ;
  int              i, j, w, n, c,m;
  double           d;

  //initialisation
  reset();
  sigma_.clear();
  cost = 0.0;

  
  //affichage
  if ( debug_ ) {
    if ( RealFlowSizes )
      cerr << "Sincronia is ran on real flow sizes" << endl;
    else
      cerr << "Sincronia is ran on predicted flow sizes" << endl;
  }

  
  //populate the set S with all coflows and initialize weights
  S.clear(); 
  for (k=0; k<nbCoflows_; k++) {
    S.insert(coflow_[k].getId());
    weights[k]= coflow_[k].getWeight();
  }
  n=S.size();
  
  //  algorithm
  for(t=n; t>=1; t--) {   
    if ( debug_ )
      cerr << "Sinronia: iteration " << t << endl;
    
    // find the bottleneck port
    b= bottleneck_link(S, max_end_time, RealFlowSizes) ;
    if ( debug_ )
      cerr << "Sinronia: bottleneck link is " << link_[b].getId() << endl;
      
    // find the coflow K with the largest weighted proc. time
    K=largest_wpt(b, S, S_b, weights, RealFlowSizes); 
    if ( debug_ )
      cerr << "Sinronia: scheduled coflow is coflow " << coflow_[K].getId() << endl;
    
    // Set primal variable
    coflow_[K].setCT(max_end_time);                              

    // Update cost
    if ( debug_ )
      cerr << "Sinrconia : increase cost with w=" << coflow_[K].getWeight() << " and CCT=" << max_end_time << endl;
    cost += coflow_[K].getWeight()*max_end_time;
    
    // Update weights
    update_sincroniaCoflows_weights(S, weights, b, K, RealFlowSizes);            
   
    // Remove K from the set of unscheduled coflows
    j=coflow_[K].getId();
    sigma_.push_front(j);
    S.erase(j);     
  } 
    // debug_ = true;
  //mise a jour des priorites
  if ( debug_ )
    cerr << "Sincronia : sigma=[";
  list<int>::iterator os = sigma_.begin();
  m = 0;
  for ( ; os != sigma_.end(); ) {
    k = idMap_[*os];  
    for (i=0; i<coflow_[k].getNbFlow(); i++) 
      coflow_[k].setFlowPriority(i,m);
    m++;   
    if ( debug_ )
        cerr << *os;
    os++;
      if ( debug_ ){ 
      if ( os != sigma_.end() )
	      cerr << ", ";
      else
	      cerr << "]" << endl; }
    
  }
  // debug_= false;
  //  cost = sigma_.size(); 
}


//----------------------------------------------------------------//
// Public Method:     Sincronia with released time                //
//----------------------------------------------------------------//


void Network::Sincronia_released_time( double & cost,bool RealFlowSizes ) {  
  set<int>                      S;
  double                        max_end_time=0;
  int                           K=0, b,t, k, id_coflow; 
  // set<int>                      S_b ;
  // int                           i, j, w, n, c,m;
  int                           n, id_link,j;
  double                        d;
  const float                   kappa = 0.5;
  map<pair<int, int>, float>    alpha;
  map<pair<int, set<int>>, float> beta;
  vector<float>                 Ll(nbLinks_, 0.0);
  map<pair<int, int>, float>    Llk;
  vector< vector<int> >         Subsets ;
  map<int, double>              weights;
  cost = 0.0;
  //initialisation
  reset();
  sigma_.clear();
  // cost = 0.0;

  // debug_ = true;
  //affichage
  if ( debug_ ) {
    if ( RealFlowSizes )
      cerr << "Sincronia is ran on real flow sizes" << endl;
    else
      cerr << "Sincronia is ran on predicted flow sizes" << endl;
  }

  //populate the set S with all coflows 
  S.clear(); 
  for (k=0; k<nbCoflows_; k++) {
    S.insert(coflow_[k].getId());
  }
  n=S.size();
  
  
  //  Initialisation des variables duales
  for (int l = 0; l < nbLinks_; ++l) {
    for (k=0; k<nbCoflows_; k++) {
      alpha[{l,k}] = 0;
    }
    
    for (int s : S) {
      set<int> Ss = {s};
      beta[make_pair(l,Ss)] = 0;
    }
  }
  // exit(-1);
  // Les processing times
  for (int l = 0; l < nbLinks_; ++l) {
    id_link=link_[l].getId();
    for (int k: S) {
      Llk[{l,k}]= total_transmission_time( id_link, k, RealFlowSizes);
      Ll[l] += Llk[{l,k}];
    }
    
  }
  
  //  algorithm
  for(t=n; t>=1; t--) {   
    if ( debug_ )
      cerr << "Sinronia: iteration " << t << endl;

    // find the bottleneck port
    b= bottleneck_link(S, max_end_time, RealFlowSizes) ;
    if ( debug_ )
      cerr << "Sinronia: bottleneck link is " << link_[b].getId() << endl;

    K = last_coflow_released(S);
    if ( debug_ )
      cerr << "Sinronia: last released coflow " << K << endl;

    if(coflow_[K].getStartTime() > kappa * Ll[b]){ //first case
      if ( debug_ )
        cerr << "Sinronia: \tCase 1: " << endl;
      double sum_beta = 0.0;
      for (int l = 0; l < nbLinks_; ++l) {
          id_link=link_[l].getId();
          for (int s : S) {
            set<int> Ss = {s};
            // sum_beta += total_transmission_time( id_link, s, RealFlowSizes) * beta[make_pair(l, Ss)]; // sum_(l in L){ sum_(k in S){ p_(l,k)* beta_(l,S)}}
            sum_beta += Llk[make_pair(l,s)] * beta[make_pair(l, Ss)]; // sum_(l in L){ sum_(k in S){ p_(l,k)* beta_(l,S)}}
          }
      }

      alpha[{b, K}] = coflow_[K].getWeight() - sum_beta; // saturation alpha
      id_coflow=coflow_[K].getId();
      if ( debug_ )
        cerr << "\t\tId coflow = " << id_coflow << endl;
      
    }
    else{  //Second case
      if ( debug_ )
        cerr << "Sinronia: \tCase 2: " << endl;
      double min_ratio = numeric_limits<float>::infinity();
      int K_ = -1; // other coflow 

      for(int j : S){ // argmin on set of coflow
        double sum_beta = 0.0;
        for (int l = 0; l < nbLinks_; ++l) {
          id_link=link_[l].getId();
          for (int s : S) {
            set<int> Ss = {s};
            sum_beta += Llk[make_pair(l,j)]* beta[make_pair(l, Ss)]; // sum_(l in L){ sum_(k in S){ p_(l,k)* beta_(l,S)}}
          }
        }
        double numerateur = coflow_[K].getWeight() - sum_beta;

        double denominateur = Llk[make_pair(b,j)]; //total_transmission_time( link_[b].getId(), j, RealFlowSizes);
        denominateur = max(denominateur, 1e-6); // éviter division par zéro

        float ratio = numerateur / denominateur;
        if ( debug_ )
          cerr << "\t\t\t pente( "<< j << ") = "<< ratio << endl;
        if (ratio < min_ratio){
          min_ratio = ratio ; 
          K_ = j;
        }
      }

      if(K_ !=-1){
        if ( debug_ )
          cerr << "\t\t\t j' = " << K_ << endl;
        float sum_beta = 0.0;
        for (int l = 0; l < nbLinks_; ++l) {
          id_link=link_[l].getId();
          for (int s : S) {
              set<int> Ss = {s};
              sum_beta += Llk[make_pair(l,K_)]* beta[make_pair(l, Ss)]; //total_transmission_time( id_link, K_, RealFlowSizes) * beta[make_pair(l, Ss)];
          }
        }

        beta[make_pair(b,S)] = (coflow_[K_].getWeight() - sum_beta) / max(total_transmission_time( b, K_, RealFlowSizes), 1e-6);
        
        // for (int s : S) {// Update all beta of each single set of S
        //   set<int> Ss = {s};
        //   beta[make_pair(b,Ss)] = (coflow_[K_].getWeight() - sum_beta) / max(total_transmission_time( b, K_, RealFlowSizes), 1e-6);
        // }

        if ( debug_ )
          cerr << "\t\t\t beta["<<b <<", S]=" << beta[make_pair(b,S)] << endl;

        id_coflow=coflow_[K_].getId();
        if ( debug_ )
          cerr << "\t\tId coflow = " << id_coflow << endl;
      }  
      
    }

    sigma_.push_front(id_coflow);
    S.erase(id_coflow); //erasing from the coflow set

    for (int l = 0; l < nbLinks_; ++l) {
      id_link=link_[l].getId();
      Ll[l] -= Llk[{l,id_coflow}];//total_transmission_time( id_link, id_coflow, RealFlowSizes);
    }
    
        // exit(-1);
  } 
  // debug_ = true;
  //mise a jour des priorites
  if ( debug_ )
    cerr << "Sincronia : sigma=[";
  list<int>::iterator os = sigma_.begin();
  int m = 0;
  for ( ; os != sigma_.end(); ) {
    k = idMap_[*os];  
    for (int i=0; i<coflow_[k].getNbFlow(); i++) 
      coflow_[k].setFlowPriority(i,m);
    m++;   
    if ( debug_ )
        cerr << *os;
    os++;
      if ( debug_ ){ 
      if ( os != sigma_.end() )
	      cerr << ", ";
      else
	      cerr << "]" << endl; }
    
  }
  // debug_ = false;
}

//----------------------------------------------------------------//
// Public Method:     last_coflow_released                       //
//----------------------------------------------------------------//

int Network::last_coflow_released(set<int>  S) {  
  int j = 0;
  int r = 0;

  for (int k : S) {
    if (coflow_[k].getStartTime() > r) {
      r = coflow_[k].getStartTime();
      j = k;
    }
  }
  return j;
}



//----------------------------------------------------------------//
// Public Method:     average_proc_time                      //
//----------------------------------------------------------------//

float Network::average_proc_time() {  
  float  sum_p_lk = 0.0;

  for (int k = 0; k < nbCoflows_; k++) {
    float sum_ = 0.0;
    for( int j=0; j< coflow_[k].getNbFlow(); j++){
      sum_ += coflow_[k].getFlowSize(j);
    }
    sum_p_lk += sum_ / coflow_[k].getNbFlow();
  } 
  return sum_p_lk / nbCoflows_;
}



//----------------------------------------------------------------//
// Public Method:     RandomSigma                       //
//----------------------------------------------------------------//

void Network::RandomSigma(double & cost ) {  


  int k ;
  vector<int> myVector;
  //initialisation
  random_device rd;
  mt19937 g(rd());
  reset();
  sigma_.clear();

  //affichage
  if ( debug_ ) {
   
      cerr << "RandomSigma is ran on predicted flow sizes" << endl;
  }


  
  for (int k=0; k<nbCoflows_; k++) {
    myVector.push_back(coflow_[k].getId());
    coflow_[k].setCT(0);  
  }

  shuffle (myVector.begin(), myVector.end(), g);
  
   vector<int>::iterator it = myVector.begin();

  for ( ; it != myVector.end(); ) {   
    sigma_.push_back(*it) ; 
    it++;}
    
 //mise a jour des priorites
  if ( debug_ ) 
    cerr << "RandomSigma : sigma=[";
  list<int>::iterator os = sigma_.begin();
  int m = 0;
  for ( ; os != sigma_.end(); ) {
    k = idMap_[*os];  
    for (int i=0; i<coflow_[k].getNbFlow(); i++) 
      coflow_[k].setFlowPriority(i,m);
    m++;   
    if ( debug_ )
      cerr << *os;
    os++;
    if ( debug_ ) {
      if ( os != sigma_.end() )
	cerr << ", ";
      else
	cerr << "]" << endl;
    }
  } 
  
  cost = sigma_.size(); 
}





    
//----------------------------------------------------------------//
// Public Method:     schedule                                    //
//----------------------------------------------------------------//

int Network::schedule(double & cost, Network::Algorithm algo, bool RealFlowSizes ) {
  
  switch( algo )
    {
    case Network::LP :
      if ( debug_ )
	cerr << "EXECUTION OF LP-BASED ALGORITHM" << endl;
      optimal_solution(cost);
      return 0;

    case Network::LP_LB :
      if ( debug_ )
	cerr << "EXECUTION OF LP-BASED LOWER BOUND" << endl;
#ifdef GUROBI  
      GSolve(cost) ;
#endif
      return 1;

    case Network::SINCRONIA :
      if ( debug_ )
	cerr << "EXECUTION OF SINCRONIA ALGORITHM" << endl;
      Sincronia(cost, RealFlowSizes);
      return 2;

    case Network::SINCRONIARelease :
      if ( debug_ )
	cerr << "EXECUTION OF SINCRONIA ALGORITHM WITH RELEASED TIME" << endl;
      Sincronia_released_time(cost, RealFlowSizes);
      return 3;

    case Network::RR :
      if ( debug_ )
	cerr << "EXECUTION OF RR ALGORITHM" << endl;
      roundRobin(cost);
      return 4;

    case Network::RANDOMSigma  :
      if ( debug_ )
	cerr << "EXECUTION OF RandomSigma ALGORITHM" << endl;
      RandomSigma(cost);
      return 5;
      
    default :
      cerr << "Unknown algorithm " << endl;
      exit(-1);
    }
}
 
//----------------------------------------------------------------//
// Public Method:     MUWP                                     //
//----------------------------------------------------------------//

int Network::selection(int alpha,double mu_max,int deadline ,list<int>& selection_list, bool RealFlowSizes) {
 
  int solver = 1; //0:  grb; 1: clp
   if ( debug_ )
	cerr << "EXECUTION OF MUWP" << endl;
  if (solver == 0){
    #ifdef GUROBI  
        selection_list = MUWPSolve(alpha, mu_max,deadline,RealFlowSizes) ;
    #endif
  }
  else if (solver == 1)
  {
    selection_list = MUWPSolve_CLP(alpha, mu_max,deadline,RealFlowSizes) ;
  }
  else {
    cerr <<"Error solver" <<endl;
    exit(-1);
  }
  
return 1;
}
    
//----------------------------------------------------------------//
// Public Method: reset                                           //
//----------------------------------------------------------------//

void Network::reset() {
  int  k, i;
  
   
  for (k=0; k<nbCoflows_; k++)
    coflow_[k].reset();

  for (i=0; i<nbLinks_; i++) {
    link_[i].setReleaseDate(0.0);
    link_[i].setCompletionTime(0.0);
    link_[i].release();
  }
}



//----------------------------------------------------------------//
// Public Method:       generateSubsets                           //
//----------------------------------------------------------------//

void Network:: generateSubsets(set<int>& nums, set<int>::iterator start, vector<int>& subset, vector< vector<int> > &result) {
    // Add the current subset to the result (excluding the empty subset)
    if (!subset.empty()) {
        result.push_back(subset);
	// cerr << "Add subset (";
	// for (int j = 0; j < subset.size(); j++)
	//   cerr << subset[j] << " ";
	// cerr << ")" << endl;
    }

    // Generate subsets recursively
    for (auto it = start; it != nums.end(); it++) {
        // Add the current element to the subset
        subset.push_back(*it);

        // Generate subsets starting from the next element
        generateSubsets(nums, next(it), subset, result);

        // Remove the current element from the subset
        subset.pop_back();
    }
}


//----------------------------------------------------------------//
// Public Method:       subsets                           //
//----------------------------------------------------------------//

vector< vector<int> > Network::subsets(set<int>& nums) {
    vector< vector<int> > result;
    vector<int>           subset;

    // Generate all possible subsets
    generateSubsets(nums, nums.begin(), subset, result);

    return result;
}


#ifdef GUROBI

//----------------------------------------------------------------//
// Public Method:       GSolve                                     //
//----------------------------------------------------------------//
void Network::GSolve(double & cost) {

  int                 i, j, k, t;
  int                 status;
  GRBConstr         * cstr = 0;

  try {
    GRBEnv env = GRBEnv("/tmp/x.log");
    GRBModel model = GRBModel(env);

    //     create variable
    GRBVar      *z = new GRBVar[nbCoflows_];
   
    //limites de temps et sur la precision pour la resolution du modele
    model.getEnv().set(GRB_IntParam_OutputFlag, 0);
    //    model.getEnv().set(GRB_DoubleParam_TimeLimit, GRB_TIME_LIMIT);
    //    model.getEnv().set(GRB_IntParam_MIPFocus, 1);
    //    model.getEnv().set(GRB_DoubleParam_MIPGap, 0.000001);
    
    //pas d'output
    model.getEnv().set(GRB_IntParam_OutputFlag, 0);
    
    //Mise a jour du modele
    model.update();
    if ( debug_ )
      cerr << "Modele Gurobi mis a jour" << endl;
    
    //Fonction objectif
    obj_function(model, z);
    
    //Ajout des contraintes de conservation des flots
    write_cstr(model,z);

    if ( debug_ )
      cerr << "Contraintes ecrites" << endl;
    
    //Mise a jour du modele
    model.update();
    if ( debug_ )
      cerr << "Modele Gurobi mis a jour" << endl;
    
    //Ecriture du modele dans un fichier
    if (debug_ )
      model.write("/tmp/prob.lp");
    
    //Optimisation du modele
    //    cerr << "Lancement de l'optimisation avec Gurobi" << endl;
    model.optimize();
    status = model.get(GRB_IntAttr_Status);
    if (status == GRB_UNBOUNDED)
      {
   	cerr << "The model cannot be solved "
   	     << "because it is unbounded" << endl;
	exit(-1);
      }
    else if (status == GRB_INFEASIBLE)
      {
   	cerr << "The model cannot be solved "
   	     << "because there is no feasible solution" << endl;

   	// do IIS
   	cout << "The model is infeasible; computing IIS" << endl;
   	model.computeIIS();
   	cout << "\nThe following constraint(s) "
   	     << "cannot be satisfied:" << endl;
   	cstr = model.getConstrs();
   	for (int i = 0; i < model.get(GRB_IntAttr_NumConstrs); ++i)
   	  {
   	    if (cstr[i].get(GRB_IntAttr_IISConstr) == 1)
   	      {
   		cout << cstr[i].get(GRB_StringAttr_ConstrName) << endl;
   	      }
   	  }
	exit(-2);
      }
    else
      {
   	double  tmp;

	if ( debug_ ) { 
	  cost = model.get(GRB_DoubleAttr_ObjVal);
   	  cerr << "Runtime : " << model.get(GRB_DoubleAttr_Runtime) << endl;
	  cerr << "GUROBI: Sum of CCT is " << cost << endl;
	}
   	cost = 0.0;

   	for (k=0; k<nbCoflows_; k++) {
   	  tmp = z[k].get(GRB_DoubleAttr_X);
	  if ( debug_ )
	    cerr << "Coflow " << k << " completes at time " << tmp << endl;
	  cost+=tmp;
	  coflow_[k].setCCT(tmp);	  
   	}
      }
    delete [] z;    
  } catch(GRBException e) {
    cerr << "Error code = " << e.getErrorCode() << endl;
    cerr << e.getMessage() << endl;
  } catch(...) {
    cerr << "Exception during optimization" << endl;
  }
}



//----------------------------------------------------------------//
// Public Method: obj_function                       //
//----------------------------------------------------------------//

 void Network::obj_function(GRBModel & model, GRBVar *z)
 {
   int          k;
   double       w;
   GRBLinExpr   obj=0;

     for (k=0; k <nbCoflows_; k++) {
     string cof_name = itos(k);
     
      z[k] = model.addVar(0.0, GRB_INFINITY, 0, GRB_CONTINUOUS, "z_"+cof_name);
      obj += coflow_[k].getWeight()*z[k];
   }
   model.setObjective(obj,GRB_MINIMIZE);

   if ( debug_ )
     cerr << "Objectif d'optimisation fixe" << endl;
 }






//----------------------------------------------------------------//
// Public Method: write_cstr                                 //
//----------------------------------------------------------------//

 void Network::write_cstr(GRBModel & model, GRBVar *z)
 {
    GRBLinExpr               lhsExpr;
    int                      t, k, l, id_link;
    double                   F1=0;
    double                   F2=0;
    double                   F = 0;
    set<int>                 S;
    vector< vector<int> >    Subsets ;
 
   
    //contrainte de conservation du flot
  
  
      for (int i = 0; i < nbCoflows_; i++) {
      S.insert(i);
      }
     Subsets = subsets(S);
     for (l=0; l<nbLinks_; l++) {
       id_link=link_[l].getId();
       for (int j = 0; j < Subsets.size(); j++) {
        F1=0;
        F2=0;
        lhsExpr=0;
        for (int k = 0; k< Subsets[j].size(); k++) {
          F1+= pow(total_transmission_time( id_link, Subsets[j][k], false),2);
          F2+=total_transmission_time( id_link, Subsets[j][k], false) ;
          lhsExpr +=z[Subsets[j][k]]*total_transmission_time(id_link, Subsets[j][k],false);
        } 
        F = 0.5*(F1+pow(F2,2));
        if ( F > 0 )
          model.addConstr(lhsExpr, GRB_GREATER_EQUAL, F,  "Par_Ineq_l_"+itos(id_link)+"_S_"+itos(j));
            }
          }
 }


//----------------------------------------------------------------//
// Public Method:       MUWPSolve                                     //
//----------------------------------------------------------------//
list<int> Network::MUWPSolve(int alpha,double mu_max,int D, bool RealFlowSizes) {
  GRBConstr         * cstr = 0;
  list<int> selected_coflows; // Ensemble S à retourner

  bool muwp_cos = true; // Check for COS or CS MUWP solver
  // debug_ = true;
  try
  {
    // GRBEnv env = GRBEnv("/tmp/x.log");

    // GRBModel model = GRBModel(env);

    // ------- To stop printing from terminal
    int saved_stdout = dup(fileno(stdout));
    int saved_stderr = dup(fileno(stderr));

    // Redirect stdout and stderr to /dev/null
    freopen("/dev/null", "w", stdout);
    freopen("/dev/null", "w", stderr);

    // Initialize Gurobi environment (this prints the license message)
    GRBEnv env = GRBEnv("/tmp/x.log");
    GRBModel model = GRBModel(env);

    // Restore original stdout/stderr
    fflush(stdout);
    fflush(stderr);
    dup2(saved_stdout, fileno(stdout));
    dup2(saved_stderr, fileno(stderr));
    // ------- To stop printing from terminal

    // Variables binaires y_k : 1 si coflow k est sélectionné, 0 sinon
    GRBVar* y = new GRBVar[nbCoflows_];
    for (int k = 0; k < nbCoflows_; ++k) {
      if (muwp_cos) 
        y[k] = model.addVar(0.0, 1.0, 0.0, GRB_CONTINUOUS, "y_" + itos(k));  // y_k ∈ [0,1] 
      else 
        y[k] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY, "y_" + itos(k));  // y_k ∈ {0,1} 
    }
     
      


    //limites de temps et sur la precision pour la resolution du modele
    model.getEnv().set(GRB_IntParam_OutputFlag, 0);
    
    //pas d'output
    model.getEnv().set(GRB_IntParam_OutputFlag, 0);

    //Fonction objectif
    obj_function_muwp(model, y);

    //Ajout des contraintes de conservation des flots
    
    if (muwp_cos)
      write_cstr_cos_muwp(model,y,alpha, mu_max, D, RealFlowSizes); // MUWP for COS
    else
      write_cstr_muwp(model,y,alpha, mu_max,D, RealFlowSizes);  // MUWP for CS ------not correct

    if ( debug_ )
      cerr << "Contraintes ecrites" << endl;
    
    //Mise a jour du modele
    model.update();
    if ( debug_ )
      cerr << "Modele Gurobi mis a jour" << endl;
    
    //Ecriture du modele dans un fichier
    if (debug_ )
      model.write("/tmp/prob.lp");
    
    //Optimisation du modele
    model.optimize();
    if ( debug_ )
      cerr << "Modele Gurobi optimisé" << endl;

  int status = model.get(GRB_IntAttr_Status);
  // cerr << "status " << status << endl;
  // cerr << "val GRB_OPTIMAL " << GRB_OPTIMAL << endl;

    
    if (status == GRB_OPTIMAL) {
      cerr <<"GRB: func val= " << model.get(GRB_DoubleAttr_ObjVal)<< endl;
      for (int k = 0; k < nbCoflows_; ++k){
        cerr << "  sol["<< k <<"]= " << y[k].get(GRB_DoubleAttr_X) << endl;
        double s= getAllFlowSize(k, RealFlowSizes);
        // cerr << "\t\tcoflow " << coflow_[k].getId()<<" sum flows size " << s << " value of opt: " << y[k].get(GRB_DoubleAttr_X) << endl;
        if (muwp_cos){
          if (y[k].get(GRB_DoubleAttr_X) >= 0.5){
            selected_coflows.push_back(coflow_[k].getId());
            // cerr << "\t\tcoflow " << coflow_[k].getId()<< endl;
        }
        }
        else{
          if (y[k].get(GRB_DoubleAttr_X) >= 1){
          selected_coflows.push_back(coflow_[k].getId());
        }
        }
        
          
      }
    } 
    else if (status == GRB_INFEASIBLE) {
      cerr << "Modèle infaisable. Calcul de l’IIS..." << endl;
      model.computeIIS();
      GRBConstr* cstr = model.getConstrs();
      for (int i = 0; i < model.get(GRB_IntAttr_NumConstrs); ++i) {
          if (cstr[i].get(GRB_IntAttr_IISConstr) == 1) {
              cerr << "Contrainte conflictuelle : " << cstr[i].get(GRB_StringAttr_ConstrName) << endl;
          }
      }
    }
    delete[] y;
  } catch (GRBException& e) {
    cerr << "Erreur GRB " << e.getErrorCode() << ": " << e.getMessage() << endl;
  }
  // debug_ = false;
  return selected_coflows;
}

double Network::getAllFlowSize(int k, bool RealFlowSizes){
  double sum =0.0;
  for (int j=0; j< coflow_[k].getNbFlow(); j++){
    if (RealFlowSizes)
      sum += coflow_[k].getFlowSize(j);
    else 
      sum += coflow_[k].getFlowPredSize(j);
  }
  return sum ;
}
//----------------------------------------------------------------//
// Public Method: Objectif Function of MUWP                                  //
//----------------------------------------------------------------//

void Network::obj_function_muwp(GRBModel & model, GRBVar *y)
 {
   GRBLinExpr obj = 0;

    for (int k = 0; k < nbCoflows_; ++k) {
        obj += coflow_[k].getWeight() * (1 - y[k]); // minimize rejected coflows
        // obj += -coflow_[k].getWeight() *  y[k]; 
    }

    model.setObjective(obj, GRB_MINIMIZE);
    if (debug_)
        cerr << "Objectif: Minimiser la somme des coflows non selectionnés." << endl;
}

//----------------------------------------------------------------//
// Public Method: write_cstr_muwp                                 //
//----------------------------------------------------------------//

void Network::write_cstr_muwp(GRBModel &model, GRBVar *y,int alpha,double mu_max, int D, bool RealFlowSizes) {
    // Variables : x[k][j][t] = 1 si le flow j du coflow k est planifié au slot t
    GRBVar*** x = new GRBVar**[nbCoflows_];

    for (int k = 0; k < nbCoflows_; ++k) {
        int nf = coflow_[k].getNbFlow();
        x[k] = new GRBVar*[nf];

        for (int j = 0; j < nf; ++j) {
            x[k][j] = new GRBVar[D];  // D = deadline
            GRBLinExpr sum = 0;
            double vk_j ;
            if(RealFlowSizes)
              vk_j = coflow_[k].getFlowSize(j);  // v
            else
              vk_j = coflow_[k].getFlowPredSize(j); // v_hat
            // cerr << "flow  "<< coflow_[k].getFlowId(j) <<" vk_j: " << vk_j << endl;
            for (int t = 0; t < D; ++t) {  // [0, D[
                x[k][j][t] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY,"x_" + itos(k) + "_" + itos(j) + "_" + itos(t));
                
                // Contraintes : x_{k,j}^t ≤ y_k
                model.addConstr(x[k][j][t] <= y[k],
                 "x_leq_y_" + itos(k) + "_" + itos(j) + "_" + itos(t)); //un flow ne peut etre actif que si son coflow est actif
                
                sum += x[k][j][t];
            }
            // // Contrainte de conservation du flot : sum_t x_{k,j}^t >= v_{k,j}* y_k //et sum_t x_{k,j}^t +1 <= v_{k,j}* y_k
            model.addConstr(sum >= vk_j* y[k], 
            "flow_conserv_k_1st" + itos(k) + "_j" + itos(j)); //sum_t x[k][j][t] == v_kj * y[k];

            // model.addConstr(sum +1 <= vk_j* y[k], 
            // "flow_conserv_k_2nd" + itos(k) + "_j" + itos(j)); //sum_t x[k][j][t] +1 <= v_kj * y[k];
        }        
    }

    // Contraintes de capacité de lien : sum_{k,j} x_{k,j}^t ≤ 1  pour chaque lien et chaque slot t
    for (int t = 0; t < D; ++t) {
        for (int l = 0; l < nbLinks_; ++l) {
            GRBLinExpr load = 0;
            int val = 0;
            for (int k = 0; k < nbCoflows_; ++k) {
                int nf = coflow_[k].getNbFlow();
                
                for (int j = 0; j < nf; ++j) {
                  int path_len = coflow_[k].getLength(j);

                  for (int m = 0; m < path_len; ++m) {          
                    if (l == coflow_[k].getLink(j,m)) {  // à implémenter selon ta structure
                        load += x[k][j][t];
                        // val += x[k][j][t];
                        // cerr << "Lien " << l << ", t=" << t << ", charge estimée : " << val << endl;
                        break;  // Un lien ne peut apparaître qu'une fois dans un chemin
                    }
                  }
                }
            }
            model.addConstr(load <= 1.0, "cap_link_t" + itos(t) + "_l" + itos(l));
        }      
    }
    // Libération mémoire de x
    for (int k = 0; k < nbCoflows_; ++k) {
        int nf = coflow_[k].getNbFlow();
        for (int j = 0; j < nf; ++j) {
            delete[] x[k][j];
        }
        delete[] x[k];
    }
    delete[] x;

    // Contraintes de définition des variables : x_{k,j}^t ∈ {0,1} est géré automatiquement par addVar()
    if (debug_) cerr << "Contraintes de conservation et de capacité ajoutées." << endl;
}


//----------------------------------------------------------------//
// Public Method: write_cstr_muwp_cos                                 //
//----------------------------------------------------------------//

void Network::write_cstr_cos_muwp(GRBModel &model, GRBVar *y,int alpha,double mu_max, int D, bool RealFlowSizes) {
    int id_link;
    int prod = ceil(mu_max *D); // integer of mu_max* D

    for (int l = 0; l < nbLinks_; ++l) {
        id_link=link_[l].getId();

        GRBLinExpr load = 0;

        for (int k = 0; k < nbCoflows_; ++k) {
            double sum = total_transmission_time(id_link, k,RealFlowSizes);

            load += sum * y[k];  //  ajout à la somme totale sur le lien l
            
        }
        model.addConstr(load, GRB_LESS_EQUAL, prod, "cap_link_l" + itos(l)); //  mu_max* D  
    }
}





 #endif




//-----------------CLPSimplex  ----------------- //

//----------------------------------------------------------------//
// Public Method:       MUWPSolve_CLP                                     //
//----------------------------------------------------------------//

list<int> Network::MUWPSolve_CLP(int alpha, double mu_max, int D, bool RealFlowSizes) {
   list<int> selected_coflows;

    const int numCols = nbCoflows_;
    const int numRows = nbLinks_;

    int       id_link;

    vector<double> colLower(numCols, 0.0);
    vector<double> colUpper(numCols, 1.0);
    vector<double> objective(numCols);

    // Objective: minimize sum of w_k * (1 - y_k) <=> maximize sum w_k * y_k => minimize -w_k * y_k
    for (int k = 0; k < nbCoflows_; ++k) {
        objective[k] =  -coflow_[k].getWeight();
    }

    // Build sparse matrix in CCS -Compressed Column Storage- format
    vector<int> starts;
    vector<int> indices;
    vector<double> elements;
    starts.push_back(0);
    int count = 0;

    for (int k = 0; k < nbCoflows_; ++k) {
        for (int l = 0; l < nbLinks_; ++l) {
            id_link=link_[l].getId();
            double plk = total_transmission_time(id_link, k,RealFlowSizes);
            if (plk > 0) {                
                indices.push_back(l);
                elements.push_back(plk);
                count++;
            }
        }
        starts.push_back(count);
    }

    vector<double> rowLower(numRows, -1e20);

    int prod = ceil(mu_max *D); // integer of mu_max* D
    // cerr <<"Prod mu_max * D= " << prod << endl;
    vector<double> rowUpper(numRows, prod); // sum p_i,j <= mu_max* D

    ClpSimplex model;

    model.setPrimalTolerance(1e-5);  // plus strict que par défaut
    model.setDualTolerance(1e-5);


    model.setLogLevel(0); // Désactive tous les messages du solveur 
    model.loadProblem(numCols, numRows,
                      starts.data(), indices.data(), elements.data(),
                      colLower.data(), colUpper.data(), objective.data(),
                      rowLower.data(), rowUpper.data());

    model.writeLp("/tmp/clp_model.lp");

    int status = model.primal();
    if (status != 0) {
        std::cerr << "Erreur CLP: statut = " << status << std::endl;
        return selected_coflows;
    }

    const double EPSILON = 1e-6;

    const double* sol = model.primalColumnSolution();
    // cerr <<"CLP: func val= " << model.objectiveValue()<< endl;
    for (int k = 0; k < nbCoflows_; ++k) {
      // cerr << "  sol["<< coflow_[k].getId() <<"]= " << sol[k] << endl;
        if (sol[k] >= 0.5 - EPSILON) {
            selected_coflows.push_back(coflow_[k].getId());
            // cerr <<"\t\t selected coflow " << coflow_[k].getId() <<endl;
        }
    }

    return selected_coflows;
}

// Fonction auxiliaire pour calculer p_{l,k}
double Network::compute_p_lk(const Coflow& c, int link, bool RealFlowSizes) {
    double sum = 0.0;
    for (int j = 0; j < c.getNbFlow(); ++j) {
        for (int m = 0; m < c.getLength(j); ++m) {
            if (c.getLink(j, m) == link) {
                sum += RealFlowSizes ? c.getFlowSize(j) : c.getFlowPredSize(j);
                // break;
            }
        }
    }
    return sum;
}




//----------------------------------------------------------------//
// Public Method: set_flows_pred                                  //
//----------------------------------------------------------------//

void Network::set_flows_pred(double stdev) {

  double                      size;
  int                         nb_flows, l, d;

  if ( meanFlowSize_ < 0.0 ) {
    double                      m = 3369.05;

    for (int k=0 ; k<nbCoflows_;k++){
      for (int j=0 ; j< coflow_[k].getNbFlow();j++) {
	l = coflow_[k].getFlowDestination(j);
	nb_flows = coflow_[k].numberOfFlowsOnLink(l);
	size = m/nb_flows;
	coflow_[k].setFlowPredSize(j,size);      
	if ( debug_ )
	  cerr << "V[" << coflow_[k].getId() << "," << coflow_[k].getFlowId(j) << "]=" << size << endl;
      }
    }
  }
  else {
    for (int k=0 ; k<nbCoflows_;k++){
      for (int j=0 ; j< coflow_[k].getNbFlow();j++) {
	size = meanFlowSize_;
	coflow_[k].setFlowPredSize(j,size);      
	if ( debug_ )
	  cerr << "V[" << coflow_[k].getId() << "," << coflow_[k].getFlowId(j) << "]=" << size << endl;
      }
    }
  }
}

// From predictions code 
void Network::set_flows_pred_true(double delta,int s) {
  double predFlowVolume, size, noise;

  // random_device rd;
  // mt19937 gen(rd()); //random seed


  // unsigned int seed = 42;                // fix seed
  mt19937 gen(s); 
  
  //  normal_distribution<double> error_dist(mean_noise, stddev_noise);
  uniform_real_distribution<double> unif(1.0-delta,1.0+delta);
  double                            u;
  
  if ( debug_ )
    cerr << "\nPredicted flow sizes:" << endl;
  for (int k=0 ; k<nbCoflows_;k++){
    for (int j=0 ; j< coflow_[k].getNbFlow();j++) {
      size = coflow_[k].getFlowSize(j);
      predFlowVolume = 0.0;
      u = unif(gen);
      predFlowVolume = u * size;
      //uniform_real_distribution<> dis(i+1, i+100); 
       // predFlowVolume= dis(gen); 
      coflow_[k].setFlowPredSize(j,predFlowVolume);
      debug_ = true;
      if ( debug_ )
        cerr << "V[" << coflow_[k].getId() << "," << coflow_[k].getFlowId(j) << "]=" << predFlowVolume << endl;
      debug_ = false;
    }
  }
}
// // From predictions code 
// int tirage_geometrique(double p, mt19937& gen) {
//     std::uniform_real_distribution<> dis(0.0, 1.0);
//     int count = 1;
//     while (dis(gen) > p) {
//         count++;
//     }
//     return count;
// }
// void Network::set_flows_pred_geo(double p) {
//   random_device rd;
//   mt19937 gen(rd());
//   if ( debug_ )
//     cerr << "\nPredicted flow sizes:" << endl;

//   for (int k=0 ; k<nbCoflows_;k++){
//     for (int j=0 ; j< coflow_[k].getNbFlow();j++) {

//       // Tirage géométrique centré et normalisé 
//       double esperance = 1.0 / p;
//       double variance = (1 - p) / (p * p);
//       double ecart_type = sqrt(variance);

//       int tirage = tirage_geometrique(p);
//       double z = (tirage - esperance) / ecart_type;

//       double predFlowVolume = z * coflow_[k].getFlowSize(j);
      
//       coflow_[k].setFlowPredSize(j,predFlowVolume);

//       cerr << "V[" << coflow_[k].getId() << "," << coflow_[k].getFlowId(j) << "]=" <<"real size= "<< coflow_[k].getFlowSize(j)<<" , predicted size= "<< predFlowVolume << endl;
//       if ( debug_ )
// 	cerr << "V[" << coflow_[k].getId() << "," << coflow_[k].getFlowId(j) << "]=" << predFlowVolume << endl;
//     }
//   }
// }


//----------------------------------------------------------------//
// Public Method: roundRobin                                      //
//----------------------------------------------------------------//

void Network::roundRobin(double & cost) {
  
  for (int k=0 ; k<nbCoflows_;k++){
    for (int i=0 ; i<coflow_[k].getNbFlow(); i++) {
      coflow_[k].setFlowPriority(i,1);
    }
  }
  cost = 0.0;
}



