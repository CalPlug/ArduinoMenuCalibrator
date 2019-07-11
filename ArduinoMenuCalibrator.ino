// Arduino Based Multi-form Least-Squares Regression Calibration Utility
//Development by Avinash Pai and Bryan Karimi with component contributions cited.
//Note: This program uses cited numeric calculation methods developed in part or in whole by other contributers.  The developers of this utility thank these authors for helping us develop this utility!
//Released under GNU Licence, Copyright (2019) Regents of the University of California 

#include <math.h>
#include <EEPROM.h>                      
#include <stdarg.h>
#include <Arduino.h>

//General #defines
#define regressionPrecision 5  //decicmal precision for recorded values saved to EEPROM
#define reportingPrecision 4  //decicmal precision displayed to the screen
#define EEPROMDEBUG //enable to view statements printed during EEPROM operations, this may use a lot of memory, be careful!

//global holders for entered values
//warning, this is dynamically allocated, be careful about heap fragmentation!  See: https://arduino.stackexchange.com/questions/3774/how-can-i-declare-an-array-of-variable-size-globally and https://arduino.stackexchange.com/questions/682/is-using-malloc-and-free-a-really-bad-idea-on-arduino and 
//this is only the declaration (using C++ style), the actual meory allocation has not happened at this point
double* px;       // dynamic array for x's (DAQ system values)
double* py;       // dynamic array for y's (calibrated values)
double* pyregress;  // local calculated regression values for each regression type
volatile uint8_t totalPoints = 0;  //declare default case of 0 points, total point entered into the array

//EEPROM Configuration Settings
//Allows the output to be written to the EEPROM in a common format that can be extended on in multiple writes.  An initial flag is used as a quick test for readback to verify if valud values have been already pusghed to the EEPROM.  on the second value set, this write is supressed with the EEPROM offset provided so the values in the EEPROM for all sensors are sequenctial.
//#define EEPROMOUT //when defined, debug is enabled to read out header info when function is called
//#define DEFAULTTHEEEPROM  //This is normally commented out to turn off by default during normal operation!  When turned on the EEPROM is defaulted at the beginning of the Setup()   Note: turn this on and run once if new chip to put in the field separators in place in the EEPROM to overwite later - consider this a partition step.  Typically run this for a vigin chip
#define EEPROMAppend //If selected the EEPROm will append to last entered value, if not defined, the latest value will overwrite and the header in the EEPROM will be adjusted accordingly
#define reportInvertedValues 0  //  store values in EEPROM as the inverse, often used for very small decimals
#define EEPROMVariableBufferSize 25 
#define EEPROMVariableLength 12
#define EEPROMDecimalPrecision 8  
#define ENTRYNAMEMAX 8 //8 chars max for allowed entryname for data name flag in EEPROM
int reportToEEPROM = 1; //flag to report to EEPR

//global values for the EEPROM (mirrored cached version, with these being the initialization values)
char configured_status_cached[2] = {0}; //read-in field values
char totalentriesread_cached[5] = {0}; //read-in field values
char eepromoffsetread_cached[6] = {0};  //read-in field values
char invertedStatus[3] = {0};
char configuredStatus[3] = {0};
bool usecached = 0; //default do not use cached points

int EEPROMCurrentPosition = 0;
int EEPROMFirstaddress = 0; //define the offset for the first entered EEPROM value
int maxheaderreadaddress = 10; //numper of positions after which to stop reading the header, this keeps it from reading the whole thing if the EEPROM is not initalized and swamping the console
int offsetInEEPROM = 10;  // offset address to begin writing in EEPROM for sensor data after the header
bool reportConfigured = 1; //status for the written configured bit flag, a 1 indicates to the reading that values are valid, any other number flags invalid or not configured values currently in EERPOM, don't bother reading in the rest.
bool reportedConfiguredStatus = 1;  // updated status for configure, this is the read out value 


void setup() {
 
  Serial.begin(115200);
  //Serial.setTimeout(10000); //set Serial Timeout
  Serial.println(F("Arduino Least Squares Fit Tool v.1.0 | Note: USE Newline or CR/LN in serial terminal program"));
  
  //an IFDEF is used to activate the EEPROM defaulter if enabled
  #ifdef DEFAULTTHEEEPROM
  WriteDefaultEERPOM(); //run this for a vigin chip
  #endif 
}

// Functions for Fit Analysis By Least Squares, multiple methods for different regression types and corresponding output helper functions
void fabls_linear(unsigned int n,double *px,double *py)
{  
  //Algorithm used based on:  https://forum.arduino.cc/index.php?topic=96292.0, general fitting examples, fortran and C 
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
      mask |= '\x01';
      z[0] = s;
      textSectionBreak();
      Serial.print("Linear:   y = (");
      Serial.print(a2,regressionPrecision);
      Serial.print(")x ");
      Serial.print((char)sign);
      Serial.print(" ");
      Serial.print("(");
      Serial.print(fabs(a1),regressionPrecision);
      Serial.print(")");
      Serial.print(" ; s = ");
      Serial.println(s,regressionPrecision);
   }
   //end of regression algorithm

   regressionErrorHeader();  //Display the text header for the  fittign errors returned data (common display function used to save memory)
   double averagepercenterrorHolder[n] = {0}; //initialize holder array with all 0's
   double averageabsoluteerrorHolder[n] = {0}; //initialize holder array with all 0's
   for (unsigned int i = 0; i < n; ++i)
   {
      double y = (a2) * px[i] + (a1);
      double absoluteError = y - py[i]; 
      pyregress[i] = y;
      double error = safeDiv((y - py[i]), py[i])*100.0;
      printRegErrors (px[i], py[i], y, absoluteError, error); 
      averagepercenterrorHolder [i] = error; //save into array during calculation
      averageabsoluteerrorHolder[i] = absoluteError;  //save into array during calculation
   }
  double averageabsoluteerror = averagecalc(n, averageabsoluteerrorHolder); //calculate absolute error average
  double averagepercenterror = averagecalc(n, averagepercenterrorHolder); //calculate absolute error average
  Serial.print("Avg Abs Err: ");
  Serial.println(averageabsoluteerror, regressionPrecision);
  Serial.print(F("Avg % Err: "));
  Serial.println(averagepercenterror,regressionPrecision);
  
 double rSquaredReturn;
 determinationCoefficient(n, py, pyregress, 1, rSquaredReturn); // calcuate and print correlation coefficient

 Serial.print("r^2 = ");
 Serial.println(rSquaredReturn, reportingPrecision);
 //Serial.print(F("adjusted r^2 = "));  //adjusted r^2 used with mutivariate, not used in this regression type
 //Serial.println(adjRSquaredReturn, reportingPrecision); //adjusted r^2 used with mutivariate, not used in this regression type
 textSectionBreak();

 int append = 1;
 int inverted = 0; 
 char entryname[ENTRYNAMEMAX]={0};//entry name holder
 reportToEEPROM = saveToEEPROMPrompt(append, inverted, entryname, ENTRYNAMEMAX); //reference return append status
   if(reportToEEPROM == 1)
   {
    Serial.println(F("Recording values in EERPOM..."));
      int expressionTotalTerms = 2;
      char expressionTerms[4] = {0};
      char constant[EEPROMVariableBufferSize];
      char linear[EEPROMVariableBufferSize];
      if(reportInvertedValues == 1)
      {
          a1 = 1.0/(a1);
          a2 = 1.0/(a2); 
      }
      itoa(reportInvertedValues, invertedStatus, 10); //convert as base 10, integer to array to pass into EERPOM functions requiring CHAR inputs
      itoa(reportedConfiguredStatus, configuredStatus, 10); //convert as base 10, integer to array to pass into EERPOM functions requiring CHAR inputs
      itoa(expressionTotalTerms, expressionTerms, 10); //convert as base 10, integer to array to pass into EERPOM functions requiring CHAR inputs
      dtostrf(a1, EEPROMVariableLength, EEPROMDecimalPrecision, constant); // Leave room for too large numbers!
      dtostrf(a2, EEPROMVariableLength, EEPROMDecimalPrecision, linear); // Leave room for too large numbers!
      Serial.println();
      int currentoffset = ReadCalEEPROMHeader(configured_status_cached, totalentriesread_cached, eepromoffsetread_cached);
      EEPROMStatusMessages(1); //EEPROM readout message (redundant text pulled from function)
      Serial.println (currentoffset, DEC); //print the position as an integer
      //need to convert read offset to an int to feed into the next position to write the next EEPROM entry
      EEPROMStatusMessages(2); //EEPROM readout message (redundant text pulled from function)
      Serial.println ((offsetInEEPROM + currentoffset + 1), DEC);
      currentoffset = WriteCalEEPROM((offsetInEEPROM + currentoffset + 1),"sensor1","linear", expressionTerms, invertedStatus, constant, linear, "0", "0", "0", "0", "0", "0", "0", "0", EEPROMCurrentPosition);  // values to write into EEPROM, variables unused up to 10 are reported as "0"
      //Note: use : as the denote of a new device entry
      EEPROMStatusMessages(3); //EEPROM readout message (redundant text pulled from function)
      Serial.println (currentoffset, DEC); //print the position as an integer
      EEPROMStatusMessages(5); //EEPROM readout message (redundant text pulled from function)
      Serial.println (EEPROMCurrentPosition, DEC);
      //using field append by adding 
      #ifdef EEPROMAppend
      WriteCalEEPROMHeader(EEPROMCurrentPosition, "1", 1); //Append option: update the header after the last write, write in last EEPROM address location
      #endif
      #ifndef EEPROMAppend
      WriteCalEEPROMHeader(offsetInEEPROM, "1", 1); ////using overwite next field as first as option: update the header after the last write, write in last EEPROM address location as first
      #endif
    }
  
   delay(1000); //end function after delay, make sure buffer is cleared
}

