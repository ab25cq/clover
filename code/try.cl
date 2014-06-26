int a = 1;

for(int i=0; i<10; i++) {
    try {
        a = 2;
        return;
        a = 3;
    }
    catch(Exception e) {
        a = 3;
    }
}

print("try test1...");
if(a == 2) {
    println("OK");
}
else {
    println("FALSE");
    System.exit(2);
}

for(int i=0; i<10; i++) {
    try {
        throw new Exception("AAA");
    }
    catch(Exception e) {
        a = 3;
        return;
        a = 5;
    }
}

print("try test2...");
if(a == 3) {
    println("OK");
}
else {
    println("FALSE");
    System.exit(2);
}

print("try test3...");
TryTest b = new TryTest();
if(b.method() == 1) {
    println("OK");
}
else {
    println("FALSE");
    System.exit(2);
}

print("try test4...");
TryTest c = new TryTest();
if(c.method2() == 111) {
    println("OK");
}
else {
    println(c.method2().to_str());
    println("FALSE");
    System.exit(2);
}
