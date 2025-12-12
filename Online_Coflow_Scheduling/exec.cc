#include <math.h>
#include <chrono>
#include <ctime>
#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <vector>
#include <random>
#include <array>

#include <iomanip>
#include "tinydir.h"

#include "Simulator.hxx"
#include "Stats.hxx"

#define EPS   1.0e-6

#define ALL -1
#define R 0
#define CL 1
#define NC 2
#define RO 3
#define Opt 4
#define CRT 5

using namespace std;




//****************************************************************//
//*                                                              *//
//------------ Public Data Structures SECTION --------------------//
//*                                                              *//
//****************************************************************//

bool            online=false; // false ;
double          slot_size = 0.0;
double          lambda=1.0;

double          mean_flow_size = 1.0;
double          stdev_flow_size = 0.01;

int             start_file=-1;
int             end_file=1000000;

int             nb_files = -1;
int             version = 2;
int             period=10;
double          prev_cct_pred;


bool            COMPUTE_OPT = true ;

vector<int>      INST;
vector<double>   LB;
vector<double>   SIN;
vector<double>   SIN_T;
vector<double>   RR;
vector<double>   PRED;
vector<double>   RANDO;
vector<string>   ORDER;

vector<long double> time_sched;
vector<long double> total_time;

int choix = -1;

// check if the the output file has been provided from  the command line
bool		OFILE_FLAG = false;
string 		ofname;



vector<int>    *INST_fig;
vector<double> *DELTA_fig;
vector<double> *MEAN_fig;
vector<double> *MIN_fig;
vector<double> *MAX_fig;
vector<double> *MEDIAN_fig;
vector<double> *STDDEV_fig;
vector<double> *FREQ_fig;

vector<long double> *time_sched_fig;
vector<long double> *total_time_fig;

int                NB_predictions = 3; //1000 ;

bool               sp_anticipate = false; // compute de update SP that deals with workconservation 

//****************************************************************//
//*                                                              *//
//*----------- Private Functions Definition SECTION -------------*//
//*                                                              *//
//****************************************************************//


string int_to_string(int i) {stringstream s; s << i; return s.str(); }

string double_to_string(double x) {stringstream s; s << x; return s.str(); }

string list_int_to_string(list<int> & l) {
  ostringstream os;
  list<int>::iterator it = l.begin();

  for ( ; it != l.end(); it++) {
    os << *it << " ";
  }
  string str(os.str());
  return str;
}


//----------------------------------------------------------------//
// Private Function:   set_to_zero                                //
//----------------------------------------------------------------//

void set_to_zero(double & cost, double & cct, double & cct_norm) {
  cost = 0.0;
  cct = 0.0;
  cct_norm = 0.0;
}


//----------------------------------------------------------------//
// Private Function:   run_algorithm                              //
//----------------------------------------------------------------//

void debug(Simulator & net, bool flag=true) {

  if ( flag ) {
    net.print();
    net.setDebugEvent(true);
  }
  else
    net.setDebugEvent(false);
}



//----------------------------------------------------------------//
// Private Function:   run_algorithm                              //
//----------------------------------------------------------------//

// Modify for online adaptation
void run_algorithm(Simulator & net, Network::Algorithm & algo, 
		   double & cost, double & cct, double & cct_norm,
		   double & mu_min, double & mu_max,
		   long double & time_elapsed_ms, long double & total_time_ms,  bool RealFlowSizes ) {

  clock_t            c_start, c_end, c_end2;

  c_start = clock();
  if ( !online) {
    //Algorithme de scheduling
    net.schedule(cost, algo, RealFlowSizes);
    c_end = clock();
      //Simulation
    if ( (algo != Network::LP) && (algo != Network::LP_LB)  ) 
      net.offline_simulation(algo);
  }  
  else {
    // cerr << ">>> MODE ONLINE: Execution <<<" << endl;  
      // net.online_simulation(algo, online, slot_size, RealFlowSizes);
      if (sp_anticipate)
        net.online_simulation_greedy_anticipate(algo, slot_size, RealFlowSizes);
      else 
        net.online_simulation_greedy(algo, slot_size, RealFlowSizes);

  } 
  c_end2 = clock();
  
  //Evaluation des statistiques
  net.getMeanCCT(online, cct, cct_norm, mu_min, mu_max, RealFlowSizes);
  //  cerr << "\tAverage CCT = " << cct << endl;
  
  //calcul des temps de calcul
  time_elapsed_ms = 1000.0 * (c_end-c_start) / CLOCKS_PER_SEC;
  total_time_ms = 1000.0 * (c_end2-c_start) / CLOCKS_PER_SEC;
}