void fabls_quad(unsigned int n,double *px,double *py)
{  
  //Algorithm used based on:  https://forum.arduino.cc/index.php?topic=96292.0, general fitting examples, fortran and C 
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
      textSectionBreak();
      mask |= '\x02';
      z[1] = s;
      Serial.print("Quadratic:   y = (");
      Serial.print(a3,regressionPrecision);
      Serial.print(")x^2 ");
      Serial.print((char)sign2);
      Serial.print(" (");
      Serial.print(fabs(a2),regressionPrecision);
      Serial.print(") x ");
      Serial.print((char)sign);
      Serial.print(" ");
      Serial.print("(");
      Serial.print(fabs(a1),regressionPrecision);
      Serial.print(")");
      Serial.print(" ; s = ");
      Serial.println(s,regressionPrecision);

   }
   //end of regression algorithm
   
   regressionErrorHeader();  //Display the text header for the  fittign errors returned data (common display function used to save memory)
   double averagepercenterrorHolder[n] = {0}; //initialize holder array with all 0's
   double averageabsoluteerrorHolder[n] = {0}; //initialize holder array with all 0's
   
   for (unsigned int i = 0; i < n; ++i)
   {
      double y = ((a3) * (px[i] * px[i])) + ((a2) * px[i]) + (a1); 
      double absoluteError = y - py[i]; 
      pyregress[i] = y;
      double error = ((y - py[i])/py[i])*100;
      printRegErrors (px[i], py[i], y, absoluteError, error);
      averagepercenterrorHolder [i] = error; //save into array during calculation
      averageabsoluteerrorHolder[i] = absoluteError;  //save into array during calculation
   }
   double averageabsoluteerror = averagecalc(n, averageabsoluteerrorHolder); //calculate absolute error average
   double averagepercenterror = averagecalc(n, averagepercenterrorHolder); //calculate absolute error average
   Serial.print(F("Avg Abs Err: "));
   Serial.println(averageabsoluteerror, regressionPrecision);
   Serial.print(F("Avg % Err: "));
   Serial.println(averagepercenterror,regressionPrecision);
   
   double rSquaredReturn;
 
   determinationCoefficient(n, py, pyregress, 1, rSquaredReturn); // calcuate and print correlation coefficient
   Serial.print(F("r^2 = "));
   Serial.println(rSquaredReturn);
   //Serial.print(F("adjusted r^2 = "));
   //Serial.println(adjRSquaredReturn); 
   textSectionBreak();
   int append = 1;
   int inverted = 0; 
   char entryname[ENTRYNAMEMAX]={0};//entry name holder
   reportToEEPROM = saveToEEPROMPrompt(append, inverted, entryname, ENTRYNAMEMAX); //reference return append status
   if(reportToEEPROM == 1)
   {
    Serial.println(F("Recording values in EERPOM..."));
      int expressionTotalTerms = 3;
      char expressionTerms[4] = {0};
      char constant[EEPROMVariableBufferSize];
      char linear[EEPROMVariableBufferSize];
      char squared[EEPROMVariableBufferSize];
      if(reportInvertedValues == 1)
      {
          a1 = 1.0/(a1);
          a2 = 1.0/(a2); 
          a3 = 1.0/(a3);
      }
      itoa(reportInvertedValues, invertedStatus, 10); //convert as base 10, integer to array to pass into EERPOM functions requiring CHAR inputs
      itoa(reportedConfiguredStatus, configuredStatus, 10); //convert as base 10, integer to array to pass into EERPOM functions requiring CHAR inputs
      itoa(expressionTotalTerms, expressionTerms, 10); //convert as base 10, integer to array to pass into EERPOM functions requiring CHAR inputs
      dtostrf(a1, EEPROMVariableLength, EEPROMDecimalPrecision, constant); // Leave room for too large numbers!
      dtostrf(a2, EEPROMVariableLength, EEPROMDecimalPrecision, linear); // Leave room for too large numbers!
      dtostrf(a3, EEPROMVariableLength, EEPROMDecimalPrecision, squared);
      Serial.println();
      int currentoffset = ReadCalEEPROMHeader(configured_status_cached, totalentriesread_cached, eepromoffsetread_cached);
      EEPROMStatusMessages(1); //EEPROM readout message (redundant text pulled from function)
      Serial.println (currentoffset, DEC); //print the position as an integer
      //need to convert read offset to an int to feed into the next position to write the next EEPROM entry
      EEPROMStatusMessages(2); //EEPROM readout message (redundant text pulled from function)
      Serial.println ((offsetInEEPROM + currentoffset + 1), DEC);
      currentoffset = WriteCalEEPROM((offsetInEEPROM + currentoffset + 1),"sensor1","quadratic", expressionTerms, invertedStatus, constant, linear, squared, "0", "0", "0", "0", "0", "0", "0", EEPROMCurrentPosition);  // values to write into EEPROM, variables unused up to 10 are reported as "0"
      //Note: use : as the denote of a new device entry
      EEPROMStatusMessages(3); //EEPROM readout message (redundant text pulled from function)
      Serial.println (currentoffset, DEC); //print the position as an integer
      EEPROMStatusMessages(5); //EEPROM readout message (redundant text pulled from function)
      Serial.println (EEPROMCurrentPosition, DEC);
      //using field append by adding 
      #ifdef EEPROMAppend
      WriteCalEEPROMHeader(EEPROMCurrentPosition, "1", 1); //Append option: update the header after the last write, write in last EEPROM address location
      #endif
      #ifndef EEPROMAppend
      WriteCalEEPROMHeader(offsetInEEPROM, "1", 1); ////using overwite next field as first as option: update the header after the last write, write in last EEPROM address location as first
      #endif
    }
  
   delay(1000); //end function after delay, make sure buffer is cleared
 
}

