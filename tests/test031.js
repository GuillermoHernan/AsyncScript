// test for string split
var b = "1,4,7";
var a = b.split(",");

assert (a.length == 3, "a.length = " + a.length);
assert (a[0]==1, "a[0]=" + a[0]); 
assert (a[1]==4, "a[1]=" + a[1]); 
assert (a[2]==7, "a[2]=" + a[2]);

result = 1;