//----------------------------------------------------------------//
// Private Function:   store_results                              //
//----------------------------------------------------------------//

void store_results(int index, double lb, double sin,double sin_t, double rr, double ro, double pred, long double time_elapsed_ms, long double total_time_ms, string order_str) {

  INST.push_back(index);
  LB.push_back(lb);
  SIN.push_back(sin);
  SIN_T.push_back(sin_t);
  RR.push_back(rr);
  RANDO.push_back(ro);
  PRED.push_back(pred);
  time_sched.push_back(time_elapsed_ms);
  total_time.push_back(total_time_ms);
  ORDER.push_back(order_str);
}


//----------------------------------------------------------------//
// Private Function:   store_results_fig                              //
//----------------------------------------------------------------//

void store_results_fig(int n, int index, double delta, Stats s, long double time_elapsed_ms, long double total_time_ms) { 
  INST_fig[n].push_back(index);
  DELTA_fig[n].push_back(delta);
  MEAN_fig[n].push_back(s.mean());
  MIN_fig[n].push_back(s.minimum());
  MAX_fig[n].push_back(s.maximum());
  MEDIAN_fig[n].push_back(s.median());
  STDDEV_fig[n].push_back(s.stddev());
  FREQ_fig[n].push_back(s.frequency());
  time_sched_fig[n].push_back(time_elapsed_ms);
  total_time_fig[n].push_back(total_time_ms);
}


//----------------------------------------------------------------//
// Private Function:   write_average_proc_time                              //
//----------------------------------------------------------------//
void write_average_proc_time(int N, int L, float avg,const char * outputDirName){
  
  string filename = "average_proc_time.txt";
  string outputFileName = string(outputDirName) + "/" + filename;

  ofstream outfile;

  // Ouvrir en mode "append" si existant, ou créer sinon (std::ios::app)
  outfile.open(filename, std::ios::out | std::ios::app);

  if (!outfile.is_open()) 
    cerr << "Erreur lors de l'ouverture du fichier." << endl;
  

 
  outfile <<"Average N="<<N <<" L= "<< L <<" : "<<avg << endl;

  outfile.close();

  std::cout << "Valeur écrite dans le fichier avec succès." << std::endl;

}
//----------------------------------------------------------------//
// Private Function:   process_file                               //
//----------------------------------------------------------------//

