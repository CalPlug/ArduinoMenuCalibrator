// Helper function for calibration of ADE9078 by Avinash Pai with contributions cited.
#include <math.h>
#include <EEPROM.h>                      
#define ARDBUFFER 16
#include <stdarg.h>
#include <Arduino.h>

#define buffSize 16
char inputSeveral[buffSize]; // schar array for input function below
                             // space for 16 chars and a terminator

#define maxChars 12 // a shorter limit to make it easier to see what happens
                   //  if too many chars are entered

#define regressionPrecision 5  
#define reportingPrecision 4 

//EEPROM Configuration Settings
#define reportToEEPROM 1  // enable this, make value equal to 1 to allow recording values to the EEPROM after regression
#define reportInvertedValues 0  //  store values in EEPROM as the inverse, often used for very small decimals
#define EEPROMVariableBufferSize 25 
#define EEPROMVariableLength 12
#define EEPROMDecimalPrecision 8  
int EEPROMCurrentPosition = 0;
int offsetInEEPROM = 0;  // offset address to begin writing in EEPROM
bool reportConfigured = 1; 
bool reportedConfiguredStatus = 1;  // updated status for configure
char invertedStatus[3];
char configuredStatus[3];

double* px;       // dynamic array for x's (DAQ system values)
double* py;       // dynamic array for y's (calibrated values)
double* pyregress;  // local calculated regression values for each regression type

uint8_t totalPoints = 0;  //declare default case of 0 points

