/*
 * Modules test
 */

import './modules/exportTest.js'

const a = multiply (5, 4);
const b = add (9,7);
const c = substract (11, 13);

assert (a==20, "'a' value = " + a);
assert (b==16, "'b' value = " + b);
assert (c==-2, "'c' value = " + c);

result = true;