void process_file(const char * fileName, const char * outputDirName, int index) {
  Simulator          net( fileName, false, true, false, version, true );  // second arg: debug; last: sincronia file 
  double             cost, delta, cct_ref;
  double             cct_lp, cct_norm_lp;
  double             cct_sin, cct_norm_sin;
  double             cct_sin_t, cct_norm_sin_t;
  double             cct_pred, cct_pred_norm;
  double             cct_r, cct_r_norm;
  double             cct_ro, cct_ro_norm;
  double             mu_min, mu_max;
  double             x, ratio;
  long double        time_elapsed_ms, total_time_ms, avg_elapsed, avg_total;
  Network::Algorithm algo = Network::LP_LB;
  bool               RealFlowSizes = false;
  list<int>          pred_order;
  
  // array<double,12>   pred_error = {0.0, 0.01, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 0.99}; //old values of delta
  // array<double,10>   pred_error = {0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.99}; //10 values of delta
  
  // array<double,1>   pred_error = {0.99}; //1 value of delta
  array<double,5>   pred_error = {0.0, 0.3, 0.5, 0.7, 0.99};
  // array<double,3>   geometric_pred_param = {0.25, 0.5, 0.75};
  Stats              stats_cct_pred, stats_cct_r;
   
  //execution de Gurobi
  if ((choix == Opt) || (choix == ALL)) {
       cerr << "\tExecution de Gurobi pour la solution optimale..." << endl;
    algo = Network::LP_LB;
    RealFlowSizes = true;
    run_algorithm(net, algo, cost, cct_lp, cct_norm_lp, mu_min, mu_max, time_elapsed_ms, total_time_ms, RealFlowSizes);
    cerr << "\t\t\t=> ***gurobi cct : " << cct_lp << endl;
  }
  else{
    cct_lp = -1; }

  // execution de Sincronia clairvoyant
  if ((choix == CL) || (choix == ALL)) {
      cerr << "\t> Clairvoyant Sincronia:" << endl;
    net.reset();
    set_to_zero(cost, cct_sin, cct_norm_sin);
    algo = Network::SINCRONIA;
    RealFlowSizes = true;
    net.setLambda(1.0);
    run_algorithm(net, algo, cost, cct_sin, cct_norm_sin, mu_min, mu_max, time_elapsed_ms, total_time_ms, RealFlowSizes);
    cerr << "\t\t\t=> average cct CL=" << cct_sin << endl;
  }
  else 
    cct_sin = -1;

  // execution de Sincronia clairvoyant with released time
  if ((choix == CRT) || (choix == ALL)) {
    cerr << "\t> Clairvoyant Sincronia with released time:" << endl;
    net.reset();
    set_to_zero(cost, cct_sin_t, cct_norm_sin_t);
    algo = Network::SINCRONIARelease;
    RealFlowSizes = true;
    net.setLambda(1.0);
    run_algorithm(net, algo, cost, cct_sin_t, cct_norm_sin_t, mu_min, mu_max, time_elapsed_ms, total_time_ms, RealFlowSizes);
    cerr << "\t\t\t=> average cct CL=" << cct_sin_t << endl;
  }
  else 
    cct_sin_t = -1;

  

   // ############### Execution de Sincronia avec les valeurs predites ####################
  
  if ((choix == NC) || (choix == ALL)) { 
      cerr << "Execution de Sincronia avec les valeurs predites..." << endl;
      RealFlowSizes = false;
      algo = Network::SINCRONIA;
      net.setLambda(1.0);
      // char pred_law= "U";
    // boucle sur delta
        for (const auto & delta : pred_error) {

          // initialisation
          avg_elapsed = 0;
          avg_total = 0;
          stats_cct_pred.clear();

          // affichage
          cerr << "\tPredictions with delta=" << delta << endl;
          int nb_pred = NB_predictions;
          if (delta == 0.0) nb_pred = 1; // une seule si vraie valeur
          // boucle sur les predictions
          for (int i=0 ; i<nb_pred ;i++ )
            {  
              net.reset();
              set_to_zero(cost, cct_pred, cct_pred_norm);
              net.set_flows_pred_true(delta,i) ;
              net.setMu_max(1+delta);
              // cerr << "-----------------------Start -------------------------" << endl;
              
              run_algorithm(net, algo, cost, cct_pred, cct_pred_norm, mu_min, mu_max, time_elapsed_ms, total_time_ms, RealFlowSizes);
              // cerr << "cct_pred : " << cct_pred << endl;
              cerr << "\t\t\t=> average cct pred=" << cct_pred  << endl;

              stats_cct_pred.push_back(cct_pred);
              
              avg_elapsed += time_elapsed_ms;
              avg_total += total_time_ms;
            } // fin boucle sur les predictions
            cerr << endl; 
            pred_order = net.getSigmaOrder();
            // store results
            stats_cct_pred.run_stats();

            avg_elapsed /= NB_predictions;
            avg_total /= NB_predictions;

            stats_cct_pred.print_stats();
            // cerr << "index "<<index << " delta "<< delta <<" avg "<< avg_elapsed <<" avg t "<< avg_total << endl;
            store_results_fig(0, index, delta, stats_cct_pred, avg_elapsed, avg_total);	
        }
  }
  else 
    cct_pred = -1;

  //Ratio
  
  // ################# Ration Prediction/ clairvoyant ##################
  if ((choix == R) || (choix == ALL)) { 
      // Sauvegarde l'état initial du mode "online"
      bool was_online = online;
      online = false;

      // ----- Clairvoyant Sincronia (avec vraies tailles) -----
      cerr << "\t> Clairvoyant Sincronia with released time:" << endl;
      net.reset();
      set_to_zero(cost, cct_sin_t, cct_norm_sin_t);
      algo = Network::SINCRONIARelease;
      RealFlowSizes = true;
      net.setLambda(1.0);
      run_algorithm(net, algo, cost, cct_sin_t, cct_norm_sin_t, mu_min, mu_max, time_elapsed_ms, total_time_ms, RealFlowSizes);
      cerr << "\t\t\t=> average cct CL=" << cct_sin_t << endl;

      // Restaure l'état précédent
      online = was_online;

      // ----- Sincronia prédictif -----
      cerr << "\t>Prediction Sincronia :" << endl;
      RealFlowSizes = false;
      algo = Network::SINCRONIA;
      net.setLambda(1.0);
      
      // boucle sur delta
        for (const auto & delta : pred_error) {

          // initialisation
          avg_elapsed = 0;
          avg_total = 0;
          stats_cct_pred.clear();

          // affichage
          cerr << "\t\tPredictions with delta=" << delta << endl;
          int nb_pred = NB_predictions;
          if (delta == 0.0) nb_pred = 1; // une seule si vraie valeur
          // boucle sur les predictions
          cerr << "test 3"<< endl; 
          for (int i=0 ; i<nb_pred ;i++ )
            { 
              cerr << "test "<< endl; 
              net.reset();
              set_to_zero(cost, cct_pred, cct_pred_norm);
              net.set_flows_pred_true(delta,i) ;
              net.setMu_max(1+delta);
              // cerr << "-----------------------Start -------------------------" << endl;
              
              run_algorithm(net, algo, cost, cct_pred, cct_pred_norm, mu_min, mu_max, time_elapsed_ms, total_time_ms, RealFlowSizes);
              // cerr << "cct_pred : " << cct_pred << endl;
              cerr << "\t\t\t=> average cct pred=" << cct_pred  <<" for nb_pred i " << i<< endl;
              if(cct_sin_t !=0) x = cct_pred/cct_sin_t;
              else x = -1;
              cerr << "error : " << x << endl;
              stats_cct_pred.push_back(x);
          
              avg_elapsed += time_elapsed_ms;
              avg_total += total_time_ms;
            } // fin boucle sur les predictions
            cerr << endl; 
            pred_order = net.getSigmaOrder();

            // store results
            stats_cct_pred.run_stats();

            avg_elapsed /= NB_predictions;
            avg_total /= NB_predictions;

            stats_cct_pred.print_stats();
            cerr << "test1 "<< endl; 
            store_results_fig(1, index, delta, stats_cct_pred, avg_elapsed, avg_total);		
            cerr << "test 2"<< endl; 
        }
  }
  else 
    cct_r = -1;
  //execution de Greedy rate allocation avec Random Order
  cct_ro = -1;

  
  //stockage des resultats
  store_results(index, cct_lp, cct_sin,cct_sin_t, cct_r, cct_ro, cct_pred, time_elapsed_ms, total_time_ms, list_int_to_string(pred_order));

  // ****************Get the average p_lk***************
  // int N = net.getNbCoflows();
  // int L = net.getNbFlows();
  // float avg = net.average_proc_time();
  // write_average_proc_time(N,L,avg, outputDirName);
  // cerr <<"\t\tAverage p_lk=" << avg << endl;

}




