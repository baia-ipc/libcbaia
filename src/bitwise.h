/* (c) 2017-2026 Giorgio Gonnella */

#ifndef BITWISE_H
#define BITWISE_H

/* Generic single bit setter/getter; the bit number can be up to the
   width of the type of E minus 1 */
#define GET_BIT(E, BITNUM) (((E) >> (BITNUM)) & 1)
#define SET_BIT(E, BITNUM) ((E) |= ((__typeof__(E))1 << (BITNUM)))
#define UNSET_BIT(E, BITNUM) ((E) &= ~((__typeof__(E))1 << (BITNUM)))

#endif
