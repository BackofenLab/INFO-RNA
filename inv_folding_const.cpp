/***************************************************************************
                       inv_folding_const.cpp  -  main
                             -------------------
    begin                : 30-08-2004
    copyright            : (C) 2004 by Anke Busch
    email                : anke.busch@informatik.uni-freiburg.de
 ***************************************************************************/

#include <iostream>
#include <stdlib.h>

#include "basics.h"
#include "constraints.h"
#include "struct.h"
#include "inverse.h"
#include "search.h"

using namespace std;


/**********************************************************************************
*    Constants                                                                   *
*          defined in "basics.h"
**********************************************************************************/

const int MAX_LOOPS = 2000; //max. number of loops

const double T                    = 310.15  ; // temperature in Kelvin(37C);K=C+273.15
const double RT                   = 0.616   ; // Boltzmann gas constant in kcal/mol
// RT=kT=1.380622*10^(-23)Joule/K *310.15K (37C=273.15K+37K=310.15K)

const double MIN_DOUBLE = -1000000.0;
const double MAX_DOUBLE =  1000000.0;
const double DOUBLE_DIFF = 0.00001;

const int MIN_INT = -1000000;
const int MAX_INT = 1000000;

/**********************************************************************************
*                               global variabels                                  *
**********************************************************************************/
int step;   //is 1 if we are in the initialization step, 2 during the local search (the variable is necessary since constraint mismatches
            //are allowed only during the second step, but the functions with allow them (PairPenaly, BasePenalty) are used during the
            //initialization as well

char* brackets;  //Structure given in bracket notation
int struct_len;  //size of the structure
char *ElementStruct;  // another representation of the structure (incl. elements like S, M, B,...)
int** BP_Order;  //order of the base pairs (BPs) in which they are examined
int numBP;       //number of base pairs
int*  BP_Pos_Nr; //vector in which for each pos. in brackets the pos. in BP_Order is stored

double** D;    //Dynamic recursion matrix
int**** Trace; //Dynamic Traceback Matrix (has 2 dimensions more than D, since in each D-corresponding field the coordinates of all
               // predecessor fields have to be stored (two coord. per predecessor) and since there could be more than one predecessors
               // 2 more dimensions are needed)

char* iupac_const; //sequence constraints in IUPAC Code. If not given by the user, iupac_const is set to NNNNNN....
int** seq_constraints; //IUPAC constraints translated in boolean constraint, i.e. it is a 2dim. array with as many
                       //rows as sequence positions exist and 4 columns per row represention the possible base assignments,
                       //the entries are set to 0 (base not allowed) or 1 (allowed) (0 = A, 1 = C, 2 = G, 3 = U)
int max_mis; //max. number of mismatches that are allowed among the constrained position or in an interval
int num_mis; //counter for occuring mismatches
int* mis_vec; //vector where for each position is stored whether a mismatch is allowed or not

char* best_char_seq;   // designed sequence
bool free_bases_set2A; // is true, if free bases have to be set to 'A'
bool free_bases_set2C;
bool free_bases_set2G;
bool free_bases_set2U;
bool random_init;      // true, if the initializing sequence should be designed with random and not optimal

int search_strategy;       // gives the search strategy ( 1 = adaptive walk, 2 = full local search, 3 = stochastic local search)
int neighbour_choice;      // gives the kind of ranking of the neighbors (1 = random, 2 = energy dependent)
int only_mutation_is_step; // gives the information how to count the step done during the stochastic local search
int step_multiplier;       // maximal number of steps during SLS = step_multiplier * length
double p_accept;           //probability to accept worse neighbors during SLS



/**********************************************************************************
*                        help for calling the program                             *
**********************************************************************************/

void usage(char* name)
{
   cout << "\ncall: " << name << " \"structure\" [-c \"constraints\"] \n";
   cout << "                   [-v \"allowed mismatches (binary vector)\"]\n";
   cout << "                   [-n max. number of allowed mismatches] [-r random init]\n";
   cout << "                   [-f[ACGUMRWSYKVHDBN] assignment where free bases are set to]\n";
   cout << "                   [-F[mp]] [-R [repeats]] [-S search strategy] [-m]\n";
   cout << "                   [-s length multiplier for max. number of steps during SLS] \n";
   cout << "                   [-N neighbor choice] [-p prob. to accept worse neighbors]\n\n";
   exit(1);
}


