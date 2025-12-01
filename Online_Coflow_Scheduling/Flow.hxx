#ifndef __Flow_hxx__
#define __Flow_hxx__



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

#define MIN_PRIORITY  100000



//****************************************************************//
//*                                                              *//
//----------------- Public Classes SECTION -----------------------//
//*                                                              *//
//****************************************************************//



//----------------------------------------------------------------//
// Class :      Flow                                              //
//                                                                //
//                                                                //
// Description : classe representant un flot et la liste des      //
// liens qu'il utilise                                            //
//                                                                //
//----------------------------------------------------------------//


class  Flow
{
public:
  enum FlowType { FUTURE_FLOW=0, PENDING, RUNNING, DONE};

  // attributs prives 
private:
  int              id_; 
  double           size_;
  double           residualSize_;
  int              length_;
  int            * path_;
  int              priority_;
  FlowType         status_;
  double           ct_;
  double           minCapa_;
  double           PredSize_;
  
  // methodes amies
public:
  friend   istream   &   operator>>(istream &stream, Flow &);
  friend   ostream   &   operator<<( ostream &, Flow const & );

  
  // methodes de classe 
public:

  // constructeurs 
  Flow() {
    residualSize_ = 0.0;
    path_ = 0;
    status_ = FUTURE_FLOW;
    ct_ = 0.0;
    minCapa_ = 0.0;
  }


  Flow(int id, double size, int length, double PredSize): 
    id_(id),
    size_(size),
    PredSize_(PredSize),
    residualSize_(size),
    length_(length)
  {
    path_ = new int[length_];
    status_ = FUTURE_FLOW;
    priority_ = 0;
    ct_ = 0.0;
    minCapa_ = 0.0;
  }

  Flow(const Flow &other): 
    id_(other.id_),
    size_(other.size_),
    PredSize_(other.PredSize_),
    residualSize_(other.residualSize_),
    length_(other.length_)
  {
    path_ = new int[length_];
    for (int i=0; i<length_; i++)
      path_[i] = other.path_[i];
    status_ = other.status_;
    priority_ = other.priority_;
    ct_ = other.ct_;
    minCapa_ = other.minCapa_;
  }

  //destructeur
  ~Flow()
  { 
    if ( path_ )
      delete [] path_;
  }


  // accesseurs 

  int    getId() const { return id_; }

  double getSize() const { return size_; }
  
  double getPredSize() const { return PredSize_; }

  double getResidualSize() const { return residualSize_; }
  
  int    getLength() const { return length_; }

  int    getLink(int i) const { return path_[i]; }

  FlowType getStatus() const { return status_; }

  int    getPriority() const { return priority_; }

  double getExpectedCompletionTime() const { return ct_; }
  
  int *  getPath() const { return path_; }

  double getMinCapa() const { return minCapa_; }
  
  int    getSource() const { return path_[0]; }

  int    getDestination() const { return path_[1]; }

  
  void   setId(int f) { id_ = f; }

  void   setSize(double s) { size_ = s; }
  
  void   setPredSize(double Ps) { PredSize_ = Ps; }

  void   setResidualSize(double s) { residualSize_ = s; }
  
  void   setLength(int l) { length_ = l; }

  void   setLink(int i, int j) { path_[i] = j; }

  void   setStatus(FlowType t) { status_ = t; }

  void   setPriority(int p) { priority_ = p; }

  void   setExpectedCompletionTime(double x) { ct_=x; }

  void   setMinCapa(double x) { minCapa_ = x; }

  
  // methodes de calcul
  void   allocate() { path_ = new int[length_]; }

  bool   useLink(int l);
};




//----------------------------------------------------------------//
// Class :      Coflow                                            //
//                                                                //
//                                                                //
// Description : classe representant un coflow avec sa date       //
// d'arrivee, sa deadline et la liste de ses flots                //
//                                                                //
//----------------------------------------------------------------//


class  Coflow
{
  // attributs prives 
private:
  int              id_;
  int              classId_;
  int              weight_;
  double           arrival_;
  double           deadline_;
  int              nbFlow_;
  Flow           * flow_;
  double           cct_;
  double           CT_;
  
  // methodes amies
public:
  friend   istream   &   operator>>(istream &stream, Coflow &);
  friend   ostream   &   operator<<( ostream &, Coflow const & );
  
  // methodes de classe 
public:

  // constructeurs 
  Coflow() {
    flow_ = 0;
    classId_ = 1;
    weight_ = 1;
    cct_ = 0.0;
    CT_ =0.0;
  }