void fabls_polynomial(unsigned int N, unsigned int n, double *px,double *py, double *regCoeff) // Arguments: (Total number of points, order of regression, x-pints, y-pounts)
{ 
//The algorithm used in this function is based on the following polynomial regression examples:  https://www.bragitoff.com/2015/09/c-program-for-polynomial-fit-least-squares/ (Manas Sharma, 2015) with elements of https://rosettacode.org/wiki/Polynomial_regression#C/ GSL Library 
  
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
    
    //printout matrix in function, used for debug
    /*
    Serial.println(F("\nThe Normal(Augmented Matrix) is as follows:\n"));   //printout calculation matrix 
    for (i=0;i<n;i++)            //print the Normal-augmented matrix
    {
        for (j=0;j<=n;j++)
        {
            Serial.print(B[i][j]);
            Serial.print("  ");
        }
            Serial.println();
    }    
    */
    
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
    
    //printout coefs. in function, used for debug
    /*Serial.println(F("\nThe values of the coefficients are as follows:\n"));  
    for (i=0;i<n;i++)
    {
        // Print the values of x^0,x^1,x^2,x^3,....    
        Serial.print("x^"); 
        Serial.print(i, reportingPrecision); //display with 4 decimal places
        Serial.print("=");
        Serial.println(a[i]); //end of line statement
    }*/
//end of regression algorithm
  for (int i = 0; i < (n + 1); i++)  //copy array into reference returned array
  {
    regCoeff[i] = a[i];
  }
  //The matrix used in calulation for high order polynomials can be large, the scope for the calculation is set aside from the display of the returned fitting coefs. so the calculation temporary variables can fall out of scope to conserve runtime SRAM memory
}

void fabls_polyOutput(unsigned int N, unsigned int n, double *a, double *px, double *py)
{
   pyregress = new double[N];  // define holder for calculated regression values for all points
   double averagepercenterrorHolder[N] = {0}; //initialize holder with 0's
   double averageabsoluteerrorHolder[N] = {0}; //initialize holder with 0's
   Serial.print(F("The fitted (order "));
   Serial.print(n);
   Serial.print(F(") Polynomial is given by:\n y ="));
   for (int i = 0; i < (n + 1); i++) // total points is equal to n+1
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

   Serial.println();  //Break between segments

   //calculation and display of fitting errors, conceptually we are showing residual error. See this discussion for more info:  https://www.mathworks.com/help/curvefit/residual-analysis.html
   regressionErrorHeader();  //Display the text header for the  fittign errors returned data (common display function used to save memory)
   
   for (int j = 0; j < N; ++j) //for all points
   {
    double y = 0; //set each term's calculation initialized at 0
    for (int q = 0; q < (n + 1); q++)  //for all terms
      { 
        y = y + a[q]*pow((float)px[j], (float)q);  // y has ((some constant) * (x to a power)) being added to it repeatedly through the for loop 
      }
      
    double error = ((y - py[j])/py[j])*100;
    double absoluteError = y - py[j];
    Serial.print(px[j], reportingPrecision);
    Serial.print("      ");
    Serial.print(py[j], reportingPrecision);
    Serial.print("      ");
    Serial.print(y, reportingPrecision);
    Serial.print("      ");
    Serial.print(absoluteError, reportingPrecision);
    Serial.print("      ");
    Serial.println(error, reportingPrecision);
    pyregress[j] = y;  // save regression value for group calculations
    averagepercenterrorHolder [j] = error; //save into array during calculation
    averageabsoluteerrorHolder[j] = absoluteError;  //save into array during calculation 
   }
   
   Serial.print("Avg Abs Err: ");
   Serial.println(averagecalc(N, averageabsoluteerrorHolder), regressionPrecision);
   Serial.print("Avg % Err: ");
   Serial.println(averagecalc(N, averagepercenterrorHolder),regressionPrecision);
   
   double rSquaredReturn = 0.0;
   determinationCoefficient(N, py, pyregress, 1, rSquaredReturn); // calcuate and print correlation coefficient
   Serial.print(F("r^2 = "));
   Serial.println(rSquaredReturn);
   //Serial.print(F("adjusted r^2 = ")); //adjusted R squared not used in this example
   //Serial.println(adjRSquaredReturn);   //adjusted R squared not used in this example
    
     if(reportToEEPROM == 1)
   {
      int expressionTotalTerms = (N + 1);
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
          regressionterms[q] = (1/(a[q]));
          }
      }
      itoa(reportInvertedValues, invertedStatus, 10);
      itoa(reportedConfiguredStatus, configuredStatus, 10);
      itoa(expressionTotalTerms, expressionTerms, 10);
      
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
  //Algorithm used based on:  https://forum.arduino.cc/index.php?topic=96292.0, general fitting examples, fortran and C 
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
      textSectionBreak();
      mask |= '\x04';
      z[2] = s;
      Serial.print("Exponential:   y = exp(");
      Serial.print(a2,regressionPrecision);
      Serial.print("( x ");
      Serial.print((char)sign);
      Serial.print(" )");
      Serial.print(fabs(a1),regressionPrecision);
      Serial.print(") ; s = ");
      Serial.println(s,regressionPrecision);
   }
  //end of regression algorithm
   regressionErrorHeader();  //Display the text header for the  fittign errors returned data (common display function used to save memory)
   double averagepercenterrorHolder[n] = {0}; //initialize holder array with all 0's; added this for exponential regression
   double averageabsoluteerrorHolder[n] = {0}; //initialize holder array with all 0's; added this for exponential regression 
   
   for (unsigned int i = 0; i < n; ++i)
   {
      double y = exp((a2) * px[i] + (a1)); //changed to properly calculate y values for exponential equation
      double absoluteError = y - py[i]; 

      pyregress[i] = y;
      
      double error = safeDiv((y - py[i]), py[i])*100.0;
      
      printRegErrors (px[i], py[i], y, absoluteError, error);
      
      averagepercenterrorHolder [i] = error; //save into array during calculation
      averageabsoluteerrorHolder[i] = absoluteError;  //save into array during calculation
   }
  double averageabsoluteerror = averagecalc(n, averageabsoluteerrorHolder); //calculate absolute error average
  double averagepercenterror = averagecalc(n, averagepercenterrorHolder); //calculate absolute error average
  Serial.print("Avg Abs Err: ");
  Serial.println(averageabsoluteerror, regressionPrecision);
  Serial.print(F("Avg % Err: "));
  Serial.println(averagepercenterror,regressionPrecision);
  
 double rSquaredReturn;
 determinationCoefficient(n, py, pyregress, 1, rSquaredReturn); // calcuate and print correlation coefficient

 Serial.print("r^2 = ");
 Serial.println(rSquaredReturn, reportingPrecision);
 //Serial.print(F("adjusted r^2 = "));  //adjusted r^2 used with mutivariate, not used in this regression type
 //Serial.println(adjRSquaredReturn, reportingPrecision); //adjusted r^2 used with mutivariate, not used in this regression type
 textSectionBreak();

 int append = 1;
 int inverted = 0; 
 char entryname[ENTRYNAMEMAX]={0};//entry name holder
 reportToEEPROM = saveToEEPROMPrompt(append, inverted, entryname, ENTRYNAMEMAX); //reference return append status
   if(reportToEEPROM == 1)
   {
      Serial.println(F("Recording values in EERPOM..."));
      int expressionTotalTerms = 2;
      char expressionTerms[4] = {0};
      char constant[EEPROMVariableBufferSize];
      char linear[EEPROMVariableBufferSize];
      if(reportInvertedValues == 1)
      {
          a1 = 1.0/(a1);
          a2 = 1.0/(a2); 
      }
      itoa(reportInvertedValues, invertedStatus, 10); //convert as base 10, integer to array to pass into EERPOM functions requiring CHAR inputs
      itoa(reportedConfiguredStatus, configuredStatus, 10); //convert as base 10, integer to array to pass into EERPOM functions requiring CHAR inputs
      itoa(expressionTotalTerms, expressionTerms, 10); //convert as base 10, integer to array to pass into EERPOM functions requiring CHAR inputs
      dtostrf(a1, EEPROMVariableLength, EEPROMDecimalPrecision, constant); // Leave room for too large numbers!
      dtostrf(a2, EEPROMVariableLength, EEPROMDecimalPrecision, linear); // Leave room for too large numbers!
      Serial.println();
      int currentoffset = ReadCalEEPROMHeader(configured_status_cached, totalentriesread_cached, eepromoffsetread_cached);
      EEPROMStatusMessages(1); //EEPROM readout message (redundant text pulled from function)
      Serial.println (currentoffset, DEC); //print the position as an integer
      //need to convert read offset to an int to feed into the next position to write the next EEPROM entry
      EEPROMStatusMessages(2); //EEPROM readout message (redundant text pulled from function)
      Serial.println ((offsetInEEPROM + currentoffset + 1), DEC);
      currentoffset = WriteCalEEPROM((offsetInEEPROM + currentoffset + 1),"sensor1","exponential", expressionTerms, invertedStatus, constant, linear, "0", "0", "0", "0", "0", "0", "0", "0", EEPROMCurrentPosition);  // values to write into EEPROM, variables unused up to 10 are reported as "0"
      //Note: use : as the denote of a new device entry
      EEPROMStatusMessages(3); //EEPROM readout message (redundant text pulled from function)
      Serial.println (currentoffset, DEC); //print the position as an integer
      EEPROMStatusMessages(5); //EEPROM readout message (redundant text pulled from function)
      Serial.println (EEPROMCurrentPosition, DEC);
      //using field append by adding 
      #ifdef EEPROMAppend
      WriteCalEEPROMHeader(EEPROMCurrentPosition, "1", 1); //Append option: update the header after the last write, write in last EEPROM address location
      #endif
      #ifndef EEPROMAppend
      WriteCalEEPROMHeader(offsetInEEPROM, "1", 1); ////using overwite next field as first as option: update the header after the last write, write in last EEPROM address location as first
      #endif
    }
  
   delay(1000); //end function after delay, make sure buffer is cleared
}

