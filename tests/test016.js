// This test was originally intended to test Javascript null / undefined semantics. 
//Now, just test null semantics.
var testUndefined;        // variable declared but not defined, set to value of null.
var testObj = {};  

result = 1;

assert (""+testUndefined == "null", "Not initialized variables are null");
assert (""+testObj.myProp == "null", "Accessing not defined members yields null");

//if (!(undefined == null)) result = 0;  // unenforced type during check, displays true
//if (undefined === null) result = 0;// enforce type during check, displays false
//
//
////if (null != undefined) result = 0;  // unenforced type during check, displays true
//if (null === undefined) result = 0; // enforce type during check, displays false
//
//
