#include "lois.hxx"
#include <math.h>



UnifGen  RandomGen::gen_ = UnifGen(NAG_M,NAG_A,NAG_C,NAG_G0);



void  UnifGen::test() {
  int n= 10000;
  int k= 101;
  int i, j;
  double x, Q;
  double f[101];

  // on initialise le tableau des frequences
  for ( j=0; j<k; j++) 
    f[j] = 0;
  
  // on genere n nombres suivant U(0,1) et on calcule 
  // la frequence des intervalles [0,1/k] , [1/k, 2/k] ...
  for (i=0; i<n; i++) {
    x = k * unif();
    j = (int) floor( x );
    f[j] += 1.0;
  }
  
  // on calcule la statistique Q
  Q = 0.0;
  x = ((double) n) / ((double) k);
  for ( j=0; j<k; j++)
    Q += pow( (f[j] - x), 2.0);
  Q /= x;
  
  // on compare a la valeur du khi-carre pour k=100 et alpha=1%
  cerr << "Q = "
       << Q
       << " a comparer avec "
       << VAL_KHI_CARRE
       << endl;
  if ( Q > VAL_KHI_CARRE )
    cerr << "Echantillons probablement non uniformes" << endl;
  else
    cerr << "Test du khi-carre passe avec succes" << endl;
}


double ExpoGen::getMoment(int n) {
  double factn = 1.0;

  for (int i=1; i<=n; i++)
    factn *= i;

  return factn * pow(average_, (double) n);
}









