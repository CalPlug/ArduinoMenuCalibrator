// Helper function for calibration of ADE9078 by Avinash Pai with contributions cited.
#include <math.h>
//#include <EEPROM.h>                      
#define ARDBUFFER 16
#include <stdarg.h>
#include <Arduino.h>

const byte buffSize = 32;
char inputSeveral[buffSize]; // space for 31 chars and a terminator

byte maxChars = 12; // a shorter limit to make it easier to see what happens
                   //  if too many chars are entered
String fit;


// NOTE DELAYS TEMPORARY - WHILE LOOP FOR INPUT NOT WORKING
void setup() {
  Serial.begin(9600);
  Serial.println("Starting...");

  Serial.println("Select fit: ");
  Serial.println("  (1)Linear - Minimum two points");
  Serial.println("  (2)Quadratic - Minimum three points");
  Serial.println("  (3)Exponential - Minimum three points, no zero points");
  Serial.println("  (4)Logarithmic - Minimum three points, not zero points");
  Serial.println("  (5)Power - Minimum three points, no zero points");
  Serial.println("  (0)Exit");
  delay(3000);
  readSeveralChars();
  unsigned int fitChoice = atoi(inputSeveral);

  // Exit
  if (fitChoice == 0)
  {
      Serial.print("Exiting calibration process...");
      delay(2000);
      exit(0);
  }
  // Linear
  if(fitChoice == 1) { 
    fit = "Linear";
    Serial.print("Fit Chosen: ");
    Serial.println(fit);

    Serial.print("Input total points: ");
    delay(2000);
    readSeveralChars();
    unsigned int totalPoints = atoi(inputSeveral);
    Serial.println(totalPoints);

    if (totalPoints < 2)
    {
        Serial.print("At least two points needed for linear. Restarting calibration process...");
        delay(2000);
        setup();
    }
    else if (totalPoints == 2)
    {
       Serial.println("WARNING - Minimum points met. Overdefined recommended.");
      
    }
    delay(3000);
    double px[totalPoints];
    double py[totalPoints];
    for (unsigned int i = 0; i < totalPoints; ++i)
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
    fabls_linear(totalPoints, px, py);
  }
  // Quadratic
  else if (fitChoice == 2) {
    fit = "Quadratic";
    Serial.print("Fit Chosen: ");
    Serial.println(fit);

    Serial.print("Input total points: ");
    delay(2000);
    readSeveralChars();
    unsigned int totalPoints = atoi(inputSeveral);
    Serial.println(totalPoints);

    if (totalPoints < 3)
    {
        Serial.print("At least three points needed for quadratic. Restarting calibration process...");
        delay(2000);
        setup();
    }
    else if (totalPoints == 3)
    {
       Serial.println("WARNING - Minimum points met. Overdefined recommended.");
      
    }
    delay(3000);
    double px[totalPoints];
    double py[totalPoints];
    for (unsigned int i = 0; i < totalPoints; ++i)
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
    fabls_quad(totalPoints, px, py);
    
  }
  // Exponential
  else if (fitChoice == 3) {
    fit = "Exponential";
    Serial.print("Fit Chosen: ");
    Serial.println(fit);

    Serial.print("Input total points: ");
    delay(2000);
    readSeveralChars();
    unsigned int totalPoints = atoi(inputSeveral);
    Serial.println(totalPoints);

    if (totalPoints < 3)
    {
        Serial.print("ERROR - At least three points needed for exponential. Restarting calibration process...");
        delay(2000);
        setup();
    }
    else if (totalPoints == 3)
    {
       Serial.println("WARNING - Minimum points met. Overdefined recommended.");
      
    }
    delay(3000);
    double px[totalPoints];
    double py[totalPoints];
    for (unsigned int i = 0; i < totalPoints; ++i)
    {
      ardprintf("Input x%d", i+1);
      delay(2000);
      readSeveralChars();
      px[i] = atof(inputSeveral);
      Serial.println(px[i]);
      if (px[i] == 0)
      {
          Serial.println("ERROR - No zero points for exponential. Restarting calibration process... ");
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
    fabls_exp(totalPoints, px, py);

    
  }
  // Logarithmic
  else if (fitChoice == 4) {
    fit = "Logarithmic";
    Serial.print("Fit Chosen: ");
    Serial.println(fit);

    Serial.print("Input total points: ");
    delay(2000);
    readSeveralChars();
    unsigned int totalPoints = atoi(inputSeveral);
    Serial.println(totalPoints);

    if (totalPoints < 3)
    {
        Serial.print("At least three points needed for logarithmic. Restarting calibration process...");
        delay(2000);
        setup();
    }
    else if (totalPoints == 3)
    {
       Serial.println("WARNING - Minimum points met. Overdefined recommended.");
      
    }
    delay(3000);
    double px[totalPoints];
    double py[totalPoints];
    for (unsigned int i = 0; i < totalPoints; ++i)
    {
      ardprintf("Input x%d", i+1);
      delay(2000);
      readSeveralChars();
      px[i] = atof(inputSeveral);
      Serial.println(px[i]);
      if (px[i] == 0)
      {
          Serial.println("ERROR - No zero points for logarthimic. Restarting calibration process...");
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
    fabls_log(totalPoints, px, py);
  }
  // Power
  else if (fitChoice == 5) {
    fit = "Power";
    Serial.print("Fit Chosen: ");
    Serial.println(fit);

    Serial.print("Input total points: ");
    delay(2000);
    readSeveralChars();
    unsigned int totalPoints = atoi(inputSeveral);
    Serial.println(totalPoints);

    if (totalPoints < 3)
    {
        Serial.print("At least three points needed for power. Restarting calibration process...");
        delay(2000);
        setup();
    }
    else if (totalPoints == 3)
    {
       Serial.println("WARNING - Minimum points met. Overdefined recommended.");
      
    }
    delay(3000);
    double px[totalPoints];
    double py[totalPoints];
    for (unsigned int i = 0; i < totalPoints; ++i)
    {
      ardprintf("Input x%d", i+1);
      delay(2000);
      readSeveralChars();
      px[i] = atof(inputSeveral);
      Serial.println(px[i]);
      if (px[i] == 0)
      {
        Serial.println("ERROR - No zero points for power. Restarting calibration process...");
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
    fabls_power(totalPoints, px, py);
  }
  // Invalid
  else {
    Serial.println("Invalid choice. Restarting calibration process...");
    delay(2000);
    setup();  
  }
  
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
{  byte mask='\x00',sign,sign2;
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
      ardprintf("Linear:      y = (%f) x %c %f; s = %f\n",a2,sign,fabs(a1),s);
      mask |= '\x01';
      z[0] = s;
   }
}

void fabls_quad(unsigned int n,double *px,double *py)
{  byte mask='\x00',sign,sign2;
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
}


void fabls_exp(unsigned int n,double *px,double *py)
{  byte mask='\x00',sign,sign2;
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
}



void fabls_log(unsigned int n,double *px,double *py)
{  byte mask='\x00',sign,sign2;
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
      ardprintf("Logarithmic: y = (%f) ln(x) %c %f; s = %f\n",a2,sign,fabs(a1),s);
      mask |= '\x08';
      z[3] = s;
   }
}



void fabls_power(unsigned int n,double *px,double *py)
{  byte mask='\x00',sign,sign2;
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
      ardprintf("Power:       y = (%f) x ^ (%f); s = %f\n",a1,a2,s);
      mask |= '\x10';
      z[4] = s;
   }
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
