/*
 * Copyright (c) 1988-1997 Sam Leffler
 * Copyright (c) 1991-1997 Silicon Graphics, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and
 * its documentation for any purpose is hereby granted without fee, provided
 * that (i) the above copyright notices and this permission notice appear in
 * all copies of the software and related documentation, and (ii) the names of
 * Sam Leffler and Silicon Graphics may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Sam Leffler and Silicon Graphics.
 *
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 *
 * IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
 * ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#include <tiffio.h>
#include <assert.h> /*--: for Rational2Double */
#include <math.h>  /*--: for Rational2Double */

/** -----  Rational2Double: Double To Rational Conversion
----------------------------------------------------------
* There is a mathematical theorem to convert real numbers into a rational
(integer fraction) number.
* This is called "continuous fraction" which uses the Euclidean algorithm to
find the greatest common divisor (GCD).
*  (ref. e.g. https://de.wikipedia.org/wiki/Kettenbruch or
https://en.wikipedia.org/wiki/Continued_fraction
*             https://en.wikipedia.org/wiki/Euclidean_algorithm)
* The following functions implement the
* - ToRationalEuclideanGCD()		auxiliary function which mainly
implements euclidean GCD
* - DoubleToRational()			conversion function for un-signed
rationals
* - DoubleToSrational()			conversion function for signed rationals
------------------------------------------------------------------------------------------------------------------*/

/**---- ToRationalEuclideanGCD() -----------------------------------------
* Calculates the rational fractional of a double input value
* using the Euclidean algorithm to find the greatest common divisor (GCD)
------------------------------------------------------------------------*/
static void TIFFToRationalEuclideanGCD(double value, int blnUseSignedRange,
                                       int blnUseSmallRange, uint64_t *ullNum,
                                       uint64_t *ullDenom)
{
    /* Internally, the integer variables can be bigger than the external ones,
     * as long as the result will fit into the external variable size.
     */
    uint64_t numSum[3] = {0, 1, 0}, denomSum[3] = {1, 0, 0};
    uint64_t aux, bigNum, bigDenom;
    uint64_t returnLimit;
    int i;
    uint64_t nMax;
    double fMax;
    unsigned long maxDenom;
    /*-- nMax and fMax defines the initial accuracy of the starting fractional,
     *   or better, the highest used integer numbers used within the starting
     * fractional (bigNum/bigDenom). There are two approaches, which can
     * accidentally lead to different accuracies just depending on the value.
     *   Therefore, blnUseSmallRange steers this behavior.
     *   For long long nMax = ((9223372036854775807-1)/2); for long nMax =
     * ((2147483647-1)/2);
     */
    if (blnUseSmallRange)
    {
        nMax = (uint64_t)((2147483647 - 1) / 2); /* for ULONG range */
    }
    else
    {
        nMax = ((9223372036854775807 - 1) / 2); /* for ULLONG range */
    }
    fMax = (double)nMax;

    /*-- For the Euclidean GCD define the denominator range, so that it stays
     * within size of unsigned long variables. maxDenom should be LONG_MAX for
     * negative values and ULONG_MAX for positive ones. Also the final returned
     * value of ullNum and ullDenom is limited according to signed- or
     * unsigned-range.
     */
    if (blnUseSignedRange)
    {
        maxDenom = 2147483647UL; /*LONG_MAX = 0x7FFFFFFFUL*/
        returnLimit = maxDenom;
    }
    else
    {
        maxDenom = 0xFFFFFFFFUL; /*ULONG_MAX = 0xFFFFFFFFUL*/
        returnLimit = maxDenom;
    }

    /*-- First generate a rational fraction (bigNum/bigDenom) which represents
     *the value as a rational number with the highest accuracy. Therefore,
     *uint64_t (uint64_t) is needed. This rational fraction is then reduced
     *using the Euclidean algorithm to find the greatest common divisor (GCD).
     *   bigNum   = big numinator of value without fraction (or cut residual
     *fraction) bigDenom = big denominator of value
     *-- Break-criteria so that uint64_t cast to "bigNum" introduces no error
     *and bigDenom has no overflow, and stop with enlargement of fraction when
     *the double-value of it reaches an integer number without fractional part.
     */
    bigDenom = 1;
    while ((value != floor(value)) && (value < fMax) && (bigDenom < nMax))
    {
        bigDenom <<= 1;
        value *= 2;
    }
    bigNum = (uint64_t)value;

    /*-- Start Euclidean algorithm to find the greatest common divisor (GCD) --
     */
#define MAX_ITERATIONS 64
    for (i = 0; i < MAX_ITERATIONS; i++)
    {
        uint64_t val;
        /* if bigDenom is not zero, calculate integer part of fraction. */
        if (bigDenom == 0)
        {
            break;
        }
        val = bigNum / bigDenom;

        /* Set bigDenom to reminder of bigNum/bigDenom and bigNum to previous
         * denominator bigDenom. */
        aux = bigNum;
        bigNum = bigDenom;
        bigDenom = aux % bigDenom;

        /* calculate next denominator and check for its given maximum */
        aux = val;
        if (denomSum[1] * val + denomSum[0] >= maxDenom)
        {
            aux = (maxDenom - denomSum[0]) / denomSum[1];
            if (aux * 2 >= val || denomSum[1] >= maxDenom)
                i = (MAX_ITERATIONS +
                     1); /* exit but execute rest of for-loop */
            else
                break;
        }
        /* calculate next numerator to numSum2 and save previous one to numSum0;
         * numSum1 just copy of numSum2. */
        numSum[2] = aux * numSum[1] + numSum[0];
        numSum[0] = numSum[1];
        numSum[1] = numSum[2];
        /* calculate next denominator to denomSum2 and save previous one to
         * denomSum0; denomSum1 just copy of denomSum2. */
        denomSum[2] = aux * denomSum[1] + denomSum[0];
        denomSum[0] = denomSum[1];
        denomSum[1] = denomSum[2];
    }

    /*-- Check and adapt for final variable size and return values; reduces
     * internal accuracy; denominator is kept in ULONG-range with maxDenom -- */
    while (numSum[1] > returnLimit || denomSum[1] > returnLimit)
    {
        numSum[1] = numSum[1] / 2;
        denomSum[1] = denomSum[1] / 2;
    }

    /* return values */
    *ullNum = numSum[1];
    *ullDenom = denomSum[1];

} /*-- ToRationalEuclideanGCD() -------------- */

