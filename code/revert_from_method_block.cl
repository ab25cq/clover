
RevertTest test = new RevertTest(111);

test.method() (int) { |int n|
    Clover.println("n --> " + n.to_s());
    revert 222;
}

int a = test.method3();

Clover.println("test.method3() returns " + a.to_s());

if(a == 999) {
    Clover.println("OK");
}
else {
    Clover.println("False");
    System.exit(1);
}

