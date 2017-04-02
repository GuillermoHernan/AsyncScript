// functions in variables
var bob = {};
bob.add = function(x,y) { return x+y; };

const r = bob.add(3,6);

assert (r == 9, "'r' value: " + r);
result = r==9;