void fabls_log(unsigned int n,double *px,double *py)
{  
  //Algorithm used based on:  https://forum.arduino.cc/index.php?topic=96292.0, general fitting examples, fortran and C 
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
      textSectionBreak();
      //ardprintf("Logarithmic:   y = (%f) ln(x) %c %f; s = %f\n",a2,sign,fabs(a1),s);
      mask |= '\x08';
      z[3] = s;

      Serial.print(F("Logarithmic:   y = ("));
      Serial.print(a2,regressionPrecision);
      Serial.print(F(") ln(x) "));
      Serial.print((char)sign);
      Serial.print(" ");
      Serial.print("(");
      Serial.print(fabs(a1),regressionPrecision);
      Serial.print(") ; s = ");
      Serial.println(s,regressionPrecision);
   }
   double averagepercenterrorHolder[n] = {0}; //initialize holder array with all 0's
   double averageabsoluteerrorHolder[n] = {0}; //initialize holder array with all 0's
   regressionErrorHeader();  //Display the text header for the  fittign errors returned data (common display function used to save memory)
   for (unsigned int i = 0; i < n; ++i)
   {
      double y = (a2) * log(px[i]) + (a1); //change this 
      double absoluteError = y - py[i];
      // PercentError%=((regressionvalue-calibrationvalue)/calibrationvalue)*100 
      pyregress[i] = y;
      
      double error = safeDiv((y - py[i]),py[i])*100.0;
      printRegErrors (px[i], py[i], y, absoluteError, error);
      averagepercenterrorHolder [i] = error; //save into array during calculation
      averageabsoluteerrorHolder[i] = absoluteError;  //save into array during calculation
   }
   double averageabsoluteerror = averagecalc(n, averageabsoluteerrorHolder); //calculate absolute error average
   double averagepercenterror = averagecalc(n, averagepercenterrorHolder); //calculate absolute error average
   Serial.print(F("Avg Abs Err: "));
   Serial.println(averageabsoluteerror, regressionPrecision);
   Serial.print(F("Avg % Err: "));
   Serial.println(averagepercenterror,regressionPrecision);
   double rSquaredReturn;
   determinationCoefficient(n, py, pyregress, 1, rSquaredReturn); // calcuate and print correlation coefficient
   Serial.print(F("r^2 = "));
   Serial.println(rSquaredReturn);
   //Serial.print(F("adjusted r^2 = "));
   //Serial.println(adjRSquaredReturn); //  not used for this regression type and circumstance
   textSectionBreak();
   int append = 1;
 int inverted = 0; 
 char entryname[ENTRYNAMEMAX]={0};//entry name holder
 reportToEEPROM = saveToEEPROMPrompt(append, inverted, entryname, ENTRYNAMEMAX); //reference return append status
   if(reportToEEPROM == 1)
   {
    Serial.println(F("Recording values in EERPOM..."));
      int expressionTotalTerms = 2;
      char expressionTerms[4] = {0};
      char constant[EEPROMVariableBufferSize];
      char linear[EEPROMVariableBufferSize];
      if(reportInvertedValues == 1)
      {
          a1 = 1.0/(a1);
          a2 = 1.0/(a2); 
      }
      itoa(reportInvertedValues, invertedStatus, 10); //convert as base 10, integer to array to pass into EERPOM functions requiring CHAR inputs
      itoa(reportedConfiguredStatus, configuredStatus, 10); //convert as base 10, integer to array to pass into EERPOM functions requiring CHAR inputs
      itoa(expressionTotalTerms, expressionTerms, 10); //convert as base 10, integer to array to pass into EERPOM functions requiring CHAR inputs
      dtostrf(a1, EEPROMVariableLength, EEPROMDecimalPrecision, constant); // Leave room for too large numbers!
      dtostrf(a2, EEPROMVariableLength, EEPROMDecimalPrecision, linear); // Leave room for too large numbers!
      Serial.println();
      int currentoffset = ReadCalEEPROMHeader(configured_status_cached, totalentriesread_cached, eepromoffsetread_cached);
      EEPROMStatusMessages(1); //EEPROM readout message (redundant text pulled from function)
      Serial.println (currentoffset, DEC); //print the position as an integer
      //need to convert read offset to an int to feed into the next position to write the next EEPROM entry
      EEPROMStatusMessages(2); //EEPROM readout message (redundant text pulled from function)
      Serial.println ((offsetInEEPROM + currentoffset + 1), DEC);
      currentoffset = WriteCalEEPROM((offsetInEEPROM + currentoffset + 1),"sensor1","logarithmic", expressionTerms, invertedStatus, constant, linear, "0", "0", "0", "0", "0", "0", "0", "0", EEPROMCurrentPosition);  // values to write into EEPROM, variables unused up to 10 are reported as "0"
      //Note: use : as the denote of a new device entry
      EEPROMStatusMessages(3); //EEPROM readout message (redundant text pulled from function)
      Serial.println (currentoffset, DEC); //print the position as an integer
      EEPROMStatusMessages(5); //EEPROM readout message (redundant text pulled from function)
      Serial.println (EEPROMCurrentPosition, DEC);
      //using field append by adding 
      #ifdef EEPROMAppend
      WriteCalEEPROMHeader(EEPROMCurrentPosition, "1", 1); //Append option: update the header after the last write, write in last EEPROM address location
      #endif
      #ifndef EEPROMAppend
      WriteCalEEPROMHeader(offsetInEEPROM, "1", 1); ////using overwite next field as first as option: update the header after the last write, write in last EEPROM address location as first
      #endif
    }
  
   delay(1000); //end function after delay, make sure buffer is cleared
}

