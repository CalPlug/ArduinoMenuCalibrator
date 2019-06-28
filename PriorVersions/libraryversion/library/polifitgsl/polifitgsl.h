#ifndef _POLIFITGSL_H
#define _POLIFITGSL_H




#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>
#include <math.h>
#include "gsl/gsl_multifit.h"
#include "gsl/gsl_math.h"
#include "gsl/gsl_vector.h"
#include "gsl/gsl_test.h"
#include "gsl/gsl_rng.h"
#include "gsl/gsl_movstat.h"
#include "gsl/gsl_block.h"
#include "gsl/gsl_matrix.h"

	bool polynomialfit(int obs, int degree, 
			   double *dx, double *dy, double *store); /* n, p */
#ifdef __cplusplus
}
#endif

#endif