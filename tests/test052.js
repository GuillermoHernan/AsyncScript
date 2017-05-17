/*
 * Modules test
 */

import './modules/exportTest.js'

const a = multiply (5, 4);
const b = add (9,7);
const c = substract (11, 13);

assert (a==9, "'a' value = " + a);
assert (a==16, "'b' value = " + b);
assert (c==-2, "'c' value = " + c);

result = true;