void fabls_power(unsigned int n,double *px,double *py)
{  
  //Algorithm used based on:  https://forum.arduino.cc/index.php?topic=96292.0, general fitting examples, fortran and C 
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
      textSectionBreak();
      //ardprintf("Power:   y = (%f) x ^ (%f); s = %f\n",a1,a2,s);
      mask |= '\x10';
      z[4] = s;

      Serial.print(F("Power:   y = ("));
      Serial.print((char)sign);
      Serial.print(fabs(a1),regressionPrecision);
      Serial.print(F(") x^ ("));
      Serial.print((char)sign2);
      Serial.print(fabs(a2),regressionPrecision);
      Serial.print(") ; s = ");
      Serial.println(s,regressionPrecision);
   }

   double averagepercenterrorHolder[n] = {0}; //initialize holder array with all 0's
   double averageabsoluteerrorHolder[n] = {0}; //initialize holder array with all 0's

   regressionErrorHeader();  //Display the text header for the  fittign errors returned data (common display function used to save memory)
   for (unsigned int i = 0; i < n; ++i)
   {
      double y = (a2) * px[i] + (a1);
      double absoluteError = y - py[i];
      // PercentError%=((regressionvalue-calibrationvalue)/calibrationvalue)*100 

      pyregress[i] = y;
     
      double error = safeDiv((y - py[i]),py[i])*100.0;
      printRegErrors (px[i], py[i], y, absoluteError, error);

      averagepercenterrorHolder [i] = error; //save into array during calculation
      averageabsoluteerrorHolder[i] = absoluteError;  //save into array during calculation
   }
   
   double averageabsoluteerror = averagecalc(n, averageabsoluteerrorHolder); //calculate absolute error average
   double averagepercenterror = averagecalc(n, averagepercenterrorHolder); //calculate absolute error average
   Serial.print(F("Avg Abs Err: "));
   Serial.println(averageabsoluteerror, regressionPrecision);
   Serial.print(F("Avg % Err: "));
   Serial.println(averagepercenterror,regressionPrecision);
   
   double rSquaredReturn;
   determinationCoefficient(n, py, pyregress, 1, rSquaredReturn); // calcuate and print correlation coefficient
   Serial.print(F("r^2 = "));
   Serial.println(rSquaredReturn);
   //Serial.print(F("adjusted r^2 = "));
   //Serial.println(adjRSquaredReturn); //  not used for this regression type and circumstance

   textSectionBreak();

      int append = 1;
 int inverted = 0; 
 char entryname[ENTRYNAMEMAX]={0};//entry name holder
 reportToEEPROM = saveToEEPROMPrompt(append, inverted, entryname, ENTRYNAMEMAX); //reference return append status
   if(reportToEEPROM == 1)
   {
    Serial.println(F("Recording values in EERPOM..."));
      int expressionTotalTerms = 2;
      char expressionTerms[4] = {0};
      char constant[EEPROMVariableBufferSize];
      char linear[EEPROMVariableBufferSize];
      if(reportInvertedValues == 1)
      {
          a1 = 1.0/(a1);
          a2 = 1.0/(a2); 
      }
      itoa(reportInvertedValues, invertedStatus, 10); //convert as base 10, integer to array to pass into EERPOM functions requiring CHAR inputs
      itoa(reportedConfiguredStatus, configuredStatus, 10); //convert as base 10, integer to array to pass into EERPOM functions requiring CHAR inputs
      itoa(expressionTotalTerms, expressionTerms, 10); //convert as base 10, integer to array to pass into EERPOM functions requiring CHAR inputs
      dtostrf(a1, EEPROMVariableLength, EEPROMDecimalPrecision, constant); // Leave room for too large numbers!
      dtostrf(a2, EEPROMVariableLength, EEPROMDecimalPrecision, linear); // Leave room for too large numbers!
      Serial.println();
      int currentoffset = ReadCalEEPROMHeader(configured_status_cached, totalentriesread_cached, eepromoffsetread_cached);
      EEPROMStatusMessages(1); //EEPROM readout message (redundant text pulled from function)
      Serial.println (currentoffset, DEC); //print the position as an integer
      //need to convert read offset to an int to feed into the next position to write the next EEPROM entry
      EEPROMStatusMessages(2); //EEPROM readout message (redundant text pulled from function)
      Serial.println ((offsetInEEPROM + currentoffset + 1), DEC);
      currentoffset = WriteCalEEPROM((offsetInEEPROM + currentoffset + 1),"sensor1","power", expressionTerms, invertedStatus, constant, linear, "0", "0", "0", "0", "0", "0", "0", "0", EEPROMCurrentPosition);  // values to write into EEPROM, variables unused up to 10 are reported as "0"
      //Note: use : as the denote of a new device entry
      EEPROMStatusMessages(3); //EEPROM readout message (redundant text pulled from function)
      Serial.println (currentoffset, DEC); //print the position as an integer
      EEPROMStatusMessages(5); //EEPROM readout message (redundant text pulled from function)
      Serial.println (EEPROMCurrentPosition, DEC);
      //using field append by adding 
      #ifdef EEPROMAppend
      WriteCalEEPROMHeader(EEPROMCurrentPosition, "1", 1); //Append option: update the header after the last write, write in last EEPROM address location
      #endif
      #ifndef EEPROMAppend
      WriteCalEEPROMHeader(offsetInEEPROM, "1", 1); ////using overwite next field as first as option: update the header after the last write, write in last EEPROM address location as first
      #endif
    }
  
   delay(1000); //end function after delay, make sure buffer is cleared
}

int fitSelection(int fitChoice, uint8_t skipEntry)
{ //equivelant to a main function, basic program operation from this function is called by the loop.  program resets when this function breaks causing it to be called again
  if (fitChoice == 0)
  {
      return 0;  // selection was not a valid fit choice. Return with no action
  }
  // Linear
  if(fitChoice == 1) 
  { 
    Serial.println(F("Fit: Linear"));
    if (skipEntry!=1)
    {
    pointInputProcess (); 
    }
    // Error and warning checks for minimum points
    if (totalPoints < 2)
    {
        pointNumberWarnings(2);
        delay(500);
        return 0;  // failed; returning to previous function 
    }
    else if (totalPoints == 2)
    {
       pointNumberWarnings(1);
    }
    delay(500);
    //manualPointEntry(totalPoints); //use manual points entry function to collect user input for x and y points, update global arrays for entered data
    if (skipEntry!=1)
    {
    dataEntrySelection(totalPoints);// select point entry method
    }
    fabls_linear(totalPoints, px, py); // send inputed points to fabls calculator  

    return 1; // successfully ran linear fitting
  }
  // Quadratic
  else if (fitChoice == 2) 
  {
    Serial.println("Fit Chose: Quadratic");

    if (skipEntry!=1)
    {
    pointInputProcess (); 
    }

    if (totalPoints < 3)
    {
        pointNumberWarnings(2);
        delay(500);
        return 0;  // failed; returning to previous function 
    }
    else if (totalPoints == 3)
    {
       pointNumberWarnings(1);
      
    }
    delay(500);
        if (skipEntry!=1)
    {
    dataEntrySelection(totalPoints);// select point entry method
    }
    //manualPointEntry(totalPoints); // use manual points entry function to collect user input for x and y points, update global arrays for entered data
    fabls_quad(totalPoints, px, py); // calculate quadratic regression

    return 1;  //successfully ran
  }
  // Exponential
  else if (fitChoice == 3) 
  {
    Serial.println("Fit: Exponential");
  if (skipEntry!=1)
    {
    pointInputProcess (); 
    }
    // Error and warning checks for minimum points
    if (totalPoints < 3)
    {
        pointNumberWarnings(2);
        delay(500);
        return 0;  // failed; returning to previous function 
    }
    else if (totalPoints == 3)
    {
       pointNumberWarnings(1);    
    }
    delay(500);
    //manualPointEntry(totalPoints);
        if (skipEntry!=1)
    {
    dataEntrySelection(totalPoints);// select point entry method
    }
    fabls_exp(totalPoints, px, py); //calculate exponential regression

    return 1;  //successfully ran
  }
  // Logarithmic
  else if (fitChoice == 4) 
  {
    Serial.println("Fit: Logarithmic");

   pointInputProcess (); 

    if (totalPoints < 3)
    {
        pointNumberWarnings (2);
        delay(500);
        return 0;  // failed; returning to previous function 
    }
    else if (totalPoints == 3)
    {
       pointNumberWarnings (1);
      
    }
    delay(500);
        if (skipEntry!=1)
    {
    dataEntrySelection(totalPoints);// select point entry method
    }
    //manualPointEntry(totalPoints);
    fabls_log(totalPoints, px, py); //calculate logarithmic regressions

    return 1; // successfully ran 
  }
  // Power
  else if (fitChoice == 5) 
  {
    Serial.println("Fit: Power");
    if (skipEntry!=1)
    {
    pointInputProcess (); 
    }
    
    if (totalPoints < 3)
    {
        pointNumberWarnings(2);
        delay(500);
        return 0;  // failed; returning to previous function 
    }
    else if (totalPoints == 3)
    {
       pointNumberWarnings(1);
      
    }
    delay(500);
    
        if (skipEntry!=1)
    {
    dataEntrySelection(totalPoints);// select point entry method
    }
    fabls_power(totalPoints, px, py); //calculate power regressions

    return 1; // successfully ran
  }
  // Polynomial
  else if (fitChoice == 6)
  {
    Serial.println("Fit: Polynomial");
    
    Serial.print(F("Polynomial Degree: "));   // prompt user
    int polynomialDegree = NumericIntegerInput();
    Serial.println(polynomialDegree);

    if (skipEntry!=1)
    {
    pointInputProcess (); 
    }
    
    if (totalPoints < (polynomialDegree + 1))
    {
        pointNumberWarnings(2);
        delay(500);
        return 0;   // failed; returning to previous function 
    }
    else if (totalPoints == (polynomialDegree + 1))
    {
       pointNumberWarnings(1);
      
    }
    delay(500);

        if (skipEntry!=1)
    {
    dataEntrySelection(totalPoints);// select point entry method
    }
    double regCoeff[(polynomialDegree + 1)] = {0};
    fabls_polynomial(totalPoints, polynomialDegree, px, py, regCoeff); //calculate polynomial regressions
    fabls_polyOutput(totalPoints, polynomialDegree, regCoeff, px, py);

    return 1; // successfully ran
  }

  else if (fitChoice == 7) 
  {
    Serial.println("Displaying Pts: ");

    if (totalPoints > 0)
    {
      listPoints(totalPoints, px, py);
    }
    else 
    {
      Serial.println(F("ERR: No pts entered..."));
    }
    
    return 1; // successfully ran
  }

  else if (fitChoice == 8)
  {
    adHocPointEntry();  //Enter manual points
  }

  else if (fitChoice == 9) 
  {
    totalPoints = 0; // 0 this off when other values are erased
    //reset pointer allocation
    delete[] px;
    delete[] py;
    delete[] pyregress;
    Serial.print("Pts removed from memory, restarting...");  

    return 1; // program restarts here
  }
    else if (fitChoice == 10) 
  {
    if (usecached == 0)
    {
      Serial.print("By toggling this option to 1 or 'On', fit will \nuse cache data for any subsequent fits.");
    }
    else
    {
      Serial.print("By toggling this option to 0 or 'Off', enter \npoints in on next fit.");
    }
    usecached = !usecached; // 0 this off when other values are erased
    //Serial.print("Cached: ");
    //Serial.println(usecached,DEC);

    return 1; // program restarts here
  }
  // Invalid
  else 
  {
    Serial.println(F("Invalid sel. Res. calibration..."));
    delay(500);
    return 0;  // Restart, jumps backs to beginning
  }

  return 0;  // default case - assuming invalid selection; should never reach this point
}

