The following offers insight to how this library works, and a few changes that would need
to be made in case any changes are added.

-- The Program is designed to perform regressions on a given set of data points(x and y values). It then determines the coefficient of each term of the resulting
function. There are 6 regression functions that use the Fits Analysis by Least Squares Approach: Linear, Logarithmic, Exponential, Power, Quadratic, and Nth order. All take the total number of points, array of X values, and array of Y values. Some might take more arguments to perform the regression.

-- After the regression has been performed, the coefficients will be stored in variables within the function, which can be written to the EEPROM. The EEPROM writing functions take the array of constants, along with all the relevant information, and save it to the EEPROM(the program contains comments on how the EEPROM is formatted).

-- If a saved sensor/function is to be used in the future, one can use the getArrayOfConstants function to return an array of the constants that were calculated from the regression, which can then be used to reconstruct the function for further use. The array of constants will be in the order they appear in the function produced from the regression i.e. from the higher order terms to the lower order terms.

---------------------------------------------------------------------------------
IN CASE OF CHANGES:
1. In the readField(int whereToBeginReading) function, the max length of the name of each function in hardcoded to be 15(a conservative but sufficient number). In case any function who's name's length is larger than 15, update this value.
---------------------------------------------------------------------------------
KNOWN ISSUES:
1. Some decimal numbers are not written completely the same, but they only differ in the last decimal. So the error is less than 1%.
2. Nth order regression sometimes functions correctly if given reasonable points, and a low Nth order (less than 3), but sometimes overflows.
