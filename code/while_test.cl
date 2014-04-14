String a = Clover.output_to_s() {
    int i = 1;
    while(i <= 5) {
        Clover.println(i.to_s());
        i++;
    }
}

Clover.print("while test...");
if(a == "1\n2\n3\n4\n5\n") {
    Clover.println("OK");
}
else {
    Clover.println("FALSE");
    Clover.exit(2);
}

String b = Clover.output_to_s() {
    int i = 1;
    do {
        Clover.println(i.to_s());
        i++;
    } while(i <= 5);
}

Clover.print("do while test...");
if(b == "1\n2\n3\n4\n5\n") {
    Clover.println("OK");
}
else {
    Clover.println("FALSE");
    Clover.exit(2);
}

String c = Clover.output_to_s() {
    for(int i=1; i<=5; i++) {
        Clover.println(i.to_s());
    }
}

Clover.print("for test...");
if(c == "1\n2\n3\n4\n5\n") {
    Clover.println("OK");
}
else {
    Clover.println("FALSE");
    Clover.exit(2);
}

String d = Clover.output_to_s() {
    int i = 1;
    while(i <= 5) { 
        Clover.println(i.to_s());
        break;
        i++; 
    }
}

Clover.print("while test2...");
if(d == "1\n") {
    Clover.println("OK");
}
else {
    Clover.println("FALSE");
    Clover.exit(2);
}

String e = Clover.output_to_s() {
    int i = 1;
    do {
        Clover.println(i.to_s());
        break;
        i++;
    } while(i <= 5);
}

Clover.print("do while test2...");
if(e == "1\n") {
    Clover.println("OK");
}
else {
    Clover.println("FALSE");
    Clover.exit(2);
}

String f = Clover.output_to_s() {
    int i = 1;
    while(i <= 5) { 
        Clover.println(i.to_s());
        if(i == 3) { 
            i += 2; 
            break;
        } else { 
            i++; 
        }
    }
}

Clover.print("while test3...");
if(f == "1\n2\n3\n") {
    Clover.println("OK");
}
else {
    Clover.println("FALSE");
    Clover.print("f --> " + f);
    Clover.exit(2);
}

int i = 1;
int g = int while(i <= 5) {
    break 3;
    i++;
}

Clover.print("while result test...");
if(g == 3) {
    Clover.println("OK");
}
else {
    Clover.println("FALSE");
    Clover.exit(2);
}

i = 1;
int h = int do { 
    break 3;
    i++;
} while(i<=5);

Clover.print("do while result test...");
if(h == 3) {
    Clover.println("OK");
}
else {
    Clover.println("FALSE");
    Clover.exit(2);
}

i = 1;
int k = int while(i <= 5) {
    i++;

    if(i == 6) {
        break 1 + 1;
    }
}

Clover.print("do while test...");
if(k == 2) {
    Clover.println("OK");
}
else {
    Clover.println("FALSE");
    Clover.exit(2);
}

int l = int for(int j=0; j<5; j++) {
    if(j == 4) {
        break 555;
    }
    else {
        break 444;
    }
}

Clover.print("for result test...");
if(l == 444) {
    Clover.println("OK");
}
else {
    Clover.println("FALSE");
    Clover.exit(2);
}

