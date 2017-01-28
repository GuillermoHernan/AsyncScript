// test for array contains
var a = [1,2,4,5,7];
var b = ["bread","cheese","sandwich"];

assert (a.indexOf(1) == 0, "index of 1: " + a.indexOf(1));
assert (a.indexOf(5) == 3, "index of 1: " + a.indexOf(5));
assert (a.indexOf(42) < 0, "index of 1: " + a.indexOf(42));
assert (b.indexOf("cheese") == 1, "index of 'cheese': " + b.indexOf("cheese"));
assert (b.indexOf("sandwich") == 2, "index of 'sandwich': " + b.indexOf("sandwich"));
assert (b.indexOf("eggs") < 0, "index of 'eggs': " + b.indexOf("eggs"));

result = 1;
