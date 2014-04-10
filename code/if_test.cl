
Clover.print("if test1...");
if(true) {
    Clover.println("OK");
}
else {
    Clover.println("FALSE");
    Clover.exit(2);
}

Clover.print("if test2...");
if(false) {
    Clover.println("FALSE");
    Clover.exit(2);
}
else if(false) {
    Clover.println("FALSE");
    Clover.exit(2);
}
else if(false) {
    Clover.println("FALSE");
    Clover.exit(2);
}
else {
    Clover.println("OK");
}

Clover.print("if test3...");
if(false) {
    Clover.println("FALSE");
    Clover.exit(2);
}
else {
    Clover.println("OK");
}

Clover.print("if test4...");
int result = int if(true) { 9999 };
if(result == 9999) {
    Clover.println("OK");
}
else {
    Clover.println("FALSE");
    Clover.exit(2);
}

String result2 = String if(false) { "AAA" } else if(true) { "BBB" } else { "CCC" }

Clover.print("if test5...");
if(result2 == "BBB") {
    Clover.println("OK");
}
else {
    Clover.println("FALSE");
    Clover.exit(2);
}

Clover.print("block var test...");

int a = 1;
if(a != 1) {
    Clover.println("FALSE");
    Clover.exit(2);
}

if(true) {
    int b = 2;

    if(a != 1 || b != 2) {
        Clover.println("FALSE");
        Clover.exit(2);
    }

    if(true) {
        int c = 3;

        if(a != 1 || b != 2 || c != 3) {
            Clover.println("FALSE");
            Clover.exit(2);
        }
    }
}

if(a != 1) {
    Clover.println("FALSE");
    Clover.exit(2);
}

if(true) {
    int a = 9;
    int b = 3;

    if(a != 9 || b != 3) {
        Clover.println("FALSE");
        Clover.exit(2);
    }

    if(true) {
        int c = 4;
        int d = 5;

        if(a != 9 || b != 3 || c != 4 || d != 5) {
            Clover.println("FALSE");
            Clover.exit(2);
        }
        
        if(true) {
            int e = 6;

            if(a != 9 || b != 3 || c != 4 || d != 5 || e != 6) {
                Clover.println("FALSE");
                Clover.exit(2);
            }
        }
    }
}

if(a != 1) {
    Clover.println("FALSE");
    Clover.exit(2);
}

if(true) {
    a = 222;
}

if(a != 222) {
    Clover.println("FALSE");
    Clover.exit(2);
}

Clover.println("OK");

