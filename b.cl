
MethodBlockTestClass a = new MethodBlockTestClass(123);
int b = 345;

a.method() {|int n|
    n.toString().println();
    b.toString().println();
    caller.field.toString().println();
}
