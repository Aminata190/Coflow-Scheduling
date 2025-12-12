#ifndef __Network_hxx__
#define __Network_hxx__


//****************************************************************//
//*                                                              *//
//*------------------- Includes SECTION -------------------------*//
//*                                                              *//
//****************************************************************//


#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <set>
#include <list>
#include <queue>
#include <cstdlib>

//#include <random>

#ifdef GUROBI
#include "gurobi_c++.h"
#endif


#include "Link.hxx"
#include "Flow.hxx"
#include "EventList.hxx"


using namespace std;



//****************************************************************//
//*                                                              *//
//------------ Public Data Structures SECTION --------------------//
//*                                                              *//
//****************************************************************//


#define MAX(x,y)      (((x)>(y))?(x):(y))
#define MIN(x,y)      (((x)>(y))?(y):(x))
#define GAMMA         0.7   // constante utilisee dans DCOFLOW V3




//****************************************************************//
//*                                                              *//
//----------------- Public Classes SECTION -----------------------//
//*                                                              *//
//****************************************************************//

//----------------------------------------------------------------//
// Class :      Network                                           //
//                                                                //
//                                                                //
// Description : classe representant le reseau sous la forme d'un //
// ensemble de data centres et de base stations                   //
//                                                                //
//----------------------------------------------------------------//


class Network
{
public :
  friend  ostream  &operator<<(ostream &, Network const &);

  enum Algorithm { LP=1, LPA, CS_MHA, CS_DP, DCOFLOW, SINCRONIA,SINCRONIARelease , LP_LB, RR, RANDOMSigma};

  
protected:
  int                      nbCoflows_;      // nombre de coflows
  double                   maxTime_;        // nombre de périodes de temps
  int                      nbLinks_;        // nombre de liens
  int                      nbFlows_;        // nombre de flots
  int                      nbSlots_;        // nombre d'intervalles temporels
  Link                   * link_;           // tableau des liens
  Coflow                 * coflow_;         // tableau des coflows
  double                 * slotSize_;       // Taille des intervalles temporels
  double                 * slotStart_;      // Date de debut du slot
  double                 * slotEnd_;        // Date de fin du slot
  list<int>                sigma_;          // Ordre d'execution des coflows 
  map<int, int>            idMap_;          // id coflow -> position dans le tableau coflow_
  map<int, pair<int,int> > flowMap_;        // correspondance num_flow <-> (id coflow,position flot)
  map<int, pair<int,int> > flowIdMap_;      // correspondance flow id -> (id coflow, position flot)
  map<int,int>             linkIdMap_;      // correspondance link id -> position dans tableau link_
  bool                     debug_;          // affichage pour debug
  int                      version_;        // version de DCOFLOW
  int                      coeffWeight_;    // coefficient multiplicateur des poids superieurs a 1
  double                   meanFlowSize_;   // taille moyenne des flots

  double                   mu_max_;          // valeur constante
  double                   mu_min_;          // valeur constante
  
public :

  // constructeurs

  Network():
    nbCoflows_(0),
    maxTime_(0.0),
    nbLinks_(0),
    nbFlows_(0),
    nbSlots_(0),
    link_(0),
    coflow_(0),
    slotSize_(0),
    slotStart_(0),
    slotEnd_(0),
    debug_(false),
    version_(2),
    coeffWeight_(1),
    meanFlowSize_(1.0)
  {}

  Network( const char * inputFileName, bool debug, int version=2, bool Sincronia_format=true );


  // destructeur
  ~Network()
  { 
    if ( link_ )
      delete [] link_;
    if ( coflow_ )
      delete [] coflow_;
    if ( slotSize_ ) {
      delete [] slotSize_;
      delete [] slotStart_;
      delete [] slotEnd_;
    }
  }


  // accesseurs aux attributs
  bool     debug() const { return debug_; }

  void     setDebug(bool b) { debug_ = b; }

  void     setVersion(int i) { version_ = i; }

  void     setCoeffWeight(int i) { coeffWeight_ = i; }
  
  Coflow & getCoflow(int k) { int m=flowIdMap_[k].first; return coflow_[m]; }
  
  int      getFlowPriority(int n) { pair<int,int> x=flowIdMap_[n]; return coflow_[x.first].getFlowPriority(x.second); }

  int      getNbCoflows() const { return nbCoflows_; }

  list<int> getSigmaOrder() const { return sigma_; }
  
  void      setMeanFlowSize(double m) { meanFlowSize_ = m; }
  
  int      getNbFlows() const { return nbLinks_; }
  
  // methodes de lecture et d'ecriture

  void     setNumberofCoflows(int nb);

  void     setNumberofLinks(int nb);

  void     setWeights(int *weight);
  
  void     setNumberofFlows(int k, int nb) { coflow_[k].setNbFlows(nb); }
  
  void     readCoflows(ifstream &inFile);

  void     addCoflow(int pos, int id, int w, double a, double d, int nb_flows, double t=0.0);

  void     addCoflow(int k, Coflow & c, double t=0.0) { addCoflow(k,c.getId(), c.getWeight(), c.getStartTime(), c.getEndTime(), c.getNbFlow(), t); };

  void     addLink(int pos, int id, double capa);

  void     addLink(int i, Link & link) { addLink(i,link.getId(),link.getCapa()); }
  
  int      readFlow(ifstream & inFile, Flow & flow);

  void     addFlow(int k, int i, int n, Flow & flow, bool dynamic=false);

  void     printCCT();

  void     printSigma();
  
  void     print();

  void     printNumberOfVariables();

  void     read_Sincronia_file(ifstream & inFile);
  
