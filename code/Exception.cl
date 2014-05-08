
int a = 123;
try {
    int b = 234;

    print("try variable test1...");
    if(a == 123) {
        println("TRUE");
    }
    else {
        println("FALSE");
        Clover.exit(2);
    }

    print("try variable test2...");
    if(b == 234) {
        println("TRUE");
    }
    else {
        println("FALSE");
        Clover.exit(2);
    }

    println("try");
    throw new Exception("Normal Exception");
}
catch(Exception e) {
    int b = 777;

    print("catch variable test1...");
    if(a == 123) {
        println("TRUE");
    }
    else {
        println("FALSE");
        Clover.exit(2);
    }

    print("catch variable test2...");
    if(b == 777) {
        println("TRUE");
    }
    else {
        println("FALSE");
        Clover.exit(2);
    }

    print("catching exception object test...");
    if(e.class_name() == "Exception") {
        println("TRUE");
    }
    else {
        println("e.class_name --> " + e.class_name());
        println("FALSE");
        Clover.exit(2);
    }
}
finally {
    int b = 888;

    print("finally variable test1...");
    if(a == 123) {
        println("TRUE");
    }
    else {
        println("FALSE");
        Clover.exit(2);
    }

    print("finally variable test2...");
    if(b == 888) {
        println("TRUE");
    }
    else {
        println("FALSE");
        Clover.exit(2);
    }
    println("finally");
}

println("end");

int c = 2;

print("variable test...");
if(c == 2) {
    println("OK");
}
else {
    println("FALSE");
    Clover.exit(2);
}

//throw new Exception("Test");

ExceptionTest test = new ExceptionTest();

try {
    test.method1();
} catch(Exception e) {
    print("catch Exception Test...");
    if(e.class_name() == "Exception") {
        println("TRUE");
    }
    else {
        println("FALSE");
        Clover.exit(2);
    }
} finally {
    println("finally");
}

try {
    throw new Exception("AAA");
} catch(Exception e) {
    print("catch Exception Test2...");
    if(e.class_name() == "Exception") {
        println("TRUE");
    }
    else {
        println("FALSE");
        Clover.exit(2);
    }
}
finally {
    println("finally");
}

