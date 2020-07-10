![Calit2](https://upload.wikimedia.org/wikipedia/en/2/25/Calit2.png)

# ArduinoMenuCalibrator 
![build passing](https://img.shields.io/circleci/project/github/badges/shields/master.svg)

## California Plug Load Research Center (CalPlug) & California Institute of Telecommunications and Information Technology (Calit2)

##### Code development by Mugared Khalifa, Kejia Qiang, Bryan Karmini and Avinash Pai
##### University of California, Irvine (UC Irvine) 
##### Project Leaders: Dr. Michael J. Klopfer & Prof. G.P. Li 
Copyright The Regents of the University of California, 2019

Built with open source software and released into the public domain under GNU License for permissive use.  

Description:  This menu driven script provides the capability for using regression to determine fitting coefficients for linear, quadratic, power, exponential and logarithmic functions from input data either supplied or measured via microcontroller ADCs.  It can be used for sensor calibration and provide fitting summaries and coefficients.  After reading values and calculating fits and goodness of fits (along with average and absolute errors) it will allow the saving of fitting cofficients into onboard EEPROM to be recalled by anothe rprogram.  The intent of this operation is to use this as a program to calibrate onbboard sensors where the saved data can be recalled from the EEPROM as required.  It has been tested on Arduino Uno and Arduino Mega.
