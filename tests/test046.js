//const test

const a = 5;
const b = a * 4;

assert (a==5, "a==5");
assert (b==20, "b==20");

expectError ("const c = 22; c *= 2;");

var obj = {
    a: "mutable",
    const b: "not mutable"
}

obj.a = 17;
assert (obj.a == 17, "obj.a == 17");

obj.b = 9;
assert (obj.b == "not mutable", "obj.b not modified");

result = true;
