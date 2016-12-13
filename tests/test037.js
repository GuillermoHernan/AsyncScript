// String concatenation tests

var v1 = "aa" + "bb";
var v2 = v1;

var v3 = v2 += "cc";
v2 += "dd";

result = v1 == "aabb" && v2 == "aabbccdd" && v3 == "aabbcc";