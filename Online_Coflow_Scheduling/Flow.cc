
//****************************************************************//
//*                                                              *//
//*------------------- Includes SECTION -------------------------*//
//*                                                              *//
//****************************************************************//
#include <math.h>

#include "Flow.hxx"



//****************************************************************//
//*                                                              *//
//*------------- Private Variables SECTION ----------------------*//
//*                                                              *//
//****************************************************************//





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
// Private Function:   ostream << Flow                            //
//----------------------------------------------------------------//


ostream &operator<<(ostream &stream, Flow const &S)
{
  stream << "Flow " << S.id_ 
	 << " : size = " 
	 << S.size_; 

  return stream;
}



//----------------------------------------------------------------//
// Private Function:   istream >> Flow                            //
//----------------------------------------------------------------//

istream &operator>>(istream &stream, Flow &x)
{
  return
    (
     stream >> x.id_ 
     >> x.size_ 
     >> x.length_
     );
}


//----------------------------------------------------------------//
// Private Function:   ostream << Coflow                          //
//----------------------------------------------------------------//


ostream &operator<<(ostream &stream, Coflow const &S)
{
  stream << "Coflow " << S.id_ 
	 << " : weight = " << S.weight_
	 << ", start time = " 
	 << S.arrival_ 
	 << ", deadline="
	 << S.deadline_
	 << ", number of flows="
	 << S.nbFlow_;

  return stream;
}



//----------------------------------------------------------------//
// Private Function:   istream >> Coflow                          //
//----------------------------------------------------------------//

istream &operator>>(istream &stream, Coflow &x)
{
  return
    (
     stream >> x.id_ 
     >> x.weight_
     >> x.arrival_ 
     >> x.deadline_
     >> x.nbFlow_
     );
}



//****************************************************************//
//*                                                              *//
//*----------- Public Methods Definition SECTION ----------------*//
//*                                                              *//
//****************************************************************//


//----------------------------------------------------------------//
// Public Method:       useLink                                   //
//----------------------------------------------------------------//

bool Flow::useLink(int l) {
  int  i;

  for (i=0; i<length_; i++)
    if ( path_[i] == l )
      return true;

  return false;
}



//----------------------------------------------------------------//
// Public Method:      printFlow                                  //
//----------------------------------------------------------------//

void Coflow::printFlow(int i) {
  int k, l;
  
  cerr << flow_[i];
  cerr << "\tpath = ";
  for (k=0; k<flow_[i].getLength(); k++) {
    l = flow_[i].getLink(k);
    cerr << l << "  ";
  }
  cerr << endl;
}


//----------------------------------------------------------------//
// Public Method:     setFlowPath                                 //
//----------------------------------------------------------------//

void Coflow::setFlowPath(int i, int *path) {
  int k;
  int n = flow_[i].getLength();
  
  flow_[i].allocate();
  for (k=0; k<n; k++) 
    flow_[i].setLink(k, path[k]);
}


//----------------------------------------------------------------//
// Public Method:      readFlow                                   //
//----------------------------------------------------------------//

int Coflow::readFlow(ifstream & inFile, int i) {
  int k, l;
  
  inFile >> flow_[i];
  flow_[i].allocate();
  for (k=0; k<flow_[i].getLength(); k++) {
    inFile >> l;
    flow_[i].setLink(k,l);
  }

  return flow_[i].getId();
}


//----------------------------------------------------------------//
// Public Method:      scale                                      //
//----------------------------------------------------------------//

void   Coflow::scale(double x) {
  double s;
  int i;

  arrival_ *= x;
  deadline_ *= x;
  for (i=0; i<nbFlow_; i++) {
    s = flow_[i].getSize();
    s *= x;
    flow_[i].setSize(s);
  }
}


//----------------------------------------------------------------//
// Public Method:     loadOnLink                                  //
//----------------------------------------------------------------//

double   Coflow::loadOnLink(int l) {
  double s;
  double result = 0.0;
  int i;

  for (i=0; i<nbFlow_; i++) {
    s = flow_[i].getSize();
    if ( flow_[i].useLink(l) )
      result += s;
  }

  return result;
}


//----------------------------------------------------------------//
// Public Method:     numberOfFlowsOnLink                         //
//----------------------------------------------------------------//

int   Coflow::numberOfFlowsOnLink(int l) {
  int result = 0;
  int i;

  for (i=0; i<nbFlow_; i++) {
    if ( flow_[i].useLink(l) )
      result += 1;
  }

  return result;
}

//----------------------------------------------------------------//
// Public Method:     loadOnLinkPred                                  //
//----------------------------------------------------------------//

double   Coflow::loadOnLinkPred(int l) {
  double s;
  double result = 0.0;
  int i;

  for (i=0; i<nbFlow_; i++) {
    s = flow_[i].getPredSize();
    if ( flow_[i].useLink(l) )
      result += s;
  }

  return result;
}

//----------------------------------------------------------------//
// Public Method:     useLink                                     //
//----------------------------------------------------------------//

bool   Coflow::useLink(int l) {
  int i;

  for (i=0; i<nbFlow_; i++) {
    if ( flow_[i].useLink(l) )
      return true;
  }

  return false;
}


//----------------------------------------------------------------//
// Public Method:     reset                                       //
//----------------------------------------------------------------//

void Coflow::reset() {
  double x;
  int    i;
  
  cct_ = 0.0;
  for (i=0; i<nbFlow_; i++) {
    flow_[i].setPriority(0);
    flow_[i].setStatus(Flow::FUTURE_FLOW);
    x = flow_[i].getSize();
    flow_[i].setResidualSize(x);
  }
}


//----------------------------------------------------------------//
// Public Method:     reject                                      //
//----------------------------------------------------------------//

void Coflow::reject() {
  int    i;
  
  for (i=0; i<nbFlow_; i++) 
    flow_[i].setPriority(MIN_PRIORITY);
}

//----------------------------------------------------------------//
// Public Method:     Update_volume                               //
//----------------------------------------------------------------//


//void Flow :: Update_volume (s){



