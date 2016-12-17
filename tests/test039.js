// Assignment operators test

/*
    Assignment                      x = y 	x = y
    Addition assignment             x += y 	x = x + y
    Subtraction assignment          x -= y 	x = x - y
    Multiplication assignment       x *= y 	x = x * y
    Division assignment             x /= y 	x = x / y
    Remainder assignment            x %= y 	x = x % y
    Exponentiation assignment       x **= y 	x = x ** y
    Left shift assignment           x <<= y 	x = x << y
    Right shift assignment          x >>= y 	x = x >> y
    Unsigned right shift assignment x >>>= y 	x = x >>> y
    Bitwise AND assignment          x &= y 	x = x & y
    Bitwise XOR assignment          x ^= y 	x = x ^ y
    Bitwise OR assignment           x |= y 	x = x | y
 */

var x = 1;

assert (5 == (x+=4), "+= result");
assert (5 == x, "+=");

x -= 7;
assert (-2 == x, "-=");

x *= -12;
assert (24 == x, "*=");

x /= 6;
assert (4 == x, "/=");

x = 10;
x %= 4;
assert (2 == x, "%=");

x **= 8;
assert (256 == x, "**=");

x <<= 2;
assert (1024 == x, "<<=");

x |= 0x1F;
assert (0x41F == x, "|=");

x ^= 0xff;
assert (0x4E0 == x, "^=");

x = -17;
x >>= 1;
assert (-9 == x, ">>=");

x >>>= 22;
assert (0x3FF == x, ">>>=, x=" + x);

x &= 0x1019;
assert (0x19 == x, "&=");

result = 1;
