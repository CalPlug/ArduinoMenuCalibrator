// Arduino Based Multi-form Least-Squares Regression Calibration Utility
//Development by Avinash Pai and Bryan Karimi with component contributions cited.
//Note: This program uses cited numeric calculation methods developed in part or in whole by other contributers.  The developers of this utility thank these authors for helping us develop this utility!
//Released under GNU Licence, Copyright (2019) Regents of the University of California 


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
//#define EEPROMOUT //when defined, debug is enabled to read out header info when function is called

//#define DEFAULTTHEEEPROM  //This is normally commented out to turn off defaulter during normal operation!  When turned on the EEPROM is defaulted at the beginning of the Setup()   Note: turn this on and run once if new chip to put in the field separators in place in the EEPROm to overwite later - consider this a partition step

//EEPROM Configuration Settings
//Allows the output to be written to the EEPROM in a common format that can be extended on in multiple writes.  An initial flag is used as a quick test for readback to verify if valud values have been already pusghed to the EEPROM.  on the second value set, this write is supressed with the EEPROM offset provided so the values in the EEPROM for all sensors are sequenctial.
#define reportToEEPROM 1  // enable this, make value equal to 1 to allow recording values to the EEPROM after regression
#define reportInvertedValues 0  //  store values in EEPROM as the inverse, often used for very small decimals
#define EEPROMVariableBufferSize 25 
#define EEPROMVariableLength 12
#define EEPROMDecimalPrecision 8  

//global values for the EEPROM (mirrored cached version, with these being the initialization values)
char configured_status_cached[2];
char totalentriesread_cached[5]; 
char eepromoffsetread_cached[5];

int EEPROMCurrentPosition = 0;
int EEPROMFirstaddress = 0; //define the offset for the first entered EEPROM value
int maxheaderreadaddress = 10; //numper of positions after which to stop reading the header, this keeps it from reading the whole thing if the EEPROM is not initalized and swamping the console
int offsetInEEPROM = 10;  // offset address to begin writing in EEPROM for sensor data after the header
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
  #ifdef DEFAULTTHEEEPROM
  WriteDefaultEERPOM(); //run this vor a vigin chip
  #endif 
  Serial.begin(115200);
  Serial.println(F("Starting..."));

  // Menu select for function to fit against
  Serial.println(F("Select fit: "));
  Serial.println(F("  (1)Linear - Minimum two points"));
  Serial.println(F("  (2)Quadratic - Minimum three points")); //Specific commonly used sub-form of the general polynomial case.
  Serial.println(F("  (3)Exponential - Minimum three points, y != 0"));  // Double check restrictions on exp, log, power
  Serial.println(F("  (4)Logarithmic - Minimum three points, x != 0"));
  Serial.println(F("  (5)Power - Minimum three points, x != 0"));
  Serial.println(F("  (6)General-Form Polynomial - Minimum points = equation order")); //a 2nd power requires 2 points, 3rd power requires 3, etc.
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
    //int arraySizex = sizeof(xp) / sizeof(px[0]); //EXAMPLE SIZE DETERMINATION
    //for (int x=0; x < arraySize; x++)
       //px[x] = 0;  //zero the array
     
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


