/*
   Copyright 2021 Bruce Dawson

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

/*
CompareAsInt.cpp - various functions to compare floating point numbers using
integer operations, other functions to test these comparisons, and other
functions to print out floating point numbers and their representation.
*/

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>

// Non-optimal AlmostEqual function - not recommended.
bool AlmostEqualRelative(float A, float B, float maxRelativeError) {
  if (A == B)
    return true;
  float relativeError = fabsf((A - B) / B);
  if (relativeError <= maxRelativeError)
    return true;
  return false;
}

// Slightly better AlmostEqual function – still not recommended
bool AlmostEqualRelative2(float A, float B, float maxRelativeError) {
  if (A == B)
    return true;
  float relativeError;
  if (fabsf(B) > fabsf(A))
    relativeError = fabsf((A - B) / B);
  else
    relativeError = fabsf((A - B) / A);
  if (relativeError <= maxRelativeError)
    return true;
  return false;
}

// Slightly better AlmostEqual function – still not recommended
bool AlmostEqualRelativeOrAbsolute(float A, float B, float maxRelativeError,
                                   float maxAbsoluteError) {
  if (fabsf(A - B) < maxAbsoluteError)
    return true;
  float relativeError;
  if (fabsf(B) > fabsf(A))
    relativeError = fabsf((A - B) / B);
  else
    relativeError = fabsf((A - B) / A);
  if (relativeError <= maxRelativeError)
    return true;
  return false;
}

// Initial AlmostEqualULPs version - fast and simple, but
// some limitations.
bool AlmostEqualUlps(float A, float B, int maxUlps) {
  if (A == B)
    return true;
  int intDiff = abs(*(int *)&A - *(int *)&B);
  if (intDiff <= maxUlps)
    return true;
  return false;
}

// Usable AlmostEqual function
bool AlmostEqual2sComplement(float A, float B, int maxUlps) {
  // Make sure maxUlps is non-negative and small enough that the
  // default NAN won't compare as equal to anything.
  // This check disabled for now so that my tests run properly.
  // This assert should be enabled for normal use.
  // assert(maxUlps > 0 && maxUlps < 4 * 1024 * 1024);
  int aInt = *(int *)&A;
  // Make aInt lexicographically ordered as a twos-complement int
  if (aInt < 0)
    aInt = 0x80000000 - aInt;
  // Make bInt lexicographically ordered as a twos-complement int
  int bInt = *(int *)&B;
  if (bInt < 0)
    bInt = 0x80000000 - bInt;
  int intDiff = abs(aInt - bInt);
  if (intDiff <= maxUlps)
    return true;
  return false;
}

// Support functions and conditional compilation directives for the
// master AlmostEqual function.
#define INFINITYCHECK
#define NANCHECK
#define SIGNCHECK

inline bool IsInfinite(float A) {
  const int kInfAsInt = 0x7F800000;

  // An infinity has an exponent of 255 (shift left 23 positions) and
  // a zero mantissa. There are two infinities - positive and negative.
  if ((*(int *)&A & 0x7FFFFFFF) == kInfAsInt)
    return true;
  return false;
}

inline bool IsNan(float A) {
  // A NAN has an exponent of 255 (shifted left 23 positions) and
  // a non-zero mantissa.
  int exp = *(int *)&A & 0x7F800000;
  int mantissa = *(int *)&A & 0x007FFFFF;
  if (exp == 0x7F800000 && mantissa != 0)
    return true;
  return false;
}

inline int Sign(float A) {
  // The sign bit of a number is the high bit.
  return (*(int *)&A) & 0x80000000;
}