// NOTE: DELAYS TEMPORARY - WHILE LOOPS FOR INPUT NOT WORKING
// NOTE: USING 86% MEMORY ON ARDUINO UNO 
void setup() {
  Serial.begin(115200);
  Serial.println(F("Starting..."));

  // Menu select for function to fit against
  Serial.println(F("Select fit: "));
  Serial.println(F("  (1)Linear - Minimum two points"));
  Serial.println(F("  (2)Quadratic - Minimum three points"));
  Serial.println(F("  (3)Exponential - Minimum three points, y != 0"));  // Double check restrictions on exp, log, power
  Serial.println(F("  (4)Logarithmic - Minimum three points, x != 0"));
  Serial.println(F("  (5)Power - Minimum three points, x != 0"));
  Serial.println(F("  (0)Exit"));
  delay(3000);
  readSeveralChars();
  uint8_t fitChoice = atoi(inputSeveral);

  // Exit
  if (fitChoice == 0)
  {
      Serial.print(F("Exiting calibration process..."));
      delay(2000);
      exit(0);
  }
  // Linear
  if(fitChoice == 1) { 
    Serial.println(F("Fit Chosen: Linear"));

    Serial.print(F("Input total points: "));   // prompt user
    delay(2000);                            // delay for input (while (Serial.avaiable()) causes char array to become zero and instantly changes input variable to 0)
    readSeveralChars();
    totalPoints = atoi(inputSeveral);   // converts char array to int
    Serial.println(totalPoints);
    Serial.println(F("NOTE - X's are DAQ system values measured, Y's are final unit calibrated values"));

    // Error and warning checks for minimum poiints
    if (totalPoints < 2)
    {
        Serial.print(F("At least two points needed for linear. Restarting calibration process..."));
        delay(2000);
        setup();
    }
    else if (totalPoints == 2)
    {
       Serial.println(F("WARNING - Minimum points met. Overdefined recommended."));
      
    }
    delay(3000);
    px = new double[totalPoints]; // Load x's into array
    py = new double[totalPoints]; // Load y's into array
    for (uint8_t i = 0; i < totalPoints; ++i)       // loop through arrays and fill in values by input
    {
      ardprintf("Input x%d", i+1);        // printf for serial, function implemented below
      delay(2000);
      readSeveralChars();
      px[i] = atof(inputSeveral);
      Serial.println(px[i]);
      delay(1000);

      ardprintf("Input y%d", i+1);
      delay(2000);
      readSeveralChars();
      py[i] = atof(inputSeveral);
      Serial.println(py[i]);
      delay(1000);
    }
    fabls_linear(totalPoints, px, py); // send inputed points to fabls calculator 
  }
  // Quadratic
  else if (fitChoice == 2) {
    Serial.println("Fit Chosen: Quadratic");

    Serial.print(F("Input total points: "));
    delay(2000);
    readSeveralChars();
    totalPoints = atoi(inputSeveral);
    Serial.println(totalPoints);

    if (totalPoints < 3)
    {
        Serial.print(F("At least three points needed for quadratic. Restarting calibration process..."));
        delay(2000);
        setup();
    }
    else if (totalPoints == 3)
    {
       Serial.println(F("WARNING - Minimum points met. Overdefined recommended."));
      
    }
    delay(3000);
     px = new double[totalPoints];
     py = new double[totalPoints];
    for (uint8_t i = 0; i < totalPoints; ++i)
    {
      ardprintf("Input x%d", i+1);
      delay(2000);
      readSeveralChars();
      px[i] = atof(inputSeveral);
      Serial.println(px[i]);
      delay(1000);

      ardprintf("Input y%d", i+1);
      delay(2000);
      readSeveralChars();
      py[i] = atof(inputSeveral);
      Serial.println(py[i]);
      delay(1000);
    }
    fabls_quad(totalPoints, px, py); // calculate quadratic regression
  }
  // Exponential
  else if (fitChoice == 3) {
    Serial.println(F("Fit Chosen: Exponential"));

    Serial.print(F("Input total points: "));
    delay(2000);
    readSeveralChars();
    totalPoints = atoi(inputSeveral);
    Serial.println(totalPoints);

    if (totalPoints < 3)
    {
        Serial.print(F("ERROR - At least three points needed for exponential. Restarting calibration process..."));
        delay(2000);
        setup();
    }
    else if (totalPoints == 3)
    {
       Serial.println(F("WARNING - Minimum points met. Overdefined recommended."));
      
    }
    delay(3000);
     px = new double[totalPoints];
     py = new double[totalPoints];
    for (uint8_t i = 0; i < totalPoints; ++i)
    {
      ardprintf("Input x%d", i+1);
      delay(2000);
      readSeveralChars();
      px[i] = atof(inputSeveral);
      Serial.println(px[i]);
      delay(1000);

      ardprintf("Input y%d", i+1);
      delay(2000);
      readSeveralChars();
      py[i] = atof(inputSeveral);
      Serial.println(py[i]);
      if (py[i] == 0)         // Catch zero point errors
      {
          Serial.println(F("ERROR - y's cannot be zero for exponential. Restarting calibration process... "));
          delay(2000);
          setup();
      }
      delay(1000);
    }
    fabls_exp(totalPoints, px, py); //calculate exponential regression   
  }
  // Logarithmic
  else if (fitChoice == 4) {
    Serial.println(F("Fit Chosen: Logarithmic"));

    Serial.print(F("Input total points: "));
    delay(2000);
    readSeveralChars();
    totalPoints = atoi(inputSeveral);
    Serial.println(totalPoints);

    if (totalPoints < 3)
    {
        Serial.print(F("At least three points needed for logarithmic. Restarting calibration process..."));
        delay(2000);
        setup();
    }
    else if (totalPoints == 3)
    {
       Serial.println(F("WARNING - Minimum points met. Overdefined recommended."));
      
    }
    delay(3000);
     px = new double[totalPoints];
     py = new double[totalPoints];
    for (uint8_t i = 0; i < totalPoints; ++i)
    {
      ardprintf("Input x%d", i+1);
      delay(2000);
      readSeveralChars();
      px[i] = atof(inputSeveral);
      Serial.println(px[i]);
      if (px[i] == 0)
      {
          Serial.println(F("ERROR - x's cannot be zero for logarthimic. Restarting calibration process..."));
          delay(2000);
          setup();
      }
      delay(1000);

      ardprintf("Input y%d", i+1);
      delay(2000);
      readSeveralChars();
      py[i] = atof(inputSeveral);
      Serial.println(py[i]);
      delay(1000);
    }
    fabls_log(totalPoints, px, py); //calculate logarithmic regressions
  }
  // Power
  else if (fitChoice == 5) {
    Serial.println(F("Fit Chosen: Power"));

    Serial.print(F("Input total points: "));
    delay(2000);
    readSeveralChars();
    totalPoints = atoi(inputSeveral);
    Serial.println(totalPoints);

    if (totalPoints < 3)
    {
        Serial.print(F("At least three points needed for power. Restarting calibration process..."));
        delay(2000);
        setup();
    }
    else if (totalPoints == 3)
    {
       Serial.println(F("WARNING - Minimum points met. Overdefined recommended."));
      
    }
    delay(3000);
     px = new double[totalPoints];
     py = new double[totalPoints];
    for (uint8_t i = 0; i < totalPoints; ++i)
    {
      ardprintf("Input x%d", i+1);
      delay(2000);
      readSeveralChars();
      px[i] = atof(inputSeveral);
      Serial.println(px[i]);
      if (px[i] == 0)
      {
        Serial.println(F("ERROR - x's cannot be zero for power. Restarting calibration process..."));
        delay(2000);
        setup();
      }
      delay(1000);

      ardprintf("Input y%d", i+1);
      delay(2000);
      readSeveralChars();
      py[i] = atof(inputSeveral);
      Serial.println(py[i]);
      delay(1000);
    }
    fabls_power(totalPoints, px, py); //calculate power regressions
  }
  // Invalid
  else {
    Serial.println(F("Invalid choice. Restarting calibration process..."));
    delay(2000);
    setup();  // Restart, jumps backs to beginning
  }

  // deallocation (move to after EEPROM)
  delete[] px;
  delete[] py;
  delete[] pyregress;
  
  // LOAD EEPROM
  // Once input points are given and regression data is returned
  // prompt user to send new calibration values to EEPROM
}


