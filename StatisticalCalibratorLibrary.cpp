//Arduino Based Multi-form Least-Squares Regression Calibration Utility
//Development by Avinash Pai and Bryan Karimi with component contributions cited, August 5, 2019
//Note: This program uses cited numeric calculation methods developed in part or in whole by other contributers.  The developers of this utility thank these authors for helping us develop this utility!
//Released under GNU Licence, Copyright (2019) Regents of the University of California 


#include "Arduino.h"
#include "StatisticalCalibrator.h"
#include <iostream>
#include <string>

using namespace std;

StatisticalCalibrator::StatisticalCalibrator(int fitSelection)
{
	_fitSelection = fitSelection;
	if (fitSelection == 0)
	{
		return;  // selection was not a valid fit choice. Return with no action
	}
	// Linear
	if (fitSelection == 1)
	{
		cout << "Fit: Linear";
		if (skipEntry != 1)
		{
			pointInputProcess();
		}
		// Error and warning checks for minimum points
		if (totalPoints < 2)
		{
			pointNumberWarnings(2);
			return;  // failed; returning to previous function 
		}
		else if (totalPoints == 2)
		{
			pointNumberWarnings(1);
		}
		//manualPointEntry(totalPoints); //use manual points entry function to collect user input for x and y points, update global arrays for entered data
		if (skipEntry != 1)
		{
			dataEntrySelection(totalPoints);// select point entry method
		}
		fabls_linear(totalPoints, px, py); // send inputed points to fabls calculator  

		return; // successfully ran linear fitting
	}
	// Quadratic
	else if (fitSelection == 2)
	{
		cout << "Fit: Quadratic";

		if (skipEntry != 1)
		{
			pointInputProcess();
		}

		if (totalPoints < 3)
		{
			pointNumberWarnings(2);
			return;  // failed; returning to previous function 
		}
		else if (totalPoints == 3)
		{
			pointNumberWarnings(1);

		}
		
		if (skipEntry != 1)
		{
			dataEntrySelection(totalPoints);// select point entry method
		}
		//manualPointEntry(totalPoints); // use manual points entry function to collect user input for x and y points, update global arrays for entered data
		fabls_quad(totalPoints, px, py); // calculate quadratic regression

		return;  //successfully ran
	}
	// Exponential
	else if (fitSelection == 3)
	{
		cout << "Fit: Exponential";
		if (skipEntry != 1)
		{
			pointInputProcess();
		}
		// Error and warning checks for minimum points
		if (totalPoints < 3)
		{
			pointNumberWarnings(2);
			
			return;  // failed; returning to previous function 
		}
		else if (totalPoints == 3)
		{
			pointNumberWarnings(1);
		}
		
		//manualPointEntry(totalPoints);
		if (skipEntry != 1)
		{
			dataEntrySelection(totalPoints);// select point entry method
		}
		fabls_exp(totalPoints, px, py); //calculate exponential regression

		return;  //successfully ran
	}
	// Logarithmic
	else if (fitSelection == 4)
	{
		cout << "Fit: Logarithmic";

		pointInputProcess();

		if (totalPoints < 3)
		{
			pointNumberWarnings(2);
			
			return;  // failed; returning to previous function 
		}
		else if (totalPoints == 3)
		{
			pointNumberWarnings(1);

		}
		
		if (skipEntry != 1)
		{
			dataEntrySelection(totalPoints);// select point entry method
		}
		//manualPointEntry(totalPoints);
		fabls_log(totalPoints, px, py); //calculate logarithmic regressions

		return; // successfully ran 
	}
	// Power
	else if (fitSelection == 5)
	{
		cout << "Fit: Power";
		if (skipEntry != 1)
		{
			pointInputProcess();
		}

		if (totalPoints < 3)
		{
			pointNumberWarnings(2);
			
			return;  // failed; returning to previous function 
		}
		else if (totalPoints == 3)
		{
			pointNumberWarnings(1);

		}
		

		if (skipEntry != 1)
		{
			dataEntrySelection(totalPoints);// select point entry method
		}
		fabls_power(totalPoints, px, py); //calculate power regressions

		return; // successfully ran
	}
	// Polynomial
	else if (fitSelection == 6)
	{
		cout << "Fit: Polynomial";

		cout << "Polynomial Degree: ";   // prompt user
		int polynomialDegree = NumericIntegerInput();
		cout << polynomialDegree;

		if (skipEntry != 1)
		{
			pointInputProcess();
		}

		if (totalPoints < (polynomialDegree + 1))
		{
			pointNumberWarnings(2);
			
			return;   // failed; returning to previous function 
		}
		else if (totalPoints == (polynomialDegree + 1))
		{
			pointNumberWarnings(1);

		}
		

		if (skipEntry != 1)
		{
			dataEntrySelection(totalPoints);// select point entry method
		}
		double regCoeff[(polynomialDegree + 1)] = { 0 };
		fabls_polynomial(totalPoints, polynomialDegree, px, py, regCoeff); //calculate polynomial regressions
		fabls_polyOutput(totalPoints, polynomialDegree, regCoeff, px, py);

		return; // successfully ran
	}

	else if (fitSelection == 7)
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

		return; // successfully ran
	}

	else if (fitSelection == 8)
	{
		adHocPointEntry();  //Enter manual points

		return; // successfully ran
	}

	else if (fitSelection == 9)
	{
		totalPoints = 0; // 0 this off when other values are erased
		//reset pointer allocation
		delete[] px;
		delete[] py;
		delete[] pyregress;
		cout << "Pts removed, restarting...";

		return; // program restarts here
	}
	else if (fitSelection == 10)
	{
		if (usecached == 0)
		{
			cout << "By toggling this option to 1 or 'On', fit will \nuse cache data for any subsequent fits.";
		}
		else
		{
			cout << "By toggling this option to 0 or 'Off', enter \npoints in on next fit.";
		}
		usecached = !usecached; // 0 this off when other values are erased
		//Serial.print("Cached: ");
		//Serial.println(usecached,DEC);

		return; // program restarts here
	}
	// Invalid
	else
	{
		cout << "Invalid sel. Res. calibration...";
		
		return;  // Restart, jumps backs to beginning
	}

	return;  // default case - assuming invalid selection; should never reach this point
}