/**---- DoubleToRational() -----------------------------------------------
* Calculates the rational fractional of a double input value
* for UN-SIGNED rationals,
* using the Euclidean algorithm to find the greatest common divisor (GCD)
------------------------------------------------------------------------*/
static void TIFFDoubleToRational(double value, uint32_t *num, uint32_t *denom)
{
    /*---- UN-SIGNED RATIONAL ---- */
    double dblDiff, dblDiff2;
    uint64_t ullNum, ullDenom, ullNum2, ullDenom2;

    /*-- Check for negative values. If so it is an error. */
    /* Test written that way to catch NaN */
    if (!(value >= 0))
    {
        *num = *denom = 0;
        TIFFErrorExt(0, "TIFFLib: DoubleToRational()",
                     " Negative Value for Unsigned Rational given.");
        return;
    }

    /*-- Check for too big numbers (> ULONG_MAX) -- */
    if (value > 0xFFFFFFFFUL)
    {
        *num = 0xFFFFFFFFU;
        *denom = 0;
        return;
    }
    /*-- Check for easy integer numbers -- */
    if (value == (uint32_t)(value))
    {
        *num = (uint32_t)value;
        *denom = 1;
        return;
    }
    /*-- Check for too small numbers for "unsigned long" type rationals -- */
    if (value < 1.0 / (double)0xFFFFFFFFUL)
    {
        *num = 0;
        *denom = 0xFFFFFFFFU;
        return;
    }

    /*-- There are two approaches using the Euclidean algorithm,
     *   which can accidentally lead to different accuracies just depending on
     * the value. Try both and define which one was better.
     */
    TIFFToRationalEuclideanGCD(value, 0, 0, &ullNum, &ullDenom);
    TIFFToRationalEuclideanGCD(value, 0, 1, &ullNum2, &ullDenom2);
    /*-- Double-Check, that returned values fit into ULONG :*/
    if (ullNum > 0xFFFFFFFFUL || ullDenom > 0xFFFFFFFFUL ||
        ullNum2 > 0xFFFFFFFFUL || ullDenom2 > 0xFFFFFFFFUL)
    {
        TIFFErrorExt(0, "TIFFLib: DoubleToRational()",
                     " Num or Denom exceeds ULONG: val=%14.6f, num=%12" PRIu64
                     ", denom=%12" PRIu64 " | num2=%12" PRIu64
                     ", denom2=%12" PRIu64 "",
                     value, ullNum, ullDenom, ullNum2, ullDenom2);
        assert(0);
    }

    /* Check, which one has higher accuracy and take that. */
    dblDiff = fabs(value - ((double)ullNum / (double)ullDenom));
    dblDiff2 = fabs(value - ((double)ullNum2 / (double)ullDenom2));
    if (dblDiff < dblDiff2)
    {
        *num = (uint32_t)ullNum;
        *denom = (uint32_t)ullDenom;
    }
    else
    {
        *num = (uint32_t)ullNum2;
        *denom = (uint32_t)ullDenom2;
    }
} /*-- DoubleToRational() -------------- */

