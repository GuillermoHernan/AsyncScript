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

var f = {a:a, b:b}

f.a.x = 11;
assert (a.x == 11, "a.x == 11");
assert (f.a.x == 11, "f.a.x == 11");

var g = f.freeze();

a.x = 9;
assert (a.x == 9, "a.x == 9");
assert (f.a.x == 9, "f.a.x == 9");
assert (g.a.x == 9, "f.a.x == 9");

var h = f.deepFreeze();

a.x = 14;
assert (a.x == 14, "a.x == 9");
assert (f.a.x == 14, "f.a.x == 9");
assert (g.a.x == 14, "f.a.x == 9");
assert (h.a.x == 9, "f.a.x == 9");
assert (h.isFrozen(), "h.isFrozen");
assert (h.isDeepFrozen(), "h.isDeepFrozen");
assert (h.a.isDeepFrozen(), "h.a.isDeepFrozen");
assert (h.b.isDeepFrozen(), "h.b.isDeepFrozen");


result = true;