void loop() {
  // None
}




// https://forum.arduino.cc/index.php?topic=96292.0
// Fit Analysis By Least Squares 
double alog(double x)
{  return (x < 0) ? -log(-x) : ((x > 0) ? log(x) : 0);
}

void fabls_linear(unsigned int n,double *px,double *py)
{  
   pyregress = new double[totalPoints];
   byte mask='\x00',sign,sign2;
   unsigned int i;
   int least=-1;
   double beta,d2,denom,dy,p,percent_error,r=(n-1),x,y,yc;
   double a1,a2,a3,s,s1,s2,s3,s4,s5,s6,s7,z[5];
   byte *f = "%f %f %f %f %f\n";

   s1 = s2 = s3 = s4 = s = 0;
   for (i=0; i<n; i++)
   {  x = px[i];
      y = py[i];
      s1 += x;
      s2 += x * x;
      s3 += y;
      s4 += x * y;
   }
   if (denom = n * s2 - s1 * s1)
   {  a1 = (s3 * s2 - s1 * s4) / denom;
      a2 = (n  * s4 - s3 * s1) / denom;
      for (i=0; i<n; i++)
      {  dy = py[i] - (a2 * px[i] + a1);
         s += dy * dy;
      }
      s = sqrt(s / r);
      sign = (a1 < 0) ? '-' : '+';
      ardprintf(regressionPrecision, "Linear:   y = (%f) x %c %f; s = %f\n",a2,sign,fabs(a1),s);
      mask |= '\x01';
      z[0] = s;
   }

   Serial.print(F("X"));
   Serial.print(F("         Y"));
   Serial.print(F("         Calculated Y"));
   Serial.print(F("       Absolute Error"));
   Serial.println(F("     PercentError%"));
   for (unsigned int i = 0; i < n; ++i)
   {
      double y = (a2) * px[i] + (a1);
      double absoluteError = y - py[i]; 

      pyregress[i] = y;
      
      double error = safeDiv((y - py[i]), py[i])*100.0;
      ardprintf(reportingPrecision, "%f      %f      %f                %f            %f", px[i], py[i], y, absoluteError, error);
   }
   double rSquaredReturn;
   double adjRSquaredReturn;
   determinationCoefficient(n, py, pyregress, 1, rSquaredReturn, adjRSquaredReturn); // calcuate and print correlation coefficient

   Serial.print(F("r^2 = "));
   Serial.println(rSquaredReturn, reportingPrecision);
   Serial.print(F("adjusted r^2 = "));
   Serial.println(adjRSquaredReturn, reportingPrecision); // this is not true, just a place-holder for now
   
   if(reportToEEPROM == 1)
   {
      int expressionTotalTerms = 2;
      char expressionTerms[4] = {0};
      char constant[EEPROMVariableBufferSize];
      char linear[EEPROMVariableBufferSize];
      if(reportInvertedValues == 1)
      {
          a1 = 1.0/(a1);
          a2 = 1.0/(a2); 
      }
      itoa(reportInvertedValues, invertedStatus, 2);
      itoa(reportedConfiguredStatus, configuredStatus, 2);
      itoa(expressionTotalTerms, expressionTerms, 3);
      dtostrf(a1, EEPROMVariableLength, EEPROMDecimalPrecision, constant); // Leave room for too large numbers!
      dtostrf(a2, EEPROMVariableLength, EEPROMDecimalPrecision, linear); // Leave room for too large numbers!
      WriteCalEEPROM(reportConfigured, offsetInEEPROM, configuredStatus, "sensor1", "linear", expressionTerms, invertedStatus, constant, linear, "0", "0", "0", "0", "0", "0", "0", "0", EEPROMCurrentPosition);  // values to write into EEPROM, variables unused up to 10 are reported as "0" 
   }
   
   delay(3000);
   while(1){}; //keeps stuck in infinite loop once calculation ends
}