// This is the 'final' version of the AlmostEqualUlps function.
// The optional checks are included for completeness, but in many
// cases they are not necessary, or even not desirable.
bool AlmostEqualUlpsFinal(float A, float B, int maxUlps) {
  // There are several optional checks that you can do, depending
  // on what behavior you want from your floating point comparisons.
  // These checks should not be necessary and they are included
  // mainly for completeness.

#ifdef INFINITYCHECK
  // If A or B are infinity (positive or negative) then
  // only return true if they are exactly equal to each other -
  // that is, if they are both infinities of the same sign.
  // This check is only needed if you will be generating
  // infinities and you don't want them 'close' to numbers
  // near FLT_MAX.
  if (IsInfinite(A) || IsInfinite(B))
    return A == B;
#endif

#ifdef NANCHECK
  // If A or B are a NAN, return false. NANs are equal to nothing,
  // not even themselves.
  // This check is only needed if you will be generating NANs
  // and you use a maxUlps greater than 4 million or you want to
  // ensure that a NAN does not equal itself.
  if (IsNan(A) || IsNan(B))
    return false;
#endif

#ifdef SIGNCHECK
  // After adjusting floats so their representations are lexicographically
  // ordered as twos-complement integers a very small positive number
  // will compare as 'close' to a very small negative number. If this is
  // not desireable, and if you are on a platform that supports
  // subnormals (which is the only place the problem can show up) then
  // you need this check.
  // The check for A == B is because zero and negative zero have different
  // signs but are equal to each other.
  if (Sign(A) != Sign(B))
    return A == B;
#endif

  int aInt = *(int *)&A;
  // Make aInt lexicographically ordered as a twos-complement int
  if (aInt < 0)
    aInt = 0x80000000 - aInt;
  // Make bInt lexicographically ordered as a twos-complement int
  int bInt = *(int *)&B;
  if (bInt < 0)
    bInt = 0x80000000 - bInt;

  // Now we can compare aInt and bInt to find out how far apart A and B
  // are.
  int intDiff = abs(aInt - bInt);
  if (intDiff <= maxUlps)
    return true;
  return false;
}

// Function to test the TestCompare2sComplement function
void TestCompare2sComplement(float A, float B, bool expectedResult,
                             int maxUlps = 10) {
  bool result = AlmostEqual2sComplement(A, B, maxUlps);
  if (result != expectedResult)
    printf("Unexpected result - %1.9f, %1.9f, %d, expected %s\n", A, B, maxUlps,
           expectedResult ? "true" : "false");
}

// Function to test the TestCompareFinal function
void TestCompareFinal(float A, float B, bool expectedResult, int maxUlps = 10) {
  bool result = AlmostEqualUlpsFinal(A, B, maxUlps);
  if (result != expectedResult)
    printf("Unexpected result final - %1.9f, %1.9f, %d, expected %s\n", A, B,
           maxUlps, expectedResult ? "true" : "false");
}

// Function to test the TestCompareFinal amd TestCompare2sComplement functions
void TestCompareAll(float A, float B, bool expectedResult, int maxUlps = 10) {
  TestCompare2sComplement(A, B, expectedResult, maxUlps);
  TestCompareFinal(A, B, expectedResult, maxUlps);
}

// Function to print a number and its representation, in hex and decimal
void PrintNumber(float f, int offset) {
  (*((int *)&f)) += offset;
  printf("%+1.11g,0x%08X,%d\n", f, *(int *)&f, *(int *)&f);
}

/*
Typical print results, adjusted to have tabs instead of commas for easy
pasting into Excel:

+1.99999976	0x3FFFFFFE	1073741822
+1.99999988	0x3FFFFFFF	1073741823
+2.00000000	0x40000000	1073741824
+2.00000024	0x40000001	1073741825
+2.00000048	0x40000002	1073741826

+0.00000000	0x00000000	0
+0.00000000	0x80000000	-2147483648

+4.2038954e-045	0x00000003	3
+2.8025969e-045	0x00000002	2
+1.4012985e-045	0x00000001	1
+0	0x00000000	0
-0	0x80000000	-2147483648
-1.4012985e-045	0x80000001	-2147483647
-2.8025969e-045	0x80000002	-2147483646
-4.2038954e-045	0x80000003	-2147483645
*/

