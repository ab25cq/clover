int a = 0;

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

