// Test for actor system parser.

var code = 
    "actor ScriptConsole(inStream, outStream, errStream){\n" + 
    "var reader = IO.LineReader(inStream);\n"+
    "var outWriter = IO.TextWriter(outStream);\n"+
    "var errWriter = IO.TextWriter(errStream);\n"+
    "lineIn <- reader.lineOut;\n"+
    "input lineIn (cmd)\n"+
    "{\n"+
        "var result = safeEval (line);\n"+
        "if (result.ok)\n"+
            "outWriter.textIn ('> ' + result.value);\n"+
        "else\n"+
            "errWriter.textIn ('! ' + result.error);\n"+
    "}}\n";

var parseResult = asParse (code);

result = true;