//Serial management and direct input read from serial
void serial_flush(void) {
  //read input buffer flush
  //Serial.print("Pre-Flush Serial Buffer Fill Status: ");  //debug line
  //Serial.println(Serial.available());  //debug line, pre-flush value
  while (Serial.available()) Serial.read();
  //Serial.print("Post Flush Serial Buffer Fill Status: ");  //debug line
  //Serial.println(Serial.available());  //debug line, should be zero
}

int manualPointEntry (int i) //i is total points entered --needs to have total points and px,py passed as areguements versus global modification
{
    px = new double[totalPoints]; // Load x's into array
    py = new double[totalPoints]; // Load y's into array
    for (uint8_t i = 0; i < totalPoints; ++i)       // loop through arrays and fill in values by input
    //Enter X values
    {
      Serial.print("Input val - x");
      Serial.print(i+1);
      Serial.print(": ");
      px[i] = (double)NumericFloatInput();
      Serial.print(" Entered Val: ");
      Serial.println(px[i]);
      delay(250); //nice, easy transisitonal delay to next input
      
      //Enter Y values
      Serial.print("Input val - y");
      Serial.print(i+1);
      Serial.print(": ");
      py[i] = (double)NumericFloatInput();
      Serial.print(" Entered Val: ");
      Serial.println(py[i]);
      delay(250);
    }
}
int dataEntrySelection (int pointssel) //selects the point entry method requested
{
  
  Serial.print (F("(1)Analog Read OR (2)Manual point entry?:"));
  int modeselection = NumericIntegerInput();
  Serial.println();
  if (modeselection == 1)
  {
    
    return (AnalogReadPointEntry (pointssel));  
  }
  else if (modeselection == 2)
  {
    return(manualPointEntry(pointssel));
  }
}
int AnalogReadPointEntry (int totalpointstosample) //i is total points entered --needs to have total points and px,py passed as areguements versus global modification
{
  px = new double[totalPoints]; // Load x's into array
  py = new double[totalPoints]; // Load y's into array
  double readx=0; //holder for the last X value readin
  Serial.print (("Enter Analog pin (often 0-5): "));
  int pinselection = NumericIntegerInput();
  Serial.println ();
  Serial.print (("(0)Median OR (1)Mean?"));  //can be used to promt for use of median or mean (average) function for calculation
  int mode = NumericIntegerInput();
  Serial.println ();
  Serial.print (("How mny. sets of 3 meas. to avg?(0 to 20): "));
  int averages = NumericIntegerInput(); //mult by 3 for mean-average algorithm
  Serial.println ();

    for (uint8_t i = 0; i < totalPoints; ++i)       // loop through arrays and fill in values by input
      {
      Serial.print ("Reading pt: ");
      Serial.println(i,DEC);
      int entryloopremain = 1;//default to stay in entry loop for the point
      while (entryloopremain == 1)
       {
        //Measure X values
        if (mode == 1)
        {
           readx = readSensorInputSimpleAverage(pinselection, averages, 0, 0); //an alternative for median averaged values also exists:
        }
        else if (mode == 0)
        {
          readx = readSensorInputMedian(pinselection, averages, 0, 0, 0, 0);
        }
        Serial.print("Measured x-val: ");
        Serial.print(readx,reportingPrecision);
        Serial.print("   <--keep this value? [Yes(1),Redo(2),Abort(3)]:");
        int valuaacceptance = NumericIntegerInput();
        if (valuaacceptance==1)
        {
          px[i] = readx;
          entryloopremain = 0;
        }
        else if (valuaacceptance==2)
        {
          entryloopremain = 1; //loop just repeats
          Serial.print(("Repeating Pt"));
        }
        else if (valuaacceptance==3)
        {
          //Serial.print(("Exiting Analog Read, return to prev menu"));
          return 0; //return 0 indicating failed function completion
        }
        }
      
      Serial.println(px[i]);
      delay(250); //nice, easy transisitonal delay to next input
      
      //Enter Y values
      Serial.print("Please enter calibrated y-value");
      Serial.print(i+1);
      Serial.print(": ");
      py[i] = (double)NumericFloatInput();
      Serial.print("Entered Value: ");
      Serial.println(py[i]);
      delay(250);
    }
    //Serial.print(("Pt entry complete"));
    return 1;//return 1 indicating sucessful function operation
}

int NumericIntegerInput()
{
  serial_flush(); //flush Serial buffer to prepare for next input
  while (Serial.available()==0)
  {
    //wait for user serial input
  }
  delay(250); //delay to allow buffered input to build up in this approach during serial transmission
  
  long SelectionfitChoice = Serial.parseInt(); //NOTE: NEWLINE must be enabled so that the the escape character is available!!
  serial_flush(); //flush Serial buffer to prepare for next input
  return ((int)SelectionfitChoice);
}

