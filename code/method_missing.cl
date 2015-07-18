MethodMissingTest a = null;

print("MethodMissingTest test1...");
if(Clover.outputToString() {
    a = new MethodMissingTest();
} == "HELLO METHOD MISSING\nmethod_name --> _constructor\nparams --> {}\nmethod_block --> null\n")
{
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("MethodMissingTest test2...");
if(Clover.outputToString() {
    a.ggg(111, 222, 333) {|String a, int b|
        return 123;
    }
} == "HELLO METHOD MISSING\nmethod_name --> ggg\nparams --> {111,222,333}\nmethod_block --> Tuple$2<bool,int> Block({String,int})\n")
{
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("MethodMissingTest test3...");

if(Clover.outputToString() {
    MethodMissingTest.kkk("aaa", 111, "ccc") { |Array<int> c, int g|
        return "AAA";
    }
} == "HELLO METHOD MISSING\nmethod_name --> kkk\nparams --> {aaa,111,ccc}\nmethod_block --> Tuple$2<bool,String> Block({Array$1<int>,int})\n")
{
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