void fabls_quad(unsigned int n,double *px,double *py)
{  
   pyregress = new double[totalPoints];
   byte mask='\x00',sign,sign2;
   unsigned int i;
   int least=-1;
   double beta,d2,denom,dy,p,percent_error,r=(n-1),x,y,yc;
   double a1,a2,a3,s,s1,s2,s3,s4,s5,s6,s7,z[5];
   byte *f = "%f %f %f %f %f\n";
   
   s1 = s2 = s3 = s4 = s5 = s6 = s7 = s = 0;
   for (i=0; i<n; i++)
   {  x = px[i];
      y = py[i];
      s1 += x;
      s2 += x * x;
      s3 += x * x * x;
      s4 += x * x * x * x;
      s5 += y;
      s6 += x * y;
      s7 += x * x * y;
   }
   if (denom = n  * (s2 * s4 - s3 * s3) -
               s1 * (s1 * s4 - s2 * s3) +
               s2 * (s1 * s3 - s2 * s2) )
   {  a1 = (s5 * (s2 * s4 - s3 * s3) -
            s6 * (s1 * s4 - s2 * s3) +
            s7 * (s1 * s3 - s2 * s2)) / denom;
      a2 = (n  * (s6 * s4 - s3 * s7) -
            s1 * (s5 * s4 - s7 * s2) +
            s2 * (s5 * s3 - s6 * s2)) / denom;
      a3 = (n  * (s2 * s7 - s6 * s3) -
            s1 * (s1 * s7 - s5 * s3) +
            s2 * (s1 * s6 - s5 * s2)) / denom;
      for (i=0; i<n; i++)
      {  x = px[i];
         dy = py[i] - (a3 * x * x + a2 * x + a1);
         s += dy * dy;
      }
      s = sqrt(s / r);
      sign  = (a1 < 0) ? '-' : '+';
      sign2 = (a2 < 0) ? '-' : '+';
      ardprintf("Quadratic:   y = (%f) x^2 %c (%f) x %c %f; s = %f\n",
             a3,sign2,fabs(a2),sign,fabs(a1),s);
      mask |= '\x02';
      z[1] = s;
   }

   Serial.print("X");
   Serial.print("         Y");
   Serial.print("         Calculated Y");
   Serial.println("     PercentError%");
   for (unsigned int i = 0; i < n; ++i)
   {
      double y = ((a3) * (px[i] * px[i])) + ((a2) * px[i]) + (a1); 

      pyregress[i] = y;
      
      double error = ((y - py[i])/py[i])*100;
      ardprintf("%f      %f      %f             %f", px[i], py[i], y, error);
   }
   double rSquaredReturn;
   double adjRSquaredReturn;
   determinationCoefficient(n, py, pyregress, 1, rSquaredReturn, adjRSquaredReturn); // calcuate and print correlation coefficient
   Serial.print(F("r^2 = "));
   Serial.println(rSquaredReturn);
   Serial.print(F("adjusted r^2 = "));
   Serial.println(adjRSquaredReturn); 

   if(reportToEEPROM == 1)
   {
      int expressionTotalTerms = 3;
      bool reportedConfiguredStatus = 1;  // updated status for configure
      char invertedStatus[2];
      char configuredStatus[2];
      char expressionTerms[2];
      char constant[EEPROMVariableBufferSize];
      char linear[EEPROMVariableBufferSize];
      char squared[EEPROMVariableBufferSize];
      if(reportInvertedValues == 1)
      {
          a1 = 1/(a1);
          a2 = 1/(a2); 
          a3 = 1/(a3);
      }
      itoa(reportInvertedValues, invertedStatus, 2);
      itoa(reportedConfiguredStatus, configuredStatus, 2);
      itoa(expressionTotalTerms, expressionTerms, 2);
      dtostrf(a1, EEPROMVariableLength, EEPROMDecimalPrecision, constant); // Leave room for too large numbers!
      dtostrf(a2, EEPROMVariableLength, EEPROMDecimalPrecision, linear); // Leave room for too large numbers!
      dtostrf(a3, EEPROMVariableLength, EEPROMDecimalPrecision, squared); // Leave room for too large numbers!
      WriteCalEEPROM(1, 0, configuredStatus, "sensor1", "quadratic", expressionTerms, invertedStatus, constant, linear, squared, "0", "0", "0", "0", "0", "0", "0", EEPROMCurrentPosition);
   }
}


