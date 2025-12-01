#ifndef __lois_hxx__

#define __lois_hxx__




//****************************************************************//
//*                                                              *//
//*------------------- Includes SECTION -------------------------*//
//*                                                              *//
//****************************************************************//

#include <math.h>
#include <iostream>
#include <vector>



//****************************************************************//
//*                                                              *//
//------------ Public Data Structures SECTION --------------------//
//*                                                              *//
//****************************************************************//

using namespace std;


// definition du type uint64
typedef unsigned long long int uint64;


// Minimal Standard Generator : LCG(2^31-1, 16807, 0, 1)

#define MSG_M		2147483647LL
#define MSG_A		16807LL
#define MSG_C		0LL
#define MSG_G0		1LL


// NAG's Generator : LCG(2^59, 13^13, 0, 123456789(2^32+1))

#define NAG_M		576460752303423488LL
#define NAG_A		302875106592253LL
#define NAG_C		0LL
#define NAG_G0		530242871347629333LL


// Khi-carre pour 100 degres de libertes et 99% de confiance
#define VAL_KHI_CARRE    135.80672



//****************************************************************//
//*                                                              *//
//----------------- Public Classes SECTION -----------------------//
//*                                                              *//
//****************************************************************//








//----------------------------------------------------------------//
// Class :    UnifGen                                             //
//                                                                //
//                                                                //
// Description : classe representant une loi uniforme             //
//                                                                //
//----------------------------------------------------------------//


class UnifGen
{
  //attributs
private:

  uint64   modulo_;
  uint64   multiplicateur_;
  uint64   increment_;
  uint64   value_;


public:

  //----------------------------------------------------------------//
  //                 CONSTRUCTEURS ET DESTRUCTEUR                   //
  //----------------------------------------------------------------//

  ///Constructeur par defaut
  UnifGen( uint64 m = MSG_M, uint64 a = MSG_A, uint64 c = MSG_C, uint64 g0 = MSG_G0 ) : 
    modulo_(m), 
    multiplicateur_(a),
    increment_(c),
    value_(g0)
  {  }

  ///Constructeur par recopie
  UnifGen(UnifGen const &other) : 
    modulo_(other.modulo_), 
    multiplicateur_(other.multiplicateur_),
    increment_(other.increment_),
    value_(other.value_)
  {}


  //----------------------------------------------------------------//
  //                   ACCESSEURS                                   //
  //----------------------------------------------------------------//

  uint64  getModulo() { return modulo_; }

  uint64  getMultiplicateur() { return multiplicateur_; }

  uint64  getIncrement() { return increment_; }

  uint64  getValue() { return value_; }


  //----------------------------------------------------------------//
  //               METHODES DE CALCUL                               //
  //----------------------------------------------------------------//


  //generation d'un nombre aleatoire suivant U(0,1)
  double unif() {

    value_ = ( value_ * multiplicateur_ + increment_ ) % modulo_;
    return ((double) value_) / ((double) (modulo_-1) );
  }


  //test du khi-carre : n=10000, k=101, alpha=1%
  void  test();
};



//----------------------------------------------------------------//
// Class :   RandomGen                                            //
//                                                                //
//                                                                //
// Description : classe pour generation de nombres aleatoires     //
//                                                                //
//----------------------------------------------------------------//

class RandomGen
{
private:
  static UnifGen  gen_;

public:

  RandomGen() {}


  // generation d'un nombre aleatoire
  double  unif() { return gen_.unif(); }

  virtual double randomNumber() { return gen_.unif(); }


  //moment de la loi
  virtual double getMoment(int n) { return 1.0/(n+1); }
};






//----------------------------------------------------------------//
// Class :    ExpoGen                                             //
//                                                                //
//                                                                //
// Description : classe representant une loi exponentielle        //
//                                                                //
//----------------------------------------------------------------//


class ExpoGen: public RandomGen
{
private :

  double average_;

public :

  //----------------------------------------------------------------//
  //                 CONSTRUCTEURS ET DESTRUCTEUR                   //
  //----------------------------------------------------------------//

  ///Constructeur par defaut
  ExpoGen( double avg=1.0 ) :  
    RandomGen(),
    average_(avg)
  {  }


  //----------------------------------------------------------------//
  //                   ACCESSEURS                                   //
  //----------------------------------------------------------------//

  double  getAverage() { return average_; }

  void    setAverage(double x) { average_ = x; }

  void    setRate(double lambda) { average_ = 1.0/lambda; }

  //----------------------------------------------------------------//
  //               METHODES DE CALCUL                               //
  //----------------------------------------------------------------//

  double randomNumber() {
    return - average_ * log( unif() ) ;
  }

  double  getMoment(int n);
  
};


#endif
