/* Copyright (c) 2026 Dreamy Cecil
This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as published by
the Free Software Foundation


This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. */

#ifndef SE_INCL_FIXEDPOINT_H
#define SE_INCL_FIXEDPOINT_H

// Template for fixed-point arithmetic; redesigned to support any integer type (including unsigned)
// and dynamic width of the integer part based on the width of the fractional part
template<typename IntType, int iFrac>
class FixedPoint {
  private:
    typedef FixedPoint<IntType, iFrac> MyType;

  public:
    typedef IntType Type;
    static const int iFracSize = iFrac; // Fraction size in bits
    static const IntType iOne      = IntType(1) << iFrac; // Internal representation of "1.0"
    static const IntType iFracMask = IntType(iOne - 1);   // Mask that covers the fraction bits
    static const IntType iHalf     = IntType(iOne >> 1);  // Internal representation of "0.5"

  public:
    IntType holder;

  public:
    // Default constructor
    __forceinline FixedPoint() {};

    // Copy constructor
    __forceinline FixedPoint(const MyType &x) { holder = x.holder; };

    // Constructors from integers
    __forceinline FixedPoint(SBYTE i) : holder(IntType(i) << iFracSize) {};
    __forceinline FixedPoint(UBYTE i) : holder(IntType(i) << iFracSize) {};
    __forceinline FixedPoint(SWORD i) : holder(IntType(i) << iFracSize) {};
    __forceinline FixedPoint(UWORD i) : holder(IntType(i) << iFracSize) {};
    __forceinline FixedPoint(SLONG i) : holder(IntType(i) << iFracSize) {};
    __forceinline FixedPoint(ULONG i) : holder(IntType(i) << iFracSize) {};
    __forceinline FixedPoint(SQUAD i) : holder(IntType(i) << iFracSize) {};
    __forceinline FixedPoint(UQUAD i) : holder(IntType(i) << iFracSize) {};
  #if SE1_WIN
    __forceinline FixedPoint(int   i) : holder(IntType(i) << iFracSize) {};
    __forceinline FixedPoint(UINT  i) : holder(IntType(i) << iFracSize) {};
  #endif

    // Constructors from floats
    __forceinline FixedPoint(FLOAT  f) : holder(IntType(f * iOne)) {};
    __forceinline FixedPoint(DOUBLE f) : holder(IntType(f * iOne)) {};

    // Explicit holder value constructor
    __forceinline FixedPoint(IntType iNewHolder, bool bFixedPointDummy) : holder(iNewHolder) {};

    // Cast to integer type
    __forceinline operator IntType(void) const { return holder >> iFracSize; };

    // Cast to float
    __forceinline operator FLOAT(void)  const { return FLOAT((DOUBLE)holder / (DOUBLE)iOne); };
    __forceinline operator DOUBLE(void) const { return (DOUBLE)holder / (DOUBLE)iOne; };

  public:
    // Negation
    __forceinline MyType operator-(void) const {
      return MyType(-holder, true);
    };

    // Addition
    __forceinline MyType operator+(MyType x) const {
      return MyType(holder + x.holder, true);
    };

    __forceinline MyType &operator+=(MyType x) {
      holder += x.holder;
      return *this;
    };

    // Subtraction
    __forceinline MyType operator-(MyType x) const {
      return MyType(holder - x.holder, true);
    };

    __forceinline MyType &operator-=(MyType x) {
      holder -= x.holder;
      return *this;
    };

    // Multiplication
    __forceinline MyType operator*(MyType x) const {
      // [Cecil] FIXME: May cut off high bits if the underlying type is also 64-bit
      return MyType(IntType((SQUAD(holder) * x.holder) >> iFracSize), true);
    };

    __forceinline MyType operator*(IntType i) const {
      return MyType(holder * i, true);
    };

    friend __forceinline MyType operator*(IntType i, MyType x) {
      return MyType(x.holder * i, true);
    };

    __forceinline MyType &operator*=(MyType x) {
      return *this = *this * x;
    };

    // Division
    __forceinline MyType operator/(MyType x) const {
      // [Cecil] FIXME: May cut off high bits if the underlying type is also 64-bit
      return MyType(IntType((SQUAD(holder) << iFracSize) / x.holder), true);
    };

    __forceinline MyType &operator/=(MyType x) {
      return *this = *this / x;
    };

    // Comparison operators
    __forceinline bool operator< (MyType x) const { return holder <  x.holder; };
    __forceinline bool operator<=(MyType x) const { return holder <= x.holder; };
    __forceinline bool operator> (MyType x) const { return holder >  x.holder; };
    __forceinline bool operator>=(MyType x) const { return holder >= x.holder; };
    __forceinline bool operator==(MyType x) const { return holder == x.holder; };
    __forceinline bool operator!=(MyType x) const { return holder != x.holder; };

  // Rounding and truncation (returns fixed-point with zeroed fraction)
  public:

    __forceinline MyType Floor(void) const {
      return MyType(holder & ~iFracMask, true);
    };

    __forceinline MyType Ceil(void) const {
      bool bAnyFrac = (holder & iFracMask) != 0;
      return MyType((holder & ~iFracMask) + (bAnyFrac ? iOne : IntType(0)), true);
    };

    __forceinline MyType Round(void) const {
      bool bFracPastHalf = (holder & iFracMask) >= iHalf;
      return MyType((holder & ~iFracMask) + (bFracPastHalf ? iOne : IntType(0)), true);
    };

    // Return fixed-point fraction
    __forceinline MyType GetFraction(void) const {
      return MyType(holder & iFracMask, true);
    };

  // Rounding and truncation (returns underlying integer type shifted by the fraction size)
  public:

    friend __forceinline IntType Floor(MyType x) {
      return x.holder >> iFracSize;
    };

    friend __forceinline IntType Ceil(MyType x) {
      return (x.holder >> iFracSize) + IntType((x.holder & iFracMask) != 0);
    };

    friend __forceinline IntType Round(MyType x) {
      return (x.holder >> iFracSize) + IntType((x.holder & iFracMask) >= iHalf);
    };

    // Retrieve the integer part and the fractional part (either may be NULL if not needed)
    __forceinline void Decompose(IntType *pIntPart, DOUBLE *pFracPart) const {
      if (pIntPart  != NULL) *pIntPart  = holder >> iFracSize;
      if (pFracPart != NULL) *pFracPart = DOUBLE(holder & iFracMask) / (DOUBLE)iOne;
    };
};

// Common types
typedef FixedPoint<SQUAD, 16> sfix16_t;
typedef FixedPoint<UQUAD, 16> ufix16_t;

// [Cecil] TEMP: Old definition for compatibility
typedef FixedPoint<SLONG, 16> FIX16_16;

#endif // include-once check