void fabls_exp(unsigned int n,double *px,double *py)
{  
   pyregress = new double[totalPoints];
   byte mask='\x00',sign,sign2;
   unsigned int i;
   int least=-1;
   double beta,d2,denom,dy,p,percent_error,r=(n-1),x,y,yc;
   double a1,a2,a3,s,s1,s2,s3,s4,s5,s6,s7,z[5];
   byte *f = "%f %f %f %f %f\n";
   
   s1 = s2 = s3 = s4 = s = 0;
   for (i=0; i<n; i++)
   {  x = px[i];
      y = alog(py[i]);
      s1 += x;
      s2 += x * x;
      s3 += y;
      s4 += x * y;
   }
   if (denom = n * s2 - s1 * s1)
   {  a1 = (s3 * s2 - s1 * s4) / denom;
      a2 = (n  * s4 - s3 * s1) / denom;
      for (i=0; i<n; i++)
      {  dy = alog(py[i]) - (a2 * px[i] + a1);
         s += dy * dy;
      }
      s = sqrt(s / r);
      sign = (a1 < 0) ? '-' : '+';
      ardprintf("Exponential: y = exp(%f x %c %f); s = %f\n",a2,sign,fabs(a1),s);
      mask |= '\x04';
      z[2] = s;
   }

   Serial.print("X");
   Serial.print("         Y");
   Serial.print("         Calculated Y");
   Serial.println("       Absolute Error");
   Serial.println("     PercentError%");
   for (unsigned int i = 0; i < n; ++i)
   {
      double y = (a2) * px[i] + (a1);
      double absoluteError = y - py[i]; 

      pyregress[i] = y;
      
      double error = ((y - py[i])/py[i])*100;
      ardprintf("%f      %f      %f             %f          %f", px[i], py[i], y, absoluteError, error);
   }
   double rSquaredReturn;
   double adjRSquaredReturn;
   determinationCoefficient(n, py, pyregress, 1, rSquaredReturn, adjRSquaredReturn); // calcuate and print correlation coefficient
   Serial.print(F("r^2 = "));
   Serial.println(rSquaredReturn);
   Serial.print(F("adjusted r^2 = "));
   Serial.println(adjRSquaredReturn); // this is not true, just a place-holder for now
}