void CharArrayInput(char*inputarray, unsigned int maxchars)//maxchars is total entry alues, array must be maxchars+1 to accomodate the stop (NULL) char
{
  serial_flush(); //flush Serial buffer to prepare for next input
  char readvalue = 0; //initialize as null
  while (Serial.available()==0)
  {
    //wait for user serial input
  }
  delay(20); //delay to allow buffered input to build up in this approach during serial transmission
  unsigned int count=0;
  bool breakloop=0;
  while ((breakloop==0) && (count<maxchars)) 
  {
    readvalue = Serial.read(); //NOTE: NEWLINE must be enabled so that the the escape character is available!!
    if ((readvalue == 10) || (readvalue == 13))//newline or CR seen
      {
        readvalue = 0; //place in a null char
        inputarray[count] = readvalue; //place null character as last read value
        breakloop=1; //flag set to break loop on evaluation
      }
      else
      {
        inputarray[count] = readvalue;
      }
      count=count+1;//increase counter
      inputarray[count] = 0; //always add a null character ahead of the count (incremented before this line if it is larger than specified so array always ends with a null
  }

  serial_flush(); //flush Serial buffer to prepare for next input
}

float NumericFloatInput()
{
  serial_flush(); //flush Serial buffer to prepare for next input
  while (Serial.available()==0)
  {
    //wait for user serial input
  }
  delay(250); //delay to allow buffered input to build up in this approach during serial transmission
  
  float valueInput = Serial.parseFloat(); //NOTE: NEWLINE must be enabled so that the the escape character is available!!
  serial_flush(); //flush Serial buffer to prepare for next input
  return (valueInput);
}

int pointInputProcess () 
{ //this repitious input process is executed as a function to save memory.
    Serial.print(F("Input total pts: "));   // prompt user
    totalPoints = NumericIntegerInput();
    Serial.println(totalPoints);
    Serial.println(F("NOTE: X's are system values measured (often ADC values in arb. units), Y's are corresponding measurement calibrated values in final units"));
    return totalPoints; //return total points value (this may be global, to eventually make a passed value)
}

//Math helper functions
double ipow(double base, int exponent) //simplified calculation of power for an interger (rather than a float/double) exponent value
{
    double result = 1.0; //initialize
    for (;;)
    {
        if (exponent & 1)
            result *= base;
        exponent >>= 1;
        if (!exponent)
            break;
        base *= base;
    }

    return result;
}

double alog(double x)
{  
  return (x < 0) ? -log(-x) : ((x > 0) ? log(x) : 0);
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

double averagecalc(int len, double* values)
{
   double total = 0; //Initialize the total accumulator
   
   for(int index = 0; index < len; index++) 
   {
      total = total + values[index]; //add in each value in array
   }
   return ((double)total/(double)len); //div by total array elements

}

// Extra goodness of fit information
void determinationCoefficient(const int n, double *y, double *yRegression, const int regressors, double &rSquared)
{
    rSquared = 0.0; //initializing reference passed return
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
    //adjustedRSquared = 1.0-(((1.0-(rSquared))*(((double)n-1.0))/((double)n-((double)regressors+1.0))));  
}


//Text display functions, routine text displayed by functions to save space due to redundant literal vharacter arrays saved in prog flash or SRAM
void regressionErrorHeader(){
   Serial.print(F("X"));
   Serial.print(F("     Y"));
   Serial.print(F("         Calculated Y"));
   Serial.print(F("          Absolute Error"));
   Serial.println(F("                  PercentError%"));
}

void displayFitChoiceMenu()
{
    //Display opening menu
    // Menu select for function to fit against
  Serial.println();
  Serial.println(F("******Selection Fiting Menu******"));
  Serial.println();
  Serial.println(F("Select Regression Fitting Relationship: "));
  Serial.println(F("  (1)Linear - Min 2 pts"));
  Serial.println(F("  (2)Quadratic (2nd Order Polynomial) - Min 3 points")); //Specific commonly used sub-form of the general polynomial case.
  Serial.println(F("  (3)Exponential - Min 3 pts, y != 0"));  // Double check restrictions on exp, log, power
  Serial.println("  (4)Logarithmic - Min 3 pts, x != 0");
  Serial.println(F("  (5)Power - Min 3 pts, x != 0"));
  Serial.println(F("  (6)Nth Order Polynomial - Min pts = order+1")); //a 2nd power requires 2 points, 3rd power requires 3, etc.
  Serial.println(F("  (7)List Pts in Memory"));
  Serial.println(F("  (8)Manual Pt Entry"));
  Serial.println(F("  (9)Delete Current Points"));
  Serial.print(F("  (10)Toggle Enter New Points on fits, current status: "));
  Serial.println(usecached,DEC);
  Serial.println("  (0)Exit");
  Serial.print("Option (0-10): ");
}

void textSectionBreak(){
      Serial.println(); //add a space for the printed return to offset it from inputs
      Serial.println(F("******************"));
      Serial.println(); //add a space for the printed return to offset it from inputs
}

void pointNumberWarnings (unsigned int error){
    if (error==1)
     {
      Serial.println(F("WARNING - Min pts met, but more pts are recommended."));
     }
   else if (error==2)
     {
      Serial.println(F("ERR - Not enough pts. Exiting..."));
     }
   else
     {} //noting said if not a selected error from list
}

//EEPROM Handler Functions

int WriteCalEEPROMHeader(int eepromoffset, char* towrite_configured, int entries){  //function in development to take care of writing the first part of the EEPROM.
  #ifdef EEPROMDEBUG;
  Serial.println("Prep. EEPROM Header");
  #endif;
  char datatowrite[150] = {0};  //EEPROM entry character array holder
  char totalentries[4] = {0}; //holder
  char totalOffset[5] = {0}; //holder
  itoa(entries, totalentries, 10);  //convert the int into an entry in the char* array for the total entries in the eeprom
  itoa(eepromoffset, totalOffset, 10);  //convert the int into an entry in the char* array for the total entries in the eeprom

  char* sep = "#";
  //prepare EEPROM header
  strcat(datatowrite, towrite_configured);
  strcat(datatowrite, sep);
  strcat(datatowrite, totalentries);
  strcat(datatowrite, sep);
  strcat(datatowrite, totalOffset);
  strcat(datatowrite, sep);

  Serial.print(F("Header created: "));
  Serial.println(datatowrite);

  #ifdef EEPROMDEBUG;
  Serial.println(F("Saving Header"));
  #endif;
  int EEPROMReadLocation = save_data(EEPROMFirstaddress, datatowrite);  //load the final values into EERPOM, use the program defined value as the inital offset from EEPROM start
  #ifdef EEPROMDEBUG;
  Serial.print(F("Header Length (from 0): "));
  Serial.println(EEPROMReadLocation);
  #endif;
  delay (10);
  Serial.println(F("New Header"));
  return EEPROMReadLocation; //return last value location in EEPROM
}

int WriteDefaultEERPOM()  //call this to write the default values to "partition" the EEPROM, returns last EEPROM address
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

  char datatowrite[60] = {};  //EEPROM entry character array holder
  #ifdef EEPROMDEBUG;
  Serial.println(F("Preparing to save to EEPROM"));
  #endif;
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
  strcat(datatowrite,":");//Augment a colon to denote a new entry
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
  Serial.println(F("Default values to be updated to EEPROM: "));
  Serial.println(datatowrite);
  Serial.println();
  #ifdef EEPROMDEBUG;
  Serial.println(F("Saving New Defaults"));
  #endif;
  EEPROMReadLocation = save_data(eepromoffset, datatowrite);  //load the final values into EERPOM
  delay (10);
  #ifdef EEPROMDEBUG;
  Serial.println(F("EEPROM Defaults Updated..."));
  #endif;
  return EEPROMReadLocation; //return last EEPROM location
} //end of WriteCalEEPROM function
  
