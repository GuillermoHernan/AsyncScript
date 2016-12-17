//Forbidden variable names

expectError ("var this = 1");
expectError ("var arguments = 1");
expectError ("var eval = 1");

expectError ("function this(x) {return x}");
expectError ("function arguments(x) {return x}");
expectError ("function eval(x) {return x}");

expectError ("function x(this) {return 1}");
expectError ("function x(arguments) {return 1}");
expectError ("function x(eval) {return 1}");

expectError ("eval = 40;");
expectError ("this += 41;");
expectError ("arguments *= 2;");

expectError ("++arguments");
expectError ("this++");
expectError ("this--");
expectError ("--eval");

result = 1;