void fabls_log(unsigned int n,double *px,double *py)
{  
   pyregress = new double[totalPoints];
   byte mask='\x00',sign,sign2;
   unsigned int i;
   int least=-1;
   double beta,d2,denom,dy,p,percent_error,r=(n-1),x,y,yc;
   double a1,a2,a3,s,s1,s2,s3,s4,s5,s6,s7,z[5];
   byte *f = "%f %f %f %f %f\n";
   
   s1 = s2 = s3 = s4 = s = 0;
   for (i=0; i<n; i++)
   {  x = alog(px[i]);
      y = py[i];
      s1 += x;
      s2 += x * x;
      s3 += y;
      s4 += x * y;
   }
   if (denom = n  * s2 - s1 * s1)
   {  a1 = (s3 * s2 - s1 * s4) / denom;
      a2 = (n  * s4 - s3 * s1) / denom;
      for (i=0; i<n; i++)
      {  x = alog(px[i]);
         dy = py[i] - (x * a2 + a1);
         s += dy * dy;
      }
      s = sqrt(s / r);
      sign = (a1 < 0) ? '-' : '+';
      ardprintf("Logarithmic:   y = (%f) ln(x) %c %f; s = %f\n",a2,sign,fabs(a1),s);
      mask |= '\x08';
      z[3] = s;
   }

   Serial.print("X");
   Serial.print("         Y");
   Serial.print("         Calculated Y");
   Serial.println("     PercentError%");
   for (unsigned int i = 0; i < n; ++i)
   {
      double y = (a2) * px[i] + (a1);
      // PercentError%=((regressionvalue-calibrationvalue)/calibrationvalue)*100 

      pyregress[i] = y;
      
      double error = ((y - py[i])/py[i])*100;
      ardprintf("%f      %f      %f             %f", px[i], py[i], y, error);
   }
   double rSquaredReturn;
   double adjRSquaredReturn;
   determinationCoefficient(n, py, pyregress, 1, rSquaredReturn, adjRSquaredReturn); // calcuate and print correlation coefficient
   Serial.print(F("r^2 = "));
   Serial.println(rSquaredReturn);
   Serial.print(F("adjusted r^2 = "));
   Serial.println(adjRSquaredReturn); // this is not true, just a place-holder for now
}



void fabls_power(unsigned int n,double *px,double *py)
{  
   pyregress = new double[totalPoints];
   byte mask='\x00',sign,sign2;
   unsigned int i;
   int least=-1;
   double beta,d2,denom,dy,p,percent_error,r=(n-1),x,y,yc;
   double a1,a2,a3,s,s1,s2,s3,s4,s5,s6,s7,z[5];
   byte *f = "%f %f %f %f %f\n";
  
   s1 = s2 = s3 = s4 = s = 0;
   for (i=0; i<n; i++)
   {  x = alog(px[i]);
      y = alog(py[i]);
      s1 += x;
      s2 += x * x;
      s3 += y;
      s4 += x * y;
   }
   if (denom = n  * s2 - s1 * s1)
   {  a1 = exp((s3 * s2 - s1 * s4) / denom);
      a2 = (n  * s4 - s3 * s1) / denom;
      for (i=0; i<n; i++)
      {  dy = py[i] - a1 * pow(px[i],a2);
         s += dy * dy;
      }
      s = sqrt(s / r);
      sign = (a1 < 0) ? '-' : '+';
      ardprintf("Power:   y = (%f) x ^ (%f); s = %f\n",a1,a2,s);
      mask |= '\x10';
      z[4] = s;
   }

   Serial.print("X");
   Serial.print("         Y");
   Serial.print("         Calculated Y");
   Serial.println("     PercentError%");
   for (unsigned int i = 0; i < n; ++i)
   {
      double y = (a2) * px[i] + (a1);
      // PercentError%=((regressionvalue-calibrationvalue)/calibrationvalue)*100 

      pyregress[i] = y;
      
      double error = ((y - py[i])/py[i])*100;
      ardprintf("%f      %f      %f             %f", px[i], py[i], y, error);
   }
   double rSquaredReturn;
   double adjRSquaredReturn;
   determinationCoefficient(n, py, pyregress, 1, rSquaredReturn, adjRSquaredReturn); // calcuate and print correlation coefficient
   Serial.print(F("r^2 = "));
   Serial.println(rSquaredReturn);
   Serial.print(F("adjusted r^2 = "));
   Serial.println(adjRSquaredReturn); // this is not true, just a place-holder for now
}




