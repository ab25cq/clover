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

String c = Clover.outputToString() {
    for(int i=1; i<=5; i++) {
        println(i.toString());
    }
}

print("for test...");
if(c == "1\n2\n3\n4\n5\n") {
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

int i = 1;
int g = int while(i <= 5) {
    break 3;
    i++;
}

print("while result test...");
if(g == 3) {
    println("OK");
}
else {
    println("FALSE");
    System.exit(2);
}

i = 1;
int h = int do { 
    break 3;
    i++;
} while(i<=5);

print("do while result test...");
if(h == 3) {
    println("OK");
}
else {
    println("FALSE");
    System.exit(2);
}

i = 1;
int k = int while(i <= 5) {
    i++;

    if(i == 6) {
        break 1 + 1;
    }
}

print("do while test...");
if(k == 2) {
    println("OK");
}
else {
    println("FALSE");
    System.exit(2);
}

int l = int for(int j=0; j<5; j++) {
    if(j == 4) {
        break 555;
    }
    else {
        break 444;
    }
}

print("for result test...");
if(l == 444) {
    println("OK");
}
else {
    println("FALSE");
    System.exit(2);
}

