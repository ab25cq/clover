String a = Clover.outputToString() {
    int i = 1;
    while(i <= 5) {
        println(i.toString());
        i++;
    }
}

print("while test...");
if(a == "1\n2\n3\n4\n5\n") {
    println("OK");
}
else {
    println("FALSE");
    System.exit(2);
}

String b = Clover.outputToString() {
    int i = 1;
    do {
        println(i.toString());
        i++;
    } while(i <= 5);
}

print("do while test...");
if(b == "1\n2\n3\n4\n5\n") {
    println("OK");
}
else {
    println("FALSE");
    System.exit(2);
}

String d = Clover.outputToString() {
    int i = 1;
    while(i <= 5) { 
        println(i.toString());
        break;
        i++; 
    }
}

print("while test2...");
if(d == "1\n") {
    println("OK");
}
else {
    println("FALSE");
    System.exit(2);
}

String e = Clover.outputToString() {
    int i = 1;
    do {
        println(i.toString());
        break;
        i++;
    } while(i <= 5);
}

print("do while test2...");
if(e == "1\n") {
    println("OK");
}
else {
    println("FALSE");
    System.exit(2);
}

String f = Clover.outputToString() {
    int i = 1;
    while(i <= 5) { 
        println(i.toString());
        if(i == 3) { 
            i += 2; 
            break;
        } else { 
            i++; 
        }
    }
}

print("while test3...");
if(f == "1\n2\n3\n") {
    println("OK");
}
else {
    println("FALSE");
    print("f --> " + f);
    System.exit(2);
}