/**---- DoubleToSrational() -----------------------------------------------
* Calculates the rational fractional of a double input value
* for SIGNED rationals,
* using the Euclidean algorithm to find the greatest common divisor (GCD)
------------------------------------------------------------------------*/
static void TIFFDoubleToSrational(double value, int32_t *num, int32_t *denom)
{
    /*---- SIGNED RATIONAL ----*/
    int neg = 1;
    double dblDiff, dblDiff2;
    uint64_t ullNum, ullDenom, ullNum2, ullDenom2;

    /*-- Check for negative values and use then the positive one for internal
     * calculations, but take the sign into account before returning. */
    if (value < 0)
    {
        neg = -1;
        value = -value;
    }

    /*-- Check for too big numbers (> LONG_MAX) -- */
    if (value > 0x7FFFFFFFL)
    {
        *num = 0x7FFFFFFFL;
        *denom = 0;
        return;
    }
    /*-- Check for easy numbers -- */
    if (value == (int32_t)(value))
    {
        *num = (int32_t)(neg * value);
        *denom = 1;
        return;
    }
    /*-- Check for too small numbers for "long" type rationals -- */
    if (value < 1.0 / (double)0x7FFFFFFFL)
    {
        *num = 0;
        *denom = 0x7FFFFFFFL;
        return;
    }

    /*-- There are two approaches using the Euclidean algorithm,
     *   which can accidentally lead to different accuracies just depending on
     * the value. Try both and define which one was better. Furthermore, set
     * behavior of ToRationalEuclideanGCD() to the range of signed-long.
     */
    TIFFToRationalEuclideanGCD(value, 1, 0, &ullNum, &ullDenom);
    TIFFToRationalEuclideanGCD(value, 1, 1, &ullNum2, &ullDenom2);
    /*-- Double-Check, that returned values fit into LONG :*/
    if (ullNum > 0x7FFFFFFFL || ullDenom > 0x7FFFFFFFL ||
        ullNum2 > 0x7FFFFFFFL || ullDenom2 > 0x7FFFFFFFL)
    {
        TIFFErrorExt(0, "TIFFLib: DoubleToSrational()",
                     " Num or Denom exceeds LONG: val=%14.6f, num=%12" PRIu64
                     ", denom=%12" PRIu64 " | num2=%12" PRIu64
                     ", denom2=%12" PRIu64 "",
                     neg * value, ullNum, ullDenom, ullNum2, ullDenom2);
        assert(0);
    }

    /* Check, which one has higher accuracy and take that. */
    dblDiff = fabs(value - ((double)ullNum / (double)ullDenom));
    dblDiff2 = fabs(value - ((double)ullNum2 / (double)ullDenom2));
    if (dblDiff < dblDiff2)
    {
        *num = (int32_t)(neg * (long)ullNum);
        *denom = (int32_t)ullDenom;
    }
    else
    {
        *num = (int32_t)(neg * (long)ullNum2);
        *denom = (int32_t)ullDenom2;
    }
} /*-- DoubleToSrational() --------------*/