// https://forum.arduino.cc/index.php?topic=96292.0, general fitting examples, fortran and C 
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
   
   double averagepercenterror;
   double averagepercenterrorHolder[n];
   double averageabsoluteerror;
   double averageabsoluteerrorHolder[n];
   
   for (unsigned int i = 0; i < n; ++i)
   {
      double y = (a2) * px[i] + (a1);
      double absoluteError = y - py[i]; 

      pyregress[i] = y;
      
      double error = safeDiv((y - py[i]), py[i])*100.0;
      ardprintf(reportingPrecision, "%f      %f      %f                %f            %f", px[i], py[i], y, absoluteError, error);
      averagepercenterrorHolder [i] = error; //save into array during calculation
      averageabsoluteerrorHolder[i] = absoluteError;  //save into array during calculation
      
   }

  averageabsoluteerror = averagecalc(n, averageabsoluteerrorHolder); //calculate absolute error average
  averagepercenterror = averagecalc(n, averagepercenterrorHolder); //calculate absolute error average
  
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
      int currentoffset = ReadCalEEPROMHeader(configured_status_cached, totalentriesread_cached, eepromoffsetread_cached);
      //need to convert read offset to an int to feed into the next position to write the next EEPROM entry
      WriteCalEEPROM((offsetInEEPROM + currentoffset + 1),"sensor1","linear", expressionTerms, invertedStatus, constant, linear, "0", "0", "0", "0", "0", "0", "0", "0", EEPROMCurrentPosition);  // values to write into EEPROM, variables unused up to 10 are reported as "0"
      WriteCalEEPROMHeader(EEPROMCurrentPosition, "1", 1); //update the header after the last write, write in last EEPROM address location
   }
   
   delay(1000); //end function after delay, make sure buffer is cleared
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
      WriteCalEEPROM(offsetInEEPROM, "sensor1", "quadratic", expressionTerms, invertedStatus, constant, linear, squared, "0", "0", "0", "0", "0", "0", "0", EEPROMCurrentPosition);
   }
}

