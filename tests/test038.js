// Increment operator tests

var v1 = 1;
var v2 = ++v1;
var v3 = v1++;
var v4 = v1--;
var v5 = --v3;

result = v1 == 2 && v2 == 2 && v3 == 1 && v4 ==3 && v5==v3;