void readSeveralChars() {

  // this reads all the characters in the input buffer
  // if there are too many for the inputSeveral array the extra chars will be lost

  inputSeveral[0] = 0; // makes inputSeveral an empty string with just a terminator

  byte ndx = 0;        // the index position for storing the character

  if (Serial.available() > 0) {

    while (Serial.available() > 0) { // keep going until buffer is empty
      if (ndx > maxChars - 1) { // -1 because arrays count from 0
        ndx = maxChars;     // if there are too many chars the extra ones are
      }                     //   dumped into the last array element which will
      //   be overwritten by the string terminator
      inputSeveral[ndx] = Serial.read();
      ndx ++;

    }

    if (ndx > maxChars) {  // to make sure the terminator is not written beyond the array
      ndx = maxChars;
    }
    inputSeveral[ndx] = 0; // add a zero terminator to mark the end of the string
  }



}



// https://gist.github.com/asheeshr/9004783
// A printf function for serial communication from Arduino boards
int ardprintf(char *str, ...)
{
  int i, count=0, j=0, flag=0;
  char temp[ARDBUFFER+1];
  for(i=0; str[i]!='\0';i++)  if(str[i]=='%')  count++;

  va_list argv;
  va_start(argv, count);
  for(i=0,j=0; str[i]!='\0';i++)
  {
    if(str[i]=='%')
    {
      temp[j] = '\0';
      Serial.print(temp);
      j=0;
      temp[0] = '\0';

      switch(str[++i])
      {
        case 'd': Serial.print(va_arg(argv, int));
                  break;
        case 'l': Serial.print(va_arg(argv, long));
                  break;
        case 'f': Serial.print(va_arg(argv, double));
                  break;
        case 'c': Serial.print((char)va_arg(argv, int));
                  break;
        case 's': Serial.print(va_arg(argv, char *));
                  break;
        default:  ;
      };
    }
    else 
    {
      temp[j] = str[i];
      j = (j+1)%ARDBUFFER;
      if(j==0) 
      {
        temp[ARDBUFFER] = '\0';
        Serial.print(temp);
        temp[0]='\0';
      }
    }
  };
  Serial.println();
  return count + 1;
}

int ardprintf(int floatPrecision, char *str, ...)
{
  int i, count=0, j=0, flag=0;
  char temp[ARDBUFFER+1];
  for(i=0; str[i]!='\0';i++)  if(str[i]=='%')  count++;

  va_list argv;
  va_start(argv, count);
  for(i=0,j=0; str[i]!='\0';i++)
  {
    if(str[i]=='%')
    {
      temp[j] = '\0';
      Serial.print(temp);
      j=0;
      temp[0] = '\0';

      switch(str[++i])
      {
        case 'd': Serial.print(va_arg(argv, int));
                  break;
        case 'l': Serial.print(va_arg(argv, long));
                  break;
        case 'f': Serial.print(va_arg(argv, double),floatPrecision);
                  break;
        case 'c': Serial.print((char)va_arg(argv, int));
                  break;
        case 's': Serial.print(va_arg(argv, char *));
                  break;
        default:  ;
      };
    }
    else 
    {
      temp[j] = str[i];
      j = (j+1)%ARDBUFFER;
      if(j==0) 
      {
        temp[ARDBUFFER] = '\0';
        Serial.print(temp);
        temp[0]='\0';
      }
    }
  };
  Serial.println();
  return count + 1;
}


// Extra goodness of fit information
void determinationCoefficient(const int n, double *y, double *yRegression, const int regressors, double &rSquared, double &adjustedRSquared)
{
    rSquared = 0.0; //initializing reference passed return
    adjustedRSquared = 0.0; //same as above but for adjusted R squared
    double averageY = 0.0;
    double squareDiffSumY = 0.0;
    double regressDifferenceY = 0.0;
    double regressDiffSumY = 0.0;
   
    for (int i = 0; i < n; i++)
    {
       averageY += y[i];
    }
 
    averageY /= n;
 
    for (int i = 0; i < n; i++)
    {
        squareDiffSumY += ((y[i] - averageY) * (y[i] - averageY));
    }
 
    for (int i = 0; i < n; i++)
    {
        regressDiffSumY += ((yRegression[i] - averageY) * (yRegression[i] - averageY));
    }
   
    rSquared = (regressDiffSumY / squareDiffSumY);
    adjustedRSquared = 1.0-(((1.0-(rSquared))*(((double)n-1.0))/((double)n-((double)regressors+1.0))));  
}