//----------------------------------------------------------------//
// Private Function:  process_dir                                 //
//----------------------------------------------------------------//

void process_dir(const char * inputDirName, const char * outputDirName) {
  int          i = 0;
  tinydir_dir  dir;
  int          nb = 0;
  
  tinydir_open_sorted(&dir, inputDirName);

  // if (online) cerr << "Online in process_dir" << endl;
  for (i = 0; i < dir.n_files; i++)
    {
      tinydir_file file;
      tinydir_readfile_n(&dir, &file, i);
      string     baseName = file.name;
      string     forbiddenName("result");
      
      if ( (baseName ==".") || (baseName =="..") || (baseName.find(forbiddenName) != string::npos) ) {
	tinydir_next(&dir);
	continue;
      }

      if ( nb < start_file ) {
	nb++;
	continue;
      }
      
      
      string     fileName = string(inputDirName) + "/" + baseName;
      
      cerr << "Processing file " << fileName << endl;

      process_file(fileName.c_str(), outputDirName, nb);
      
      nb++;
      if ( nb > end_file )
	break;
      tinydir_next(&dir);
    }

  tinydir_close(&dir);
  if ( end_file >= 100000 )
    end_file = nb;
}


//----------------------------------------------------------------//
// Private Function:  roundToN                                    //
//----------------------------------------------------------------//

