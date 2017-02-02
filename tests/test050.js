// Polynomic function: Test 'call', 'indexedRead', and 'indexedWrite' methods override.

class PolynomicFunction(coefficients)
{
    function call(t)
    {
        var i = 0;
        var result = 0;
        
        printLn("Called with: " + t);
        
        for (c in this.coefficients)
        {
            result += c * t**i;
            ++i;
        }
        
        return result;
    }
}

const f = PolynomicFunction ([8, -2, 1]);

assert (f(0) == 8, "f(0)=" + f(0));
assert (f(-2) == 8, "f(-2)=" + f(-2));
assert (f(1) == 8, "f(1)=" + f(1));
assert (f(4.5) == 8, "f(4.5)=" + f(4.5));

result = 1
