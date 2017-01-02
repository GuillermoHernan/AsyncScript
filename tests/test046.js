//const test

const a = 5;
const b = a * 4;

assert (a==5, "a==5");
assert (b==20, "b==20");

expectError ("const c = 22; c *= 2;");

result = true;
