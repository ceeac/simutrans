/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DATAOBJ_MONEY_H
#define DATAOBJ_MONEY_H


#include "../simtypes.h"


class loadsave_t;


/**
 * Class for monetary values.
 */
class money_t
{
public:
	explicit money_t(sint64 amount = 0);
	explicit money_t(sint64 credits, sint64 cents);

	sint64 get_value() const { return value; }
	sint64 get_credits() const { return value / sint64(100); }

	inline double as_double() const { return (double)value / 100.0; }

public:
	bool operator< (money_t rhs) const { return value <  rhs.value; }
	bool operator<=(money_t rhs) const { return value <= rhs.value; }
	bool operator==(money_t rhs) const { return value == rhs.value; }
	bool operator!=(money_t rhs) const { return value != rhs.value; }
	bool operator>=(money_t rhs) const { return value >= rhs.value; }
	bool operator> (money_t rhs) const { return value >  rhs.value; }

	money_t operator+=(money_t rhs) { value += rhs.value;   return *this; }
	money_t operator-=(money_t rhs) { value -= rhs.value;   return *this; }
	money_t operator*=(sint64  rhs) { value *= rhs;         return *this; }
	money_t operator*=(uint64  rhs) { value *= rhs;         return *this; }
	money_t operator*=(sint32  rhs) { value *= sint64(rhs); return *this; }
	money_t operator*=(uint32  rhs) { value *= sint64(rhs); return *this; }
	money_t operator/=(sint32  rhs) { value /= sint64(rhs); return *this; }
	money_t operator/=(uint32  rhs) { value /= sint64(rhs); return *this; }

	money_t operator-() const { return money_t(-value); }
	money_t operator+() const { return *this; }

#ifndef MAKEOBJ
public:
	void rdwr(loadsave_t *file);
#endif

private:
	template<typename T> operator T() const { return value; } // do not call from outside!

	sint64 value;
};


static inline money_t operator+(money_t lhs, money_t rhs) { return money_t(lhs) += rhs; }
static inline money_t operator-(money_t lhs, money_t rhs) { return money_t(lhs) -= rhs; }

template<typename T> static inline money_t operator*(money_t lhs, T rhs) { return lhs *= rhs; }
template<typename T> static inline money_t operator*(T lhs, money_t rhs) { return rhs *= lhs; }
template<typename T> static inline money_t operator/(money_t lhs, T rhs) { return lhs /= rhs; }


#define PRICE_MAGIC money_t(0x7FFFFFFF)


#endif