  // methodes de calcul generales

  int      schedule(double & cost, Network::Algorithm algo, bool type);

  int      selection(int alpha,double mu_max,int deadline ,list<int>& selection_list, bool RealFlowSizes);
  
  void     min_capa_on_path();
  
  void     reset();

  void     getCoflowParameters(int id, int &k, double &d, int &c, int & w);

  double   normalized_CCT(int k);
  
  double   getMeanCCT(bool online, double & avg_cct, double &avg_cct_norm, double & mu_min, double & mu_max, bool RealFlowSizes);
  
  double   getNU(bool RealFlowSizes);

  double   getMU_MAX(bool RealFlowSizes);

  // methodes pour l'ordonnancement optimal sur une machine

  int     sumOfWeights(set<int> S_b);
  
  void    sort_EDD_order(set<int> &S_b, list<int> &EDD_order);
  
  void    DP(int b, set<int> &S_b);

  void    Moore_hogdson(int b, set<int> &S_b);
  
  
  // methodes pour le calcul de la solution optimale
  // Youcef Magnouche, Sebastien Martin, Jeremie Leguay, Francesco De Pellegrini, Rachid Elazouzi, and Cedric Richier.
  // Branch-and-benders-cut algorithm for the weighted coflow completion time minimization problem

  int      optimal_solution(double & cost);

  void     compute_time_slots();
  
#ifdef GUROBI  

  int      solve(double & cost, bool approx);

  void     write_flow_var(GRBModel & model, GRBVar **f, GRBVar **y, GRBVar **gamma, GRBVar *C);
  
  void     write_objective_function(GRBModel & model, GRBVar *C);

  void     write_flow_cstr(GRBModel & model, GRBVar **f, GRBVar **y, GRBVar **gamma, GRBVar *C);

  void     write_capa_cstr(GRBModel & model, GRBVar **f, GRBVar **y, GRBVar **gamma, GRBVar *C);  

  void     write_utility_var(GRBModel & model, GRBVar *u);

  void     write_objective_utilities(GRBModel & model, GRBVar *u,
				     int nb_points, double *pwl_x, double *pwl_y);

  void     write_utility_cstr(GRBModel & model, GRBVar *u, GRBVar *z, int n1, int n2);

#endif
  

  // methodes pour cs_mha
  
  int      cs_mha(double & cost);

  int      cs_dp(double & cost);


  // methodes pour DCOFLOW V1, V2 et V3
  
  double   next_release_time(int link_id, set<int> & S, bool  type);
  
  int      bottleneck_link(set<int> & S, double & max_end_time, bool type );

  int      bottleneck_coflows(int b, double max_end_time, set<int> & S, set<int> & S_b);
  
  void     dcoflow_v1(int k, int i, double &index);

  void     dcoflow_v2(int k, int i, double &index);

  void     dcoflow_v3(int k, int i, double max_end_time, double &index);
  
  int      dcoflow_reject(int b, set<int> & S_b, double max_end_time);
  
  double   evalCCT(int coflowId);

  void     removeLateCoflows(list<int> & sigma_star);
  
  int      dcoflow(double & cost);
 
  // new methods 
  
  
  vector< vector<int> > subsets(set<int>& nums) ;

  void generateSubsets(set<int>& nums, set<int>::iterator start, vector<int>& subset, vector< vector<int> > &result)  ;
  
  double total_transmission_time(int l, int k,bool RealFlowSizes) ;

  int largest_wpt(int b, set<int> &S, set<int> & S_b, map<int, double> & weights,bool type);

  double primal_variable(int b, set<int> & S);

  void update_sincroniaCoflows_weights(set<int> S, map<int, double>  &weights , int b, int IK,bool type );

  void Sincronia(double & cost, bool type);

  void Sincronia_released_time( double & cost,bool RealFlowSizes );

  int last_coflow_released(set<int>  S);

  float average_proc_time() ;


#ifdef GUROBI  
  
  void GSolve(double & cost);
  
  void write_cstr(GRBModel & model, GRBVar *z);

  void write_cstr_released(GRBModel & model, GRBVar *z); //with released time
  
  void obj_function(GRBModel & model, GRBVar *z);

  list<int> MUWPSolve(int alpha,double mu_max,int D, bool RealFlowSizes);

  void obj_function_muwp(GRBModel & model, GRBVar *y);

  void write_cstr_muwp(GRBModel &model, GRBVar *y,int alpha,double mu_max, int D, bool RealFlowSizes);

  void write_cstr_cos_muwp(GRBModel &model, GRBVar *y,int alpha,double mu_max, int D, bool RealFlowSizes) ;

#endif
  
  // CLPSimplx Solver 
  list<int> MUWPSolve_CLP(int alpha, double mu_max, int D, bool RealFlowSizes);

  double compute_p_lk(const Coflow& c, int link, bool RealFlowSizes) ;

  void set_flows_pred(double stdev);
  
  //  void set_flows_pred(double mean_flow_size, double stdev_flow_size);

  void set_flows_pred_true(double delta, int s);  // Pour generer des predictions a partir de l'error delta suivant une loi normale

  // void set_flows_pred_geo(double p);
  
  void roundRobin(double & cost);
  
  void RandomSigma(double & cost );


  double getAllFlowSize(int k, bool RealFlowSizes) ;

  double getMu_max(){ return mu_max_;};

  double getMu_min(){ return mu_min_;};

  void setMu_max(double m){ mu_max_ = m;};

  void setMu_min(double m){ mu_min_ = m;};
  
};


// methodes d'affichage 

ostream & operator<< (ostream &, Network const &);


#endif