  Coflow(int id, int w, double startTime, double endTime, int nbFlow, double ct): 
    id_(id),
    classId_(w),
    weight_(w),
    arrival_(startTime),
    deadline_(endTime),
    nbFlow_(nbFlow),
    CT_(ct)
  {
    flow_ = new Flow[nbFlow_];
    cct_ = 0.0;
  }

  Coflow(const Coflow &other): 
    id_(other.id_),
    classId_(other.classId_),
    weight_(other.weight_),
    arrival_(other.arrival_),
    deadline_(other.deadline_),
    nbFlow_(other.nbFlow_),
    CT_(other.CT_)
  {
    flow_ = new Flow[nbFlow_];
    for (int i=0; i<nbFlow_; i++)
      flow_[i] = other.flow_[i];
    cct_ = other.cct_;
  }

  //destructeur
  ~Coflow()
  { 
    if ( flow_ )
      delete [] flow_;
  }


  // accesseurs 

  int    getId() const { return id_; }

  int    getClassId() const { return classId_; }
  
  int    getWeight() const { return weight_; }
  
  double getStartTime() const { return arrival_; }

  double getEndTime() const { return deadline_; }

  int    getNbFlow() const { return nbFlow_; }

  int    getLength(int i) const { return flow_[i].getLength(); }

  int    getLink(int i, int j) const { return flow_[i].getLink(j); }

  int    getFlowId(int i) const { return flow_[i].getId(); }

  double getFlowSize(int i) const { return flow_[i].getSize(); }

  double getFlowPredSize(int i) const { return flow_[i].getPredSize(); }
  
  double getFlowResidualSize(int i, bool preemption) { if ( preemption ) return flow_[i].getResidualSize(); else return flow_[i].getSize(); }
  
  Flow::FlowType  getFlowStatus(int i) const { return flow_[i].getStatus(); }

  Flow & getFlow(int i) { return flow_[i]; }
  
  double getCCT() const { return cct_; }

  int    getFlowPriority(int i) const { return flow_[i].getPriority(); }

  double getFlowExpectedCT(int i) const { return flow_[i].getExpectedCompletionTime(); }

  double getFlowMinCapa(int i) const { return flow_[i].getMinCapa(); }

  int    getFlowSource(int i) const { return flow_[i].getSource(); }

  int    getFlowDestination(int i) const { return flow_[i].getDestination(); }

  
  
  void   setId(int id) { id_ = id; }

  void   setClassId(int id) { classId_ = id; }
  
  void   setWeight(int w) { weight_ = w; }
  
  void   setStartTime(double a) { arrival_ = a; }

  void   setDeadline(double d) { deadline_ =d; }

  void   setNbFlows(int n) { nbFlow_ = n; }
  
  void   setFlow(int i, Flow & f, bool dynamic) { flow_[i] = f; if ( dynamic) flow_[i].setSize(f.getResidualSize()); }

  void   setFlowPath(int i, int *path);

  //
  void   setFlowSize(int i , double v)  { flow_[i].setSize(v) ;}
  
  void   setFlowPredSize(int i , double v)  { flow_[i].setPredSize(v) ;}
  //
  
  void   setFlowStatus(int i, Flow::FlowType t) {flow_[i].setStatus(t); }

  void   setCCT(double x) { cct_=x; }

  void   updateCCT(double t) { if (cct_ < t) cct_=t; }
  
  void   setFlowPriority(int i, int p) { flow_[i].setPriority(p); }

  void   setFlowResidualSize(int i, double x, bool preemption) { if ( preemption ) flow_[i].setResidualSize(x); }

  void   setFlowExpectedCT(int i, double x) { flow_[i].setExpectedCompletionTime(x); }

  void   AddToFlowExpectedCT(int i, double x) { flow_[i].setExpectedCompletionTime(x + flow_[i].getExpectedCompletionTime()); } // augmenter le cct du flow 

  void   setFlowMinCapa(int i, double x) { flow_[i].setMinCapa(x); }
  
  double getCT() const { return CT_; }

  void   setCT(double ct) { CT_ = ct; }
  
  
  // methodes de calcul
  void   allocate() { flow_ = new Flow[nbFlow_]; }

  void   reset();

  void   reject();
  
  int    readFlow(ifstream & inFile, int i);

  bool   useLink(int i, int l) { return flow_[i].useLink(l); }

  double loadOnLink(int l);
  
  double loadOnLinkPred(int l);

  int    numberOfFlowsOnLink(int l);
  
  bool   useLink(int l);
  
  void   scale(double x);

  void   printFlow(int i);
};




#endif
