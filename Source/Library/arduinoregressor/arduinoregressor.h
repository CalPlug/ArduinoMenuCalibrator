//Arduino Based Multi-form Least-Squares Regression Calibration Utility
//Development by Avinash Pai and Bryan Karimi with component contributions cited, August 5, 2019
//Note: This program uses cited numeric calculation methods developed in part or in whole by other contributers.  The developers of this utility thank these authors for helping us develop this utility!
//Released under GNU Licence, Copyright (2019) Regents of the University of California 

#ifndef StatisticalCalibrator_h
#define StatisticalCalibrator_h

#include "Arduino.h" 

class StatisticalCalibrator
{
	public:
		StatisticalCalibrator(int fitSelection);

		//int fitSelection(int fitChoice, uint8_t skipEntry); //this will be merged into StatisticalCalibrator constructor
		void displayFitChoiceMenu();
		
		int manualPointEntry(int i);
		int dataEntrySelection(int pointssel);
		int AnalogReadPointEntry(int totalpointstosample);
		int NumericIntegerInput();
		float NumericFloatInput();
		int pointInputProcess();
		void CharArrayInput(char*inputarray, unsigned int maxchars);
		
		double ipow(double base, int exponent);
		double alog(double x);
		double safeDiv(double numerator, double denominator);
		double averagecalc(int len, double* values);
		
		void fabls_linear(unsigned int n, double *px, double *py);
		void fabls_quad(unsigned int n, double *px, double *py);
		void fabls_polynomial(unsigned int N, unsigned int n, double *px, double *py, double *regCoeff); // Arguments: (Total number of points, order of regression, x-pints, y-pounts)
		void fabls_polyOutput(unsigned int N, unsigned int n, double *a, double *px, double *py);
		void fabls_exp(unsigned int n, double *px, double *py);
		void fabls_log(unsigned int n, double *px, double *py);
		void fabls_power(unsigned int n, double *px, double *py);
		
		void determinationCoefficient(const int n, double *y, double *yRegression, const int regressors, double &rSquared);
		void regressionErrorHeader();
		
		void textSectionBreak();
		void printColumnSpace();

		void serial_flush(void);
		
		void pointNumberWarnings(unsigned int error);
		void printRegErrors(double valx, double valy, double y, double absoluteError, double error);
		void adHocPointEntry();

		int WriteCalEEPROMHeader(int eepromoffset, char* towrite_configured, int entries);
		int WriteDefaultEEPROM();
		int WriteCalEEPROM(int eepromoffset, char* towrite_entryvalue_name, char* towrite_type_of_regression, char* expression_terms, char* towrite_inverted, char* towrite_cal_term1, char* towrite_cal_term2, char* towrite_cal_term3, char* towrite_cal_term4, char* towrite_cal_term5, char* towrite_cal_term6, char* towrite_cal_term7, char* towrite_cal_term8, char* towrite_cal_term9, char* towrite_cal_term10, int& EEPROMLocalReadLocation);
		int ReadCalEEPROMHeader(char* configured_status, char* totalentriesread, char* eepromoffsetread);
		int save_data(int offset, char* datalocal);
		int saveToEEPROMPrompt(int& appendedquestion, int& invertedquestion, char* inputvaluename, unsigned int inputaraylength);
		void EEPROMStatusMessages(unsigned int statusinput);
		double readSensorInputMedian(int inputpin, int readcycles, bool enabSensorReadDelay, bool enabavgSensorReadDelay, int sensorReadDelay, int avgsensorReadDelay);
		double readSensorInputSimpleAverage(int inputpin, int readcycles, bool enabavgSensorReadDelay, int avgsensorReadDelay);

		void listPoints(int totalPoints, double* px, double* py);

	private:
		int _fitSelection;

};

#endif