int WriteCalEEPROM(int eepromoffset, char* towrite_entryvalue_name, char* towrite_type_of_regression, char* expression_terms, char* towrite_inverted, char* towrite_cal_term1, char* towrite_cal_term2, char* towrite_cal_term3, char* towrite_cal_term4, char* towrite_cal_term5, char* towrite_cal_term6, char* towrite_cal_term7, char* towrite_cal_term8, char* towrite_cal_term9, char* towrite_cal_term10, int& EEPROMLocalReadLocation){
  char localdatatowrite[150] = {0};  //EEPROM entry character array holder
  Serial.println(F("Preparing to save Sensor Data to EEPROM"));
  char* sep = "#";
  strcat(localdatatowrite, ":");//Augment a colon to denote a new entry
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
  
  Serial.println(F("Data values to be updated to EEPROM: "));
  Serial.println(localdatatowrite);
  Serial.println();
  #ifdef EEPROMDEBUG;
  Serial.println(F("Saving Values"));
  #endif;
  EEPROMLocalReadLocation = save_data(eepromoffset, localdatatowrite);  //load the final values into EERPOM
  delay (10);
  #ifdef EEPROMDEBUG;
  Serial.println(F("Values Updated..."));
  #endif;
  return EEPROMLocalReadLocation; //return current EEPROM location for last entry
} //end of WriteCalEEPROM function

int ReadCalEEPROMHeader(char* configured_status, char* totalentriesread, char* eepromoffsetread){
  const int fields = 3;
  char data[150]={0};
  //Read in header values from EEPROM.
  EEPROM.begin(); //NOTE: this takes 0 arguments for AVR, but a value of 512 for ESP32
  delay(20); //allow serial to print before starting EEPROM to avoid line cutoff
  Serial.println(F("Reading Header in EEPROM"));
  delay(20);
  int count = 0;
  int address = EEPROMFirstaddress; //start at lowest address
  int returnedeepromvalue=0; 
  int returnedentries = 0;
  while (count < fields && (address<EEPROMFirstaddress+maxheaderreadaddress)){ //total field number to read in (total number of fields), must match total cases checked
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
      Serial.print(F("Read-In characters (between # marker): ")); //printout headers
      Serial.print(data); //printout data
      Serial.println(); //printout separator, line break
      #endif
    }
    ++address;
  }
  #ifdef EEPROMDEBUG;
  Serial.println(F("<--Read Complete")); 
  #endif;
  delay(10);
    returnedeepromvalue = atoi (eepromoffsetread); //convert returned array values to integers
    returnedentries = atoi (totalentriesread);  //convert returned array values to integers
    return returnedeepromvalue; //return the value as an integer that was read as the last EEPROM location
}

int save_data(int offset, char* datalocal){
  //Mechanism called by another function to write pre-packaged data to the EEPROM
  #ifdef EEPROMDEBUG;
  Serial.println(F("Call to EEPROM Write Function"));
  #endif;
  delay (20);  //allow serial 
  EEPROM.begin(); //this takes 0 arguments for AVR, but a value of 512 for ESP32
  int index = 0; //variable to save final index location
  for (int i = offset; i < (strlen(datalocal)+ offset); ++i){
    EEPROM.write(i, (int)datalocal[i]);
    delay(1);
    index = i;
  }
  //EEPROM.commit(); //Not required for AVR, only ESP32
  Serial.print(F("Write Function started, address: "));
  Serial.println(offset);  // last EEPROM address written  //debug line
  Serial.print(F("...Complete, total entries for session: ")); //debug line
  Serial.println(index);  // last EEPROM address written  //debug line
  return index;  // last EEPROM address written
  delay(10);
} //end of save_data function define

int saveToEEPROMPrompt (int& appendedquestion, int& invertedquestion, char* inputvaluename, unsigned int inputaraylength){
  Serial.print(F("Save Regression to EEPROM (0=No, 1=Yes): "));
  int selectionValue =  NumericIntegerInput();
  Serial.println();
  int enteredcorrectly = 0;
    if (selectionValue==1) //force to loop until user indicates correctly entered 
      {
          while (enteredcorrectly == 0)
          {
           Serial.print (F("(1)Append OR (2)Overwrite, EEPROM?: "));
           appendedquestion = NumericIntegerInput();
           Serial.println();
           Serial.print (F("Input Reference Name ( "));  
           Serial.print (inputaraylength,DEC); //display specified array length for user inputted characters)
           Serial.print (F(" chars max.): "));
           Serial.println();
           CharArrayInput(inputvaluename, (inputaraylength));//function to read this in as inputvaluename and return by reference
           Serial.print (F("Inputted: "));
           Serial.println (inputvaluename);
           Serial.println();
           Serial.print (F("Save as recip. values? (0=No, 1=Yes): "));
           invertedquestion = NumericIntegerInput();
           Serial.println();
           Serial.println (F("All Prev values entered correctly, save? (0=Reenter vals, 1=Save, 2=Abort): "));
           enteredcorrectly = NumericIntegerInput();
           if (enteredcorrectly==2) //abort sequence
             {
              #ifdef EEPROMDEBUG;
              Serial.println (F("Saving Skipped... Exiting"));
              #endif;
              selectionValue = 0;
              return selectionValue; //leave here
             }
          }
     #ifdef EEPROMDEBUG;
     Serial.println (F("Saving...")); 
     #endif;
    }
    else if (selectionValue==0)
    {
      #ifdef EEPROMDEBUG;
      Serial.println (F("Saving Skipped... returning menus"));
      #endif;
    }
  return selectionValue;
  
}

void EEPROMStatusMessages (unsigned int statusinput){
    if (statusinput==1)
     {
      #ifdef EEPROMDEBUG;
      Serial.print (F("Read-out EEPROM header length: "));
      #endif;
     }
   else if (statusinput==2)
     {
      #ifdef EEPROMDEBUG;
      Serial.print (F("Position to write data to: "));
      #endif;
     }
        else if (statusinput==3)
     {
      #ifdef EEPROMDEBUG;
      Serial.print (F("Written data current end address: "));
      #endif;
     }
        else if (statusinput==4)
     {
      #ifdef EEPROMDEBUG;
      Serial.print (F("Position to write data to: "));
      #endif;
     }
      else if (statusinput==5)
     {
      #ifdef EEPROMDEBUG;
      Serial.print (F("Returned final data written address position: "));
      #endif;
     }
   else
     {} //nothing said if not a selected error from list
}

//Sensor Input functions
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

void listPoints(int totalPoints, double* px, double* py)
{
  Serial.print(("Pts in memory: "));
  Serial.println(totalPoints, DEC);
  Serial.println();
  for (int i = 0; i < totalPoints; i++)
  {
    Serial.print("(");
    Serial.print(px[i]);
    Serial.print(",");
    Serial.print(py[i]); 
    Serial.print(")");
    Serial.println();
  }
}
void printColumnSpace()
{
 Serial.print(F("      "));
}
void printRegErrors (double valx, double valy, double y, double absoluteError, double error) 
{
  Serial.print(valx,reportingPrecision);
  printColumnSpace();
  Serial.print(valy,reportingPrecision);
  printColumnSpace();
  Serial.print(y,reportingPrecision);
  printColumnSpace();
  Serial.print(absoluteError,reportingPrecision);
  printColumnSpace();
  Serial.println(error,reportingPrecision);
}
  
void adHocPointEntry()
{
    Serial.println("Add pts: ");
    pointInputProcess(); 
    
    dataEntrySelection (totalPoints);// select point entry method
}

void loop() 
{
  bool dealocArrays=0;//keep arrays, flag is normal pos.
  unsigned int selectedValue = 0; //initialize selection choice holder as 0
  unsigned int runStatus = 0;  // Status of previous function's run, if an error is returned, this will come back as zero, chose next action accordingly 
  displayFitChoiceMenu(); //display menu to user
  selectedValue = NumericIntegerInput();
  Serial.println(); //add in line break after input comes in for selection
  runStatus = fitSelection(NumericIntegerInput(),usecached); //take input from menu selection then process input and fits, skipentry argument in place to see if skip needed

  if (runStatus == 0) //function did not run to completion sucessfully, choose followup action
  {
   Serial.print(F("ERR: ret. to Selection Fitting Menu..."));
  }
  else if (runStatus == 1) //function did run to completion sucessfully, choose followup action
  {
    Serial.println();
    Serial.println();
    Serial.print(F("Program Comp."));
  }
  if (dealocArrays ==1)
  {
     dealocArrays =0; //reset flag
    // deallocation reset array before executing new instance if requested
    delete[] px;
    delete[] py;
    delete[] pyregress;
  }
} 
