// test for array slice
var a = [1,2,4,5,7];

var b = a.slice(2);
var c = a.slice(1,3);
var d = a.slice(1, -3);


assert (b.join() == "4,5,7", "wrong 'b' array: " + b.join());
assert (c.join() == "2,4", "wrong 'c' array: " + c.join());
assert (d.join() == "2,4,5,7", "wrong 'd' array: " + d.join());

result = 1;
