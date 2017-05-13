// 'for (... in ...)' Test, just with arrays

//const a = [4];
const a = [4,9,11,3];
var i = 0;

for (x in a)
{
    assert (x == a[i], "x=" + x + ", i=" + i);
    ++i;
}

assert (i == a.length, "'i' value: " + i);


result = 1;