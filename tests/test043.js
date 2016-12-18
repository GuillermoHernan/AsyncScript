// Test for actor system parser.

var code = 
    "actor ScriptConsole{\n" + 
    "var reader = IO.StdinLineReader();\n"+
    "lineIn <- reader.lineOut;\n"+
    "input lineIn (cmd)\n"+
    "{\n"+
        "var result = safeEval (line);\n"+
        "if (result.ok)\n"+
            "Console.log ('> ' + result.value);\n"+
        "else\n"+
            "Console.error ('! ' + result.error);\n"+
    "}}\n";

var parseResult = asParse (code);

result = true;