double safeDiv(double numerator, double denominator)
{
    if (denominator == 0)
    {
        return 0.0;

    }

    else
    {
        return (numerator/denominator);

    }
}


void WriteCalEEPROM(bool update_configured_status, int eepromoffset, char* towrite_configured, char* towrite_value_name, char* towrite_type_of_regression, char* expression_terms, char* towrite_inverted, char* towrite_cal_term1, char* towrite_cal_term2, char* towrite_cal_term3, char* towrite_cal_term4, char* towrite_cal_term5, char* towrite_cal_term6, char* towrite_cal_term7, char* towrite_cal_term8, char* towrite_cal_term9, char* towrite_cal_term10, int &EEPROMReadLocation){
  //Build EEPROM value string from inputs to the function, make the changes before calling the function to update the RAM copies of: configured, cal_1_a, cal_1_m, cal_1_b, cal_2_a, cal_2_m, cal_2_b, cal_3_a,  cal_3_m, cal_3_b, cal_batt_m, cal_batt_b
  //EEPROMReset();  //perform EEPROM clear before updating new values (disabled right now in use)
  char datatowrite[150] = {};  //EEPROM entry character array holder
  Serial.println("Preparing to save to EEPROM");
  char* sep = "#";
  int number_of_expressions = atoi(expression_terms); 
  if(update_configured_status == 1)
  {
      strcat(datatowrite, towrite_configured);
      strcat(datatowrite, sep);
  }
  strcat(datatowrite, towrite_value_name);
  strcat(datatowrite, sep);
  strcat(datatowrite, towrite_type_of_regression);
  strcat(datatowrite, sep);
  strcat(datatowrite, expression_terms);
  strcat(datatowrite, sep);
  strcat(datatowrite, towrite_inverted);
  strcat(datatowrite, sep);
  if(number_of_expressions > 0)
  {
      strcat(datatowrite, towrite_cal_term1);
      strcat(datatowrite, sep);
  }
  if(number_of_expressions > 1)
  {
      strcat(datatowrite, towrite_cal_term2);
      strcat(datatowrite, sep);
  }
  if(number_of_expressions > 2)
  {
      strcat(datatowrite, towrite_cal_term3);
      strcat(datatowrite, sep);
  }
  if(number_of_expressions > 3)
  {
      strcat(datatowrite, towrite_cal_term4);
      strcat(datatowrite, sep);
  }
  if(number_of_expressions > 4)
  {
      strcat(datatowrite, towrite_cal_term5);
      strcat(datatowrite, sep);
  }
  if(number_of_expressions > 5)
  {
      strcat(datatowrite, towrite_cal_term6);
      strcat(datatowrite, sep);
  }
  if(number_of_expressions > 6)
  {
      strcat(datatowrite, towrite_cal_term7);
      strcat(datatowrite, sep);
  }
  if(number_of_expressions > 7)
  {
      strcat(datatowrite, towrite_cal_term8);
      strcat(datatowrite, sep);
  }
  if(number_of_expressions > 8)
  {
      strcat(datatowrite, towrite_cal_term9);
      strcat(datatowrite, sep);
  }
  if(number_of_expressions > 9)
  {
      strcat(datatowrite, towrite_cal_term10);
      strcat(datatowrite, sep);
  }
  Serial.println("Current values ready to be updated to EEPROM: ");
  Serial.println(datatowrite);
  Serial.println();
  Serial.println("Saving to EEPROM");
  EEPROMReadLocation = save_data(eepromoffset, datatowrite);  //load the final values into EERPOM
  delay (10);
  Serial.println("EEPROM Update completed...");
} //end of WriteCalEEPROM function

int save_data(int offset, char* datalocal){
  //Mechanism called by another function to write pre-packaged data to the EEPROM
  Serial.println("Call to write data to EEPROM");
  EEPROM.begin(); //this takes 0 arguments for AVR, but a value of 512 for ESP32
  int index = 0;
  for (int i = offset; i < strlen(datalocal); ++i){
    EEPROM.write(i, (int)datalocal[i]);
    delay(1);
    index = i;
  }
  //EEPROM.commit(); //Not required for AVR, only ESP32
  Serial.println("Write data complete");
  return index;  // last EEPROM address written
  delay(10);
} //end of save_data function define