char* roundToN(double x, int n)
{
    char* result = new char[n+1]; // Allocate memory for the result string
    
    int digits = n-1; // Number of digits to display after decimal point
    if (digits < 0) digits = 0;
    
    snprintf(result, n+1, "%.*f", digits, x); // Format the number as a string
    
    // Pad the result string with zeros on the right side, if necessary
    int len = strlen(result);
    for (int i = len; i < n; i++) {
        result[i] = '0';
    }
    result[n] = '\0'; // Add null terminator
    
    return result;
}



//----------------------------------------------------------------//
// Private Function:  write_output_file                           //
//----------------------------------------------------------------//

void write_output_file(const char * outputDirName) {
  int         i;
  string      startStr=int_to_string(start_file);
  string      endStr=int_to_string(end_file);
  string      outputFileName;
  string      orderFileName;
  
  //ouverture du fichier de sortie
  if (OFILE_FLAG) {
	outputFileName = string(outputDirName) + "/" + ofname;
	// orderFileName = string(outputDirName) + "/order-" + ofname;
  }
  else {
    if (sp_anticipate)
      outputFileName = string(outputDirName) + "/SpUpdate_results-"+startStr+"-"+endStr+".txt";
	  else
      outputFileName = string(outputDirName) + "/results-"+startStr+"-"+endStr+".txt";
	// orderFileName = string(outputDirName) + "/order-"+startStr+"-"+endStr+".txt";
  }
  ofstream    outFile(outputFileName.c_str());
  // ofstream    orderFile(orderFileName.c_str());  
    
  cerr << "Writing output file " << outputFileName << endl;
  cerr << "Format : # INST"
       << "\t"
       << "LB"
       << "\t"
       << "SIN"
       << "\t"
       << "SIN_T"
       << "\t"
       << "RR"
       << "\t"
       << "RO"
       << "\t"
       << "PRED"
       << "\t"
       << "time sched_ms"
       << "\t"
       << "total_time_ms"
       << endl;

  //ecriture des resultats
  for (i=0; i<INST.size(); i++) {
    outFile << fixed
	    << setprecision(3)
	    << INST[i]
	    << "\t"
	    << LB[i]
	    << "\t"
	    << SIN[i]
	    << "\t"
	    << SIN_T[i]
	    << "\t"
	    << RR[i]
	    << "\t"
	    << RANDO[i]
	    << "\t"
	    << PRED[i]
	    << "\t"
	    << time_sched[i]
	    << "\t"
	    << total_time[i]
	    << endl;
  }
  outFile.close();

  
}



void write_output_file_fig(const char * outputDirName) {
  int         n=0, i,k=0;
  double      f;
  string      freq;
  string      predStr=int_to_string(NB_predictions);
  string      startStr=int_to_string(start_file);
  string      endStr=int_to_string(end_file);
  string      metricName;
  
  if ( slot_size > 0.0 ) {
    f = 1.0/slot_size;
    freq  = double_to_string(f);
  }
  else {
    f = 1000.0;
    freq = "inf";
  }
  
  metricName="cct_pred";
  string outputFileName ;
  if (sp_anticipate)
    outputFileName = string(outputDirName) + "/A_results-"+startStr+"-"+endStr+"_"+ metricName + "_n=" + predStr + ".txt";
  else 
    outputFileName = string(outputDirName) + "/results-"+startStr+"-"+endStr+"_"+ metricName + "_n=" + predStr + ".txt";
  ofstream    outFile(outputFileName.c_str());
  
  cerr << "Writing output file " << outputFileName << endl;
  cerr << "Format : # INST"
  << "\t"
  << "DELTA"
  << "\t"
  << "MEAN"
  << "\t"
  << "MIN"
  << "\t"
  << "MAX"
  << "\t"
  << "MEDIAN"
  << "\t"
  << "STDDEV"
  << "\t"
  << "FREQ"
  << "\t"
  << "time sched_ms"
  << "\t"
  << "total_time_ms"
  << endl;

  //ecriture des resultats
  for (i=0; i<MEAN_fig[n].size(); i++) {
    outFile << fixed
      << setprecision(3)
      << INST_fig[n][i]
      << "\t"
      << DELTA_fig[n][i]
      << "\t"
      << MEAN_fig[n][i]
      << "\t"
      << MIN_fig[n][i]
      << "\t"
      << MAX_fig[n][i]
      << "\t"
      << MEDIAN_fig[n][i]
      << "\t"
      << STDDEV_fig[n][i]
      << "\t"
      << FREQ_fig[n][i]
      << "\t"
      << time_sched_fig[n][i]
      << "\t"
      << total_time_fig[n][i]
      << endl;
  }
  outFile.close();

}




//----------------------------------------------------------------//
// Private Function:   main                                       //
//----------------------------------------------------------------//

int main(int argc, char **argv)
{
  double          f;
  char          * inputDirName;
  char          * outputDirName;
  int             index, c;

  //lecture des arguments
  opterr = 0;
  while ((c = getopt (argc, argv, "a:b:f:l:m:s:c:z:ohtu")) != -1)
    switch (c)
      {
      case 'o':
        online = true;
        break;
      case 'u': // SP Anticipate
        sp_anticipate = true;
        break;
      case 'a':
        start_file = atoi(optarg);
        break;
      case 'b':
        end_file = atoi(optarg);
        break;
      case 'f':
        f = atof(optarg);
	if ( f > 0.0 )
	  slot_size = 1.0/f;
	else
	  slot_size = 0.0;
        break;
      case 'l':
        lambda = atof(optarg);
        break;
      case 'm':
        mean_flow_size = atof(optarg);
        break;
      case 's':
        stdev_flow_size = atof(optarg);
        break;
      case 'c':
      switch(optarg[0])
      {
        case 'a':
          choix = ALL;
          break;
        case 'c':
          choix = CL;
          break;
        case 't': // clairvoyant with release time
          choix = CRT;
          break;
        case 'n':
          choix = NC;
          break;
        case 'r':
          choix = R; // ratio : pred/clairvoyant
          break;
        case 'p':
          choix = RO;
          break;
        case 'o':
          choix = Opt;
          break;
        default:
          choix = ALL;
          break;
        }
        break;
      case 'z':
        OFILE_FLAG=true;
	ofname = optarg;	
	break;
      // case 'u':
      //   sp_anticipate = true;
      // break;
      case 'h':
	cerr << "Usage : exec -o -u -a x1 -b x2 -f x2 -l x3 -m x4 -s x5 inputDirName outputDirName" << endl;
	cerr << "\tOption -o is to run the simulation in online mode" << endl;
  cerr << "\tOption -u is to run the simulation of online mode with anticipate intervalle" << endl;
	cerr << "\tx1 is the number of the first instance (file) to be processed" << endl;
	cerr << "\tx2 is the number of the last instance (file) to be processed" << endl;
	cerr << "\tx2 is the frequency f of updates in online mode" << endl;
	cerr << "\tx3 is the value of the arrival rate lambda in online mode" << endl;
	cerr << "\tx4 is the mean flow size (-1 for FB)" << endl;
	cerr << "\tx5 is the standard deviation of flow sizes" << endl;
	cerr << "\tinputDirName is the name of the directory where files are located" << endl;
	cerr << "\toutputDirName is the name of the directory where results are stored" << endl;
	return 0;	
      case '?':
        if ((optopt == 'I')||(optopt == 'n')||(optopt == 'l')||(optopt == 'f')||(optopt == 'm')||(optopt == 's'))
          fprintf (stderr, "Option -%c requires an argument.\n", optopt);
        else if (isprint (optopt))
          fprintf (stderr, "Unknown option `-%c'.\n", optopt);
        else
          fprintf (stderr,
                   "Unknown option character `\\x%x'.\n",
                   optopt);
        return 1;
      default:
        abort ();
      }

  inputDirName = argv[optind];
  outputDirName = argv[optind+1];
  

  //affichage
  if ( online ){
    cerr <<"\t\t ONLINE Mode activate" <<endl;
    cerr << "Running instances in " << inputDirName << " in online mode with freq=" << f << ", lambda=" << lambda << endl;
  }
  else
    cerr << "Running instances in " << inputDirName << " in offline mode" << endl;
  if ( (start_file>=0) && (end_file<=100000) )
    cerr << "Number of instances: from instance " << start_file << " to instance " << end_file << endl;
  else
    cerr << "Number of instances: all files." << endl;

  //affichage
  cerr << "Stochastic coflow scheduling" << endl;

  // //traitement des instances
  // process_dir(inputDirName, outputDirName);

  // //ecriture du fichier
  // write_output_file(outputDirName); 
  if (choix != NC){
    process_dir(inputDirName, outputDirName);

    //ecriture du fichier
    write_output_file(outputDirName); 
  }else{
    //****************************Predictions**************************** */
    //allocation memoire
    INST_fig = new vector<int>[3];
    DELTA_fig = new vector<double>[3];
    MEAN_fig = new vector<double>[3];
    MIN_fig = new vector<double>[3];
    MAX_fig = new vector<double>[3];
    MEDIAN_fig = new vector<double>[3];
    STDDEV_fig = new vector<double>[3];
    FREQ_fig = new vector<double>[3];
    time_sched_fig = new vector<long double>[3];
    total_time_fig = new vector<long double>[3];
    //traitement des instances
    process_dir(inputDirName, outputDirName);

    //ecriture du fichier
    write_output_file_fig(outputDirName); 
    
    //liberation memoire
    delete [] INST_fig;
    delete [] DELTA_fig;
    delete [] MEAN_fig;
    delete [] MIN_fig;
    delete [] MAX_fig;
    delete [] MEDIAN_fig;
    delete [] STDDEV_fig;
    delete [] FREQ_fig;
    delete [] time_sched_fig;
    delete [] total_time_fig;
    //******************************************************** */
  }
  
  // write the aeverage processing time
  // with()
  
  return 1;
}