void usage_help(char* name)
{
   cout << "\ncall: " << name << " \"structure\" [-c \"constraints\"] \n";
   cout << "                   [-v \"allowed mismatches (binary vector)\"]\n";
   cout << "                   [-n max. number of allowed mismatches] [-r random init]\n";
   cout << "                   [-f[ACGUMRWSYKVHDBN] assignment where free bases are set to]\n";
   cout << "                   [-F[mp]] [-R [repeats]] [-S search strategy] [-m]\n";
   cout << "                   [-s length multiplier for max. number of steps during SLS] \n";
   cout << "                   [-N neighbor choice] [-p prob. to accept worse neighbors]\n\n";
   cout << endl;
   cout << "\nGeneral options: \n";
   cout << "---------------------\n";
   cout << " -c \"constraints\"\t Constraining sequence given in IUPAC code.\n";
   cout << "\nOptions of the initializing part:\n";
   cout << "-----------------------------------\n";
   cout << " -r \t\t random initializing sequence\n";
   cout << " -f[ACGUMRWSYKVHDBN]\t Give the assignment (in IUPAC code) where free bases\n";
   cout << " \t\t\t in loop regions (that give no energy fraction) are\n";
   cout << " \t\t\t set to.\n"; 
   cout << "\nOptions of the local search part:\n";
   cout << "-----------------------------------\n";
   cout << " -v \"allowed mismatches\" \t binary vector,\n";
   cout << " \t\t\t\t \"1\" means a mismatch conc. the constraints is allowed,\n";
   cout << " \t\t\t\t \"0\" means a mismatch is forbidden\n";
   cout << endl;
   cout << " -n max. mismatch number\t Maximal number of allowed mismatches in a given\n";
   cout << "\t\t\t\t interval. If nothing further is given, the interval \n";
   cout << "\t\t\t\t includes the whole sequence.\n";
   cout << endl;
   cout << " -F[mp]\t\t Use minimum energy (-Fm) (default), partition function\n";
   cout << " \t\t folding (-Fp) or both (-Fmp).\n";
   cout << endl;
   cout << " -R[repeats]\t Number of repeating the local search step.\n";
   cout << endl;
   cout << " -S strategy\t Search strategy used during the local search step: \n";
   cout << "                            1 - adaptive walk\n";
   cout << "                            2 - full local search\n";
   cout << "                            3 - stochastic local search (default)\n";
   cout << endl;
   cout << " -m\t\t Kind of counting the step during the local search. Here, only\n";
   cout << " \t\t accepted mutations are counted, while usually all tested \n";
   cout << " \t\t neighbors are counted.\n";
   cout << endl;
   cout << " -s multiplier\t The maximal number of step during the stochastic local\n";
   cout << " \t\t search is given as product of the structure length and a \n";
   cout << " \t\t multiplier. The multiplier is set to 10 by default.\n";
   cout << endl;
   cout << " -N neighbor choice\t Kind of order the candidate neighbors during the local\n";
   cout << "\t\t\t search.\n";
   cout << "                            1 - randomly\n";
   cout << "                            2 - energy dependent (default)\n";
   cout << endl;
   cout << " -p probability\t Probability to accept worse neighbors during the stochastic\n";
   cout << " \t\t local search. It is set to 0.1 by default.\n";
   cout << endl;

   exit(0);
}


/**********************************************************************************
*         Print the input and the results of the initializing step                *
**********************************************************************************/

void print_in_and_output(double best_energy)
{
   printf("\n=========================\n");
   printf("Initializing Step: \n");
   printf("=========================\n");
   printf("Wanted Structure    : %s\n",brackets);
   printf("Sequence Constraints: %s\n", iupac_const);
   printf("Allowed Mismatches  : ");
   for (int i=0; i<struct_len; i++)
      printf("%d", mis_vec[i]);
   printf("\nDesigned Sequence   : %s\n",best_char_seq);
   printf("designed randomly   : %d\n", random_init);
   //printf("Energy: %1.2f\n", best_energy);
   printf("\n=========================\n");
   printf("Local Search Results: \n");
   printf("=========================\n");
}