void fabls_polynomial(int N, int n, double *px,double *py) // Arguments: (Total number of points, order of regression, x-pints, y-pounts)
{ 
//Based on polynomial regression examples:  https://www.bragitoff.com/2015/09/c-program-for-polynomial-fit-least-squares/ (Manas Sharma, 2015) with elements of https://rosettacode.org/wiki/Polynomial_regression#C/ GSL Library 
  
    int i,j,k;
    double X[2*n+1];                        //Array that will store the values of sigma(xi),sigma(xi^2),sigma(xi^3)....sigma(xi^2n)
    for (i=0;i<2*n+1;i++)
    {
        X[i]=0;
        for (j=0;j<N;j++)
            X[i]=X[i]+pow(px[j],i);        //consecutive positions of the array will store N,sigma(xi),sigma(xi^2),sigma(xi^3)....sigma(xi^2n)
    }
    double B[n+1][n+2],a[n+1];            //B is the Normal matrix(augmented) that will store the equations, 'a' is for value of the final coefficients
    for (i=0;i<=n;i++)
        for (j=0;j<=n;j++)
            B[i][j]=X[i+j];            //Build the Normal matrix by storing the corresponding coefficients at the right positions except the last column of the matrix
    double Y[n+1];                    //Array to store the values of sigma(yi),sigma(xi*yi),sigma(xi^2*yi)...sigma(xi^n*yi)
    for (i=0;i<n+1;i++)
    {    
        Y[i]=0;
        for (j=0;j<N;j++)
        Y[i]=Y[i]+pow(px[j],i)*py[j];        //consecutive positions will store sigma(yi),sigma(xi*yi),sigma(xi^2*yi)...sigma(xi^n*yi)
    }
    for (i=0;i<=n;i++)
        B[i][n+1]=Y[i];                //load the values of Y as the last column of B(Normal Matrix but augmented)
    n=n+1;                //n is made n+1 because the Gaussian Elimination part below was for n equations, but here n is the degree of polynomial and for n degree we get n+1 equations
    Serial.println(F("\nThe Normal(Augmented Matrix) is as follows:\n"));    
    for (i=0;i<n;i++)            //print the Normal-augmented matrix
    {
        for (j=0;j<=n;j++)
        {
            Serial.print(B[i][j]);
            Serial.print("  ");
        }
            Serial.println();
    }    
    for (i=0;i<n;i++)                    //From now Gaussian Elimination starts(can be ignored) to solve the set of linear equations (Pivotisation)
        for (k=i+1;k<n;k++)
            if (B[i][i]<B[k][i])
                for (j=0;j<=n;j++)
                {
                    double temp=B[i][j];
                    B[i][j]=B[k][j];
                    B[k][j]=temp;
                }
    
    for (i=0;i<n-1;i++)            //loop to perform the gauss elimination
        for (k=i+1;k<n;k++)
            {
                double t=B[k][i]/B[i][i];
                for (j=0;j<=n;j++)
                    B[k][j]=B[k][j]-t*B[i][j];    //make the elements below the pivot elements equal to zero or elimnate the variables
            }
    for (i=n-1;i>=0;i--)                //back-substitution
    {                        //x is an array whose values correspond to the values of x,y,z..
        a[i]=B[i][n];                //make the variable to be calculated equal to the rhs of the last equation
        for (j=0;j<n;j++)
            if (j!=i)            //then subtract all the lhs values except the coefficient of the variable whose value is being calculated
                a[i]=a[i]-B[i][j]*a[j];
        a[i]=a[i]/B[i][i];            //now finally divide the rhs by the coefficient of the variable to be calculated
    }
    
    Serial.println(F("\nThe values of the coefficients are as follows:\n"));
    for (i=0;i<n;i++)
    {
        // Print the values of x^0,x^1,x^2,x^3,....    
        Serial.print("x^"); 
        Serial.print(i, reportingPrecision); //display with 4 decimal places
        Serial.print("=");
        Serial.println(a[i]); //end of line statement
    }
    Serial.println(); //Break between segments

    Serial.print(F("Hence the fitted Polynomial is given by:\n y ="));
    for (i=0;i<n;i++)
    {
      if (i == 0) //supress the initial + sign in the display
         {
          Serial.print(" ("); 
         }
      else
         {
          Serial.print(" + ("); 
         }
      Serial.print(a[i],reportingPrecision); //display with 4 decimal places
      Serial.print(")");
      Serial.print("x^");
      Serial.print(i);
    }
    Serial.println(); //Break between segments

   Serial.print(F("X"));
   Serial.print(F("         Y"));
   Serial.print(F("         Calculated Y"));
   Serial.println(F("     PercentError%"));
   for (unsigned int i = 0; i < N; ++i) //for all points
   {
    double y = 0; //set each term's calculation initialized at 0
    for (int q=0; q<n; q++)  //for all terms
      { 
        y = y + a[q]*pow(px[i],q); 
      }

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
      int expressionTotalTerms = n;
      double regressionterms[expressionTotalTerms]; //holder for inverted calculated values
      bool reportedConfiguredStatus = 1;  // updated status for configure
      char invertedStatus[2];
      char configuredStatus[2];
      char expressionTerms[2];
      char* term[expressionTotalTerms];
      if(reportInvertedValues == 1)
      {
          for (int q=0; q<expressionTotalTerms; q++)
          {
          regressionterms[q] = (1/(a[i]));
          }
      }
      itoa(reportInvertedValues, invertedStatus, 2);
      itoa(reportedConfiguredStatus, configuredStatus, 2);
      itoa(expressionTotalTerms, expressionTerms, 2);
      
      for (int q=0; q<10; q++) //max terms of 10 can be accomodated in the EEPROM
      {
        if (q<expressionTotalTerms) //if the expression is less than total terms
        {
          dtostrf(a[q], EEPROMVariableLength, EEPROMDecimalPrecision, term[q]); // fill in available terms
        }
        else
        {
          dtostrf(0, EEPROMVariableLength, EEPROMDecimalPrecision, term[q]); //fill in zeros for remaining terms up to the max terms of 10 total in the EEPROM format
        }
      }
      //NOTE:  need to set up a case structure to accomodate to 10th order
      WriteCalEEPROM(offsetInEEPROM, "sensor1", "polynomial", expressionTerms, invertedStatus, term[1], term[2], term[3], term[4], term[5], term[6], term[7], term[8], term[9], term[10], EEPROMCurrentPosition);
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

   Serial.print(F("X"));
   Serial.print(F("         Y"));
   Serial.print(F("         Calculated Y"));
   Serial.println(F("       Absolute Error"));
   Serial.println(F("     PercentError%"));
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

// A clean printf function for serial communication from Arduino boards, example developed from:  https://gist.github.com/asheeshr/9004783
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

double averagecalc(int len, double *values)
{
   int index,total;
   total = 0; //Initialize the total accumulator
   
   for(index = 0; index < len; index++) 
   {
      total += values[index];
   }
   return (total/(float)len);

}


void WriteCalEEPROMHeader(int eepromoffset, char* towrite_configured, int entries){  //function in development to take care of writing the first part of the EEPROM.
  Serial.println("Preparing to write to EEPROM Header");
  char datatowrite[150] = {};  //EEPROM entry character array holder
  char totalentries[4];
  char totalOffset[5];
  itoa(entries, totalentries, 3);  //convert the int into an entry in the char* array for the total entries in the eeprom
  itoa(eepromoffset, totalOffset, 4);  //convert the int into an entry in the char* array for the total entries in the eeprom

  char* sep = "#";
  //prepare EEPROM header
  strcat(datatowrite, towrite_configured);
  strcat(datatowrite, sep);
  strcat(datatowrite, totalentries);
  strcat(datatowrite, sep);
  strcat(datatowrite, eepromoffset);
  strcat(datatowrite, sep);

  Serial.println(F("Current Header values ready to be updated to EEPROM: "));
  Serial.println(datatowrite);

  Serial.println(F("Saving Header info to EEPROM"));
  int EEPROMReadLocation = save_data(EEPROMFirstaddress, datatowrite);  //load the final values into EERPOM, use the program defined value as the inital offset from EEPROM start
  Serial.println(F("Length of Header: "));
  Serial.print(EEPROMReadLocation);
  Serial.println();
  delay (10);
  Serial.println(F("EEPROM Header Update completed..."));
}

int ReadCalEEPROMHeader(char* configured_status, char* totalentriesread, char* eepromoffsetread){
  char data[100]={};
  //Read in header values from EEPROM.
  Serial.println(F("Call to read Header data from EEPROM"));
  EEPROM.begin(); //NOTE: this takes 0 arguments for AVR, but a value of 512 for ESP32
  int count = 0;
  int address = EEPROMFirstaddress; //start at lowest address
  int returnedeepromvalue=0; 
  int returnedentries = 0;
  while (count < 3 && (address<EEPROMFirstaddress+maxheaderreadaddress)){ //total field number to read in (total number of fields), must match total cases checked
    char read_char = (char)EEPROM.read(address); 
      #ifdef EEPROMOUT
      Serial.print(F("Last EEPROM Read is: ")); //printout headers
      Serial.print(read_char); //printout data
      #endif
    delay(1);
    if (read_char == '#'){ //use # as section break between fields
      Serial.println(data);
      switch (count){
        case 0: strcpy(configured_status, data); break;
        case 1: strcpy(totalentriesread, data); break;
        case 2: strcpy(eepromoffsetread, data); break;    
      }
      count++;
      strcpy(data,"");
    } 
    else{
      strncat(data, &read_char, 1);  
      #ifdef EEPROMOUT
      Serial.print(F("Interpreted character (between field # marker): ")); //printout headers
      Serial.print(data); //printout data
      Serial.println(""); //printout separator, line break
      #endif
    }
    ++address;
  }
  delay(100);
  Serial.println(F("<--Read data complete, this was read from Header stored in EEPROM (Print EEPROM bytes as characters)"));
    itoa(totalentriesread, returnedeepromvalue, 4);
    itoa(eepromoffsetread, returnedentries, 4);
    return eepromoffsetread; //return the EEPROM of the last entry to use to place the next entry
}//end of ReadCalEEPROM() function define 

int  WriteDefaultEERPOM()  //call this to write the default values to "partition" the EEPROM, returns last EEPROM address
{
bool update_configured_status = 1; //indicate to update this status with what is placed below
int eepromoffset = EEPROMFirstaddress; //set as start address
char* towrite_configured = "0"; // update status flag (data good flag) as not configured
char* totalentriestoeeprom = "0";
char* totaloffsetinend = "0";
char* towrite_value_name = "demo";
char* towrite_type_of_regression = "demo";
char* expression_terms = "0.0";
char* towrite_inverted = "0.0";
char* towrite_cal_term1 = "0.0";
char* towrite_cal_term2 = "0.0";
char* towrite_cal_term3 = "0.0";
char* towrite_cal_term4 = "0.0";
char* towrite_cal_term5 = "0.0";
char* towrite_cal_term6 = "0.0";
char* towrite_cal_term7 = "0.0";
char* towrite_cal_term8 = "0.0";
char* towrite_cal_term9 = "0.0";
char* towrite_cal_term10 = "0.0"; 
int EEPROMReadLocation =0; //updated value for final EEPROM location

  char datatowrite[120] = {};  //EEPROM entry character array holder
  Serial.println(F("Preparing to save to EEPROM"));
  char* sep = "#";
  if(update_configured_status == 1)
  {
      strcat(datatowrite, towrite_configured);
      strcat(datatowrite, sep);
  }

  strcat(datatowrite, totalentriestoeeprom);
  strcat(datatowrite, sep);
  strcat(datatowrite, totaloffsetinend);
  strcat(datatowrite, sep);

  strcat(datatowrite, towrite_value_name);
  strcat(datatowrite, sep);
  strcat(datatowrite, towrite_type_of_regression);
  strcat(datatowrite, sep);
  strcat(datatowrite, expression_terms);
  strcat(datatowrite, sep);
  strcat(datatowrite, towrite_inverted);
  strcat(datatowrite, sep);

  int number_of_expressions = atoi(expression_terms); 
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
  Serial.println(F("Current Default values ready to be updated to EEPROM: "));
  Serial.println(datatowrite);
  Serial.println();
  Serial.println(F("Saving Defaults to EEPROM"));
  EEPROMReadLocation = save_data(eepromoffset, datatowrite);  //load the final values into EERPOM
  delay (10);
  Serial.println(F("EEPROM Update completed..."));
  return EEPROMReadLocation; //return last EEPROM location
} //end of WriteCalEEPROM function
  
void WriteCalEEPROM(int eepromoffset, char* towrite_entryvalue_name, char* towrite_type_of_regression, char* expression_terms, char* towrite_inverted, char* towrite_cal_term1, char* towrite_cal_term2, char* towrite_cal_term3, char* towrite_cal_term4, char* towrite_cal_term5, char* towrite_cal_term6, char* towrite_cal_term7, char* towrite_cal_term8, char* towrite_cal_term9, char* towrite_cal_term10, int &EEPROMReadLocation){
  char localdatatowrite[150] = {};  //EEPROM entry character array holder
  Serial.println(F("Preparing to save Sensor Data to EEPROM"));
  char* sep = "#";
  strcat(localdatatowrite, towrite_entryvalue_name);
  strcat(localdatatowrite, sep);
  strcat(localdatatowrite, towrite_type_of_regression);
  strcat(localdatatowrite, sep);
  strcat(localdatatowrite, expression_terms);
  strcat(localdatatowrite, sep);
  strcat(localdatatowrite, towrite_inverted);
  strcat(localdatatowrite, sep);

  int number_of_expressions = atoi(expression_terms); 
  if(number_of_expressions > 0)
  {
      strcat(localdatatowrite, towrite_cal_term1);
      strcat(localdatatowrite, sep);
  }
  if(number_of_expressions > 1)
  {
      strcat(localdatatowrite, towrite_cal_term2);
      strcat(localdatatowrite, sep);
  }
  if(number_of_expressions > 2)
  {
      strcat(localdatatowrite, towrite_cal_term3);
      strcat(localdatatowrite, sep);
  }
  if(number_of_expressions > 3)
  {
      strcat(localdatatowrite, towrite_cal_term4);
      strcat(localdatatowrite, sep);
  }
  if(number_of_expressions > 4)
  {
      strcat(localdatatowrite, towrite_cal_term5);
      strcat(localdatatowrite, sep);
  }
  if(number_of_expressions > 5)
  {
      strcat(localdatatowrite, towrite_cal_term6);
      strcat(localdatatowrite, sep);
  }
  if(number_of_expressions > 6)
  {
      strcat(localdatatowrite, towrite_cal_term7);
      strcat(localdatatowrite, sep);
  }
  if(number_of_expressions > 7)
  {
      strcat(localdatatowrite, towrite_cal_term8);
      strcat(localdatatowrite, sep);
  }
  if(number_of_expressions > 8)
  {
      strcat(localdatatowrite, towrite_cal_term9);
      strcat(localdatatowrite, sep);
  }
  if(number_of_expressions > 9)
  {
      strcat(localdatatowrite, towrite_cal_term10);
      strcat(localdatatowrite, sep);
  }
  
  Serial.println(F("Current Data values ready to be updated to EEPROM: "));
  Serial.println(localdatatowrite);
  Serial.println();
  Serial.println(F("Saving Data Values to EEPROM"));
  EEPROMReadLocation = save_data(eepromoffset, localdatatowrite);  //load the final values into EERPOM
  delay (10);
  Serial.println(F("EEPROM Update completed..."));
} //end of WriteCalEEPROM function


int save_data(int offset, char* datalocal){
  //Mechanism called by another function to write pre-packaged data to the EEPROM
  Serial.println(F("Call to write data to EEPROM"));
  EEPROM.begin(); //this takes 0 arguments for AVR, but a value of 512 for ESP32
  int index = 0;
  for (int i = offset; i < strlen(datalocal); ++i){
    EEPROM.write(i, (int)datalocal[i]);
    delay(1);
    index = i;
  }
  //EEPROM.commit(); //Not required for AVR, only ESP32
  Serial.println(F("Write data complete"));
  return index;  // last EEPROM address written
  delay(10);
} //end of save_data function define

double readSensorInputMedian(int inputpin, int readcycles, bool enabSensorReadDelay, bool enabavgSensorReadDelay, int sensorReadDelay, int avgsensorReadDelay){  //input pin for analog signal, number of median reads to average, enable delays for median reads, enable delay for average reads, delay value between each median read, delay value between each average reading
  //performs multiple median reads then performs an average then performs a calibration from an analog input
  double middle_holder = 0;
  for (int i=0; i<readcycles; i++){
      double middle = 0;
      int a, b, c; //holders for the raw ADC sensor reads
      //Read sensor 3 sequential times
      a = analogRead(inputpin);
      if (enabSensorReadDelay=!0){
          delay(sensorReadDelay);
      }
      b = analogRead(inputpin);
      if (enabSensorReadDelay!=0){
          delay(sensorReadDelay);
      }
      c = analogRead(inputpin);
      //Perform median filter for returned first sensor read
      if ((a <= b) && (a <= c)) {
          middle = (b <= c) ? b : c;
      } 
      else if ((b <= a) && (b <= c)) {
          middle = (a <= c) ? a : c;
      } 
      else {
          middle = (a <= b) ? a : b;
      }
      //take sums for each cycle run, holder for the summation operation with the averaging loops
      middle_holder = (middle_holder+middle); //take current read median and add it to the past reads, in prep to perform average
      if (enabavgSensorReadDelay!=0){
         delay (avgsensorReadDelay);
      }    
 }
 return (((middle_holder/(double)readcycles))); //return calculated average of median read values
}

double readSensorInputSimpleAverage(int inputpin, int readcycles, bool enabavgSensorReadDelay, int avgsensorReadDelay){  //input pin for analog signal, number of direct reads to average, enable delay for average reads, delay value between each average reading
  //function performs a simple average based on a number of reads from an analog input
  double holder = 0;
  for (int i=0; i<readcycles; i++){
      int a= 0; //holder for the raw ADC sensor reads
      unsigned long holder = 0; //holder for the sum calculation
      a = analogRead(inputpin);    
      holder = (holder+a); //take current read median and add it to the past reads, in prep to perform average
      if (enabavgSensorReadDelay!=0){
         delay (avgsensorReadDelay);
      }
 }
 return (((holder/(double)readcycles))); //return calculated average of median read values
}
