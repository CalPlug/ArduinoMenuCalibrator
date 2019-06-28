/*
  Regressioncalc.h - Library for calculating regression on Arduino
  Released into the public domain.
*/
#ifndef Regressioncalc_h
#define Regressioncalc_h

#include "Arduino.h"

class Regressioncalc
{
  public:
   Regressioncalc();
   void adddatapoint(double x, double y);

  private:
    int _EEPROMaddress;
};

#endif