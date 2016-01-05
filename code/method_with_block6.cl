
MethodBlockTestClass a = new MethodBlockTestClass(123);
int b = 345;
int c = 0;

a.method() {|int n|
    c = caller.field;
}

print("Method block caller test...");
Clover.assert(c == 123);
println("TRUE");
