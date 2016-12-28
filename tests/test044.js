// Test of object freeze / unfreeze

var a = {x:5};
var b = a.freeze();

a.x = 7;
b.x = 7;

assert (a.x == 7, "a.x == 7");
assert (b.x == 5, "b.x == 5");

var c = b.unfreeze();
var d = a.unfreeze();
var e = a.unfreeze(true);

c.x = 8;
d.x = 2;
e.x = 3;

assert (a.x == 2, "a.x == 2");
assert (b.x == 5, "b.x == 5");
assert (c.x == 8, "c.x == 8");
assert (d.x == 2, "d.x == 2");
assert (e.x == 3, "e.x == 3");

result = true;
