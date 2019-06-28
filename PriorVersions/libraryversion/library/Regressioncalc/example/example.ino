//Example Arduino Calculator Tool

#include <Arduino.h>
#include <Regressioncalc.h>
//#include <stdbool.h>
#include <math.h>
#include "polifitgsl.h" //Reference to C code Library



Regressioncalc myRegressioncalc(); //declare object

void setup() {

  //Example with degree 3 polynomical regression:  https://rosettacode.org/wiki/Polynomial_regression#C
  #define NP 11
  #define DEGREE 3
  
  double x[] = {0,  1,  2,  3,  4,  5,  6,   7,   8,   9,   10};
  double y[] = {1,  6,  17, 34, 57, 86, 121, 162, 209, 262, 321};
  
  double coeff[DEGREE];
  int i;
  bool returnedvalue;
   
   returnedvalue=polynomialfit(NP, DEGREE, x, y, coeff);
    for(i=0; i < DEGREE; i++) 
      {
        printf("%lf\n", coeff[i]);
      }
}


void loop() {

}
