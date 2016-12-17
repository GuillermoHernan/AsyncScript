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

expectError ("function() {eval = 40;}");
expectError ("function() {this += 41;}");
expectError ("function() {arguments *= 2;}");

expectError ("function() {++arguments}");
expectError ("function() {this++}");
expectError ("function() {this--}");
expectError ("function() {--eval}");

//This code is valid
function test()
{
    var u = -eval;
    var l = ~arguments;
    return !this;
}



result = 1;