/**********************************************************************************
*                                 Main-Function                                   *
**********************************************************************************/

int main(int argc, char *argv[])
{
   char *rstart, *str2;
   int hd, mfe = 1, pf = 0, repeat = 0, found;
   double energy = 0.0, kT;
   bool constraints_given = false; //mismatches in constraints are just useful if constraints are given at all. thus here the reminder
                                   //whether constraints are given
   char* mis_vec_char = NULL;
   
   //random generator init
   long sec;
   time(&sec);
   srand((unsigned)sec);

   step = 1;
   random_init = 0;
   max_mis = -1;
   num_mis = 0;

   search_strategy = 1;
   neighbour_choice = 1;
   only_mutation_is_step = 0;
   step_multiplier = 10;
   p_accept = 0.1;

   do_backtrack = 0;

   free_bases_set2A = 0;
   free_bases_set2C = 0;
   free_bases_set2G = 0;
   free_bases_set2U = 0;

   if (argc < 2)
   {
      usage(argv[0]);
      exit(-1);
   }

   if ((argv[1][0] == '-') && (argv[1][1] == 'h'))
   {
      usage_help(argv[0]);
      exit(0);
   }
   else
      brackets = argv[1];

   for (int i = 2; i<argc; i++)
   {
      if (argv[i][0] == '-')
         switch (argv[i][1])
         {
            case 'r': random_init = 1;
                      break;
            case 'f': //i++;
                      //if ( i<argc )
                      if ((int)strlen(argv[i]) > 2)
                      {
                         for(int j=2; j<(int)strlen(argv[i]); j++)
                            switch( argv[i][j] )
                            {
                               case 'A' :  free_bases_set2A = 1;
                                           //cout << endl << "free_bases_set2A: " << free_bases_set2A;
                                           break;
                               case 'C' :  free_bases_set2C = 1;
                                           //cout << endl << "free_bases_set2C: " << free_bases_set2C;
                                           break;
                               case 'G' :  free_bases_set2G = 1;
                                           //cout << endl << "free_bases_set2G: " << free_bases_set2G;
                                           break;
                               case 'U' :  free_bases_set2U = 1;
                                           //cout << endl << "free_bases_set2U: " << free_bases_set2U;
                                           break;
                               case 'M' :  free_bases_set2A = 1;
                                           free_bases_set2C = 1;
                                           break;
                               case 'R' :  free_bases_set2A = 1;
                                           free_bases_set2G = 1;
                                           break;
                               case 'W' :  free_bases_set2A = 1;
                                           free_bases_set2U = 1;
                                           break;
                               case 'S' :  free_bases_set2C = 1;
                                           free_bases_set2G = 1;
                                           break;
                               case 'Y' :  free_bases_set2C = 1;
                                           free_bases_set2U = 1;
                                           break;
                               case 'K' :  free_bases_set2G = 1;
                                           free_bases_set2U = 1;
                                           break;
                               case 'V' :  free_bases_set2A = 1;
                                           free_bases_set2C = 1;
                                           free_bases_set2G = 1;
                                           break;
                               case 'H' :  free_bases_set2A = 1;
                                           free_bases_set2C = 1;
                                           free_bases_set2U = 1;
                                           break;
                               case 'D' :  free_bases_set2A = 1;
                                           free_bases_set2G = 1;
                                           free_bases_set2U = 1;
                                           break;
                               case 'B' :  free_bases_set2C = 1;
                                           free_bases_set2G = 1;
                                           free_bases_set2U = 1;
                                           break;
                               case 'N' :  free_bases_set2A = 1;
                                           free_bases_set2C = 1;
                                           free_bases_set2G = 1;
                                           free_bases_set2U = 1;
                                           break;
                               default : usage(argv[0]);
                            }
                         //cout << endl << "free_bases_set2A: " << free_bases_set2A;
                         //cout << endl << "free_bases_set2C: " << free_bases_set2C;
                         //cout << endl << "free_bases_set2G: " << free_bases_set2G;
                         //cout << endl << "free_bases_set2U: " << free_bases_set2U;
                      }
                      else
                         usage(argv[0]);
                      break;
            case 'c': i++;
                      iupac_const = argv[i];
                      constraints_given = true;
                      if ((iupac_const == NULL) || (iupac_const[0] == '-'))
                      {
                         printf("\nThe constraints are missing!\n");
                         usage(argv[0]);
                      }
                      //printf("iu: %s\n", iupac_const);
                      break;
            case 'v': i++;
                      mis_vec_char = argv[i];
                      if ((mis_vec_char == NULL) || (mis_vec_char[0] == '-'))
                      {
                         printf("\nThe allowed mismatches are missing!\n");
                         usage(argv[0]);
                      }
                      //printf("mis: %s\n", mis_vec_char);
                      break;
            case 'F': if ((int)strlen(argv[i]) > 2)
                      {
                         mfe = 0; pf = 0;
                         for(int j=2;j<(int)strlen(argv[i]);j++)
                         {
                            switch( argv[i][j] )
                            {
                               case 'm' :  mfe = 1;
                                           break;
                               case 'p' :  pf = 1; /* old version had dangles=0 here */
                                           break;
                               default : usage(argv[0]);
                            }
                         }
                      }
                      else
                         usage(argv[0]);
                      break;
            case 'R': repeat = 100; //REPEAT_DEFAULT;
                      if(++i<argc)
                         if (sscanf(argv[i], "%d", &repeat)==0)
                            usage(argv[0]);
                      break;
            case 'S': if (argv[i][2]!='\0')
                         usage(argv[0]);
                      if (sscanf(argv[++i], "%d", &search_strategy)==0)
                         usage(argv[0]);
                      break;
            case 'N': if (argv[i][2]!='\0')
                         usage(argv[0]);
                      if (sscanf(argv[++i], "%d", &neighbour_choice)==0)
                         usage(argv[0]);
                      break;
            case 's': if (argv[i][2]!='\0')
                         usage(argv[0]);
                      if (sscanf(argv[++i], "%d", &step_multiplier)==0)
                         usage(argv[0]);
                      break;
            case 'p': if (argv[i][2]!='\0')
                         usage(argv[0]);
                      //p_accept = atof(argv[++i]);
                      if (sscanf(argv[++i], "%lf", &p_accept)==0)
                         usage(argv[0]);
                      break;
            case 'm': only_mutation_is_step = 1;
                      break;
            case 'n': if (argv[i][2]!='\0')
                         usage(argv[0]);
                      if (sscanf(argv[++i], "%d", &max_mis)==0)
                         usage(argv[0]);
                      break;
            default : usage(argv[0]);
         }
      else
         usage(argv[0]);
   }


   if ((search_strategy > 3) || (search_strategy < 1))
   {
      printf("\nThe search strategy is not valid.\n");
      exit(1);
   }

   if ((neighbour_choice > 2) || (neighbour_choice < 1))
   {
      printf("\nThe choice of the neighbours is not valid.\n");
      exit(1);
   }


   //if no constraints are given, set to NNNNN.... :
   //*********************************************************
   if (iupac_const == NULL)
   {
      iupac_const = (char*) malloc(sizeof(char)*((int)strlen(brackets)+1));
      for (int i=0; i<(int)strlen(brackets); i++)
         iupac_const[i] = 'N';
      iupac_const[(int)strlen(brackets)] = '\0';
   }

   //test, whether the structure is valid:
   //***************************************
   int correct = check_brackets(brackets);
   numBP = NumOfBP(brackets);
   struct_len = (int)strlen(brackets);

   if (correct)
   {
      getOrder(brackets);
      Pos2BP_Pos();
   }
   else
   {
      cerr << "\nNo valid structure!\n\n";
      exit(1);
   }

   //test, whether brackets and iupac_const have the same length
   //*****************************************************************************
   if (struct_len != (int)strlen(iupac_const))
   {
      cerr << "\nThe structure and the constraint vector must have the same length!\n\n";
      exit(1);
   }

   //store mismatch information in an integer vector and look for errors
   //********************************************************************
   mis_vec = (int*)malloc(sizeof(int)*struct_len);

   //if no allowed mismatches are given, set mis_vec to "0" everywhere and max_mis to "0", too
   if (mis_vec_char == NULL)
   {
      for (int i=0; i<struct_len; i++)
         mis_vec[i] = 0;
      max_mis = 0;
   }
   else
   {
      //test, whether brackets and mis_vec_char have the same length
      if (struct_len != (int)strlen(mis_vec_char))
      {
         cerr << "\nThe structure and the mismatch vector must have the same length!\n\n";
         exit(1);
      }

      //if no constraints are given but allowed mismatches, ==> constraints are missing
      if (constraints_given == false)
      {
         cerr << "\nAllowed mismatches make no sense without constraints!\n\n";
         exit(1);
      }

      //if some mismatches are allowed (conc. mis_vec_char) but the maximal number is not given or set to 0 => makes no sense
      if (max_mis < 0)
      {
         cerr << "\nThe maximal number of mismatches is not given but a vector of allowed mismatch positions!\n\n";
         exit(1);
      }

      //set the integer vector
      for (int i=0; i<(int)strlen(mis_vec_char); i++)
      {
         if (mis_vec_char[i] == '0')
            mis_vec[i] = 0;
         else if (mis_vec_char[i] == '1')
            mis_vec[i] = 1;
         else
         {
            cerr << endl << mis_vec_char[i] << " is not valid in the binary code for allowed mismatches!\n\n";
            exit(1);
         }
      }
   }


   //test, whether constraints are valid and create constraint array
   //********************************************************************
   int correct_iu = Check_iu();
   int correct_bp = Check_constraints_bp();

   if (correct_iu)
   {
      if (correct_bp)
         getSeqConstraints();
   }
   else
   {
      cerr << "\nNo valid IUPAC Code!\n\n";
      exit(1);
   }

   //****************************************************
   //****************************************************

   //Initialization:
   //***************
   double x;
   
   if (random_init == 1)
      x = Random_Init();
   else   
      x = Recursion();

   print_in_and_output(x);

   //****************************************************
   //****************************************************

   //Local Search:
   //*************
   step = 2;
   double min_en;
   char* test_str;
   test_str = (char*) malloc(sizeof(char)*(struct_len+1));

   init_rand();
   kT = (temperature+273.15)*1.98717/1000.0;
   give_up = (repeat<0);

   str2 = (char *) malloc(sizeof(char)*((unsigned)struct_len+1));

   if (repeat!=0) found = (repeat>0)? repeat : (-repeat);
   else found = 1;

   initialize_fold(struct_len);
   rstart = (char *) malloc(sizeof(char)*((unsigned)struct_len+1));

   while(found>0) 
   {
      char *string;
      string = (char *) malloc(sizeof(char)*((unsigned)struct_len+1));
      strcpy(string, best_char_seq);
      strcpy(rstart, string); /* remember start string */

      if (mfe)
      {
         energy = inverse_fold(string);
         min_en = fold(string, test_str);
         if( (repeat>=0) || (energy<=0.0) )
         {
            found--;
            hd = hamming(rstart, string);
            printf("MFE:    %s  %3d  (%4.2f)", string, hd, min_en);
            if (energy>0) /* no solution found */
            {
               printf("   d= %g\n", energy);
               energy = fold(string,str2);
               printf("NO_MFE: %s\n", str2);
            }
            else
               printf("\n");
            printf("number of mismatches: %d\n", num_mis);
            
          }
      }

      if (pf)
      {
         if (!(mfe && give_up && (energy>0)))
         {
            /* unless we gave up in the mfe part */
            double prob, min_en, sfact=1.07;

            /* get a reasonable pf_scale */
            min_en = fold(string,str2);
            pf_scale = exp(-(sfact*min_en)/kT/struct_len);
            init_pf_fold(struct_len);

            energy = inverse_pf_fold(string);
            prob = exp(-energy/kT);
            hd = hamming(rstart, string);
            min_en = fold(string,test_str);
            printf("PF:     %s  %3d  (%g)  (%4.2f)\n", string, hd, prob,min_en);
            printf("number of mismatches: %d\n", num_mis);
            free_pf_arrays();
         }
         if (! (mfe))
            found--;
      }
      free(string);
      num_mis = 0;
   }
   free(rstart);
   free_arrays();
   free(str2);

   printf("\n");
   //****************************************************
   //****************************************************
   return 0;
}