void PrintNumbers() {
  // This function just prints some numbers, as floats, hex int, and decimal int
  // for incorporation into my article on comparing floats as ints.

  float f = 2.0;
  for (int i = -2; i <= 2; ++i) {
    PrintNumber(f, i);
  }
  printf("\n");
  PrintNumber(0.0f, 0);
  PrintNumber(0.0f, 0x80000000);

  printf("\n");
  for (int i = 3; i >= 0; --i)
    PrintNumber(0.0f, i);
  for (int i = 0; i <= 3; ++i)
    PrintNumber(0.0f, i + 0x80000000);

  printf("\n");
  PrintNumber(10000.0f, 1);
}

float zero1, zero2;

int main(int argc, char *argv[]) {
  PrintNumbers();

  // Create various special numbers
  float negativeZero;
  // Initialize negativeZero with its integer representation
  *(int *)&negativeZero = 0x80000000;
  // Create a NAN
  float nan1 = sqrtf(-1.0f);
  // Create a NAN a different way - should give the same NAN on
  // Intel chips.
  float nan2 = zero1 / zero2;
  // Create an infinity
  float inf = 1 / zero1;
  // Create a NAN a third way - should give the same NAN on
  // Intel chips.
  float nan3 = inf - inf;
  // Copy one of the NANs and modify its representation.
  // This will still give a NAN, just a different one.
  float nan4 = nan3;
  (*(int *)&nan4) += 1;

  // Create a denormal by starting with zero and incrementing
  // the integer representation.
  float smallestDenormal = 0;
  (*(int *)&smallestDenormal) += 1;

  // The first set of tests check things that any self-respecting
  // comparison function should agree upon.

  // Make sure that zero and negativeZero compare as equal.
  TestCompareAll(zero1, negativeZero, true);

  // Make sure that nearby numbers compare as equal.
  TestCompareAll(2.0, 1.9999999f, true);

  // Make sure that slightly more distant numbers compare as equal.
  TestCompareAll(2.0, 1.9999995f, true);

  // Make sure the results are the same with parameters reversed.
  TestCompareAll(1.9999995f, 2.0, true);

  // Make sure that even more distant numbers don't compare as equal.
  TestCompareAll(2.0, 1.9999995f, true);

  // The next set of tests check things where the correct answer isn't
  // as obvious or important. Some of these tests check for cases that
  // are rare or can easily be avoided. Some of them check for cases
  // where the behavior of the 2sComplement function is arguably better
  // than the behavior of the fussier Final function.

  // Test wrapping from inf to -inf when maxUlps is too large.
  TestCompare2sComplement(inf, -inf, true, 16 * 1024 * 1024);
  TestCompareFinal(inf, -inf, false, 16 * 1024 * 1024);

  // Test whether FLT_MAX and infinity (representationally adjacent)
  // compare as equal.
  TestCompare2sComplement(FLT_MAX, inf, true);
  TestCompareFinal(FLT_MAX, inf, false);

  // Test whether a NAN compares as equal to itself.
  TestCompare2sComplement(nan2, nan2, true);
  TestCompareFinal(nan2, nan2, false);

  // Test whether a NAN compares as equal to a different NAN.
  TestCompare2sComplement(nan2, nan3, true);
  TestCompareFinal(nan2, nan3, false);

  // Test whether tiny numbers of opposite signs compare as equal.
  TestCompare2sComplement(smallestDenormal, -smallestDenormal, true);
  TestCompareFinal(smallestDenormal, -smallestDenormal, false);

  return 0;
}

// Code for copying/pasting into the article
void CopyPasteCode(float result, float expectedResult, int f1, int f2) {
  if (result == expectedResult)
    return;
  if (fabsf(result - expectedResult) < 0.00001f)
    return;
  float relativeError = fabsf((result - expectedResult) / expectedResult);
  if (*(int *)&f1 < *(int *)&f2)
    return;
  (*(int *)&f1) += 1;
}
