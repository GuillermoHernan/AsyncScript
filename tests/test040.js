// Array index test

var v = [2,5,10,17,19];

assert (v.length == 5, "Initial length check: "+ v.length);
assert (v[2] == 10, "v[3] == " + v[2]);
assert (v[4.0] == 19, "v[4.0] == "+ v[4.0]);
assert (v[1.8] === undefined, "v[1.8] == "+ v[1.8]);

v[1] = 9;
assert (v.length == 5, "expected length(5): "+ v.length);
assert (v[1] == 9, "v[1] == " + v[1]);

v[5] = 11;
assert (v.length == 6, "expected length(6): "+ v.length);
assert (v[5] == 11, "v[5] == " + v[1]);

v[8.0] = 31;
assert (v.length == 9, "expected length(9): "+ v.length);
assert (v[8.0] == 31, "v[8.0] == " + v[8.0]);

v[19.9] = 127;
assert (v.length == 9, "expected length(9): "+ v.length);
assert (v[19.9] == 127, "v[19.9] == " + v[19.9]);

v[6.0] = 29;
v[7] = 30;
assert (v.join() == "2,9,10,17,19,11,29,30,31", "Array content: " + v.join());


result = 1;