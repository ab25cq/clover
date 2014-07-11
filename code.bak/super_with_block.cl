
Extended1 a = new Extended1(111, 222);

String rvalue = Clover.output_to_str() {
    a.show() {
        Clover.println("this is called with block");
    }
}

print("super with block test...");
if(rvalue == "Extended1::show\nBase::show\nself.value1 --> 111\nself.value2 --> 222\nthis is called with block\n")
{
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(1);
}

rvalue = Clover.output_to_str() {
    a.show2();
}

print("super with block test2...");
if(rvalue == "Extended1::show2\nBase::show2\nself.value1 --> 111\nself.value2 --> 222\ncalling super with block\nblock returns 1\n")
{
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(1);
}

rvalue = Clover.output_to_str() {
    Extended1.show() {
        Clover.println("calling class method with block");
    }
}

print("super with block test3...");
if(rvalue == "Extended1::show() static\nBase::show() static\nblock called\n")
{
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(1);
}
