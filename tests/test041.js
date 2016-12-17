//Forbidden variable names

//This code is valid
/*function test()
{
    ++this;
    var l = ~arguments;
    return !this;
}

!eval;*/

expectError ("var this = 1");
expectError ("var arguments = 1");
expectError ("var eval = 1");

expectError ("function this(x) {return x}");
expectError ("function arguments(x) {return x}");
expectError ("function eval(x) {return x}");

expectError ("function x(this) {return 1}");
expectError ("function x(arguments) {return 1}");
expectError ("function x(eval) {return 1}");

expectError ("function() {eval = 40;}");
expectError ("function() {this += 41;}");
expectError ("function() {arguments *= 2;}");

expectError ("++arguments");
expectError ("this++");
expectError ("this--");
expectError ("--eval");
expectError ("!this");


result = 1;