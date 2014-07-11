
print("if test1...");
if(true) {
    println("OK");
}
else {
    println("FALSE");
    System.exit(2);
}

print("if test2...");
if(false) {
    println("FALSE");
    System.exit(2);
}
else if(false) {
    println("FALSE");
    System.exit(2);
}
else if(false) {
    println("FALSE");
    System.exit(2);
}
else {
    println("OK");
}

print("if test3...");
if(false) {
    println("FALSE");
    System.exit(2);
}
else {
    println("OK");
}

print("if test4...");
int result = int if(true) { 9999; };
if(result == 9999) {
    println("OK");
}
else {
    println("FALSE");
    System.exit(2);
}

String result2 = String if(false) { "AAA"; } else if(true) { "BBB"; } else { "CCC"; };

print("if test5...");
if(result2 == "BBB") {
    println("OK");
}
else {
    println("FALSE");
    System.exit(2);
}

print("block var test...");

int a = 1;
if(a != 1) {
    println("FALSE1");
    System.exit(2);
}

if(true) {
    int b = 2;

    if(a != 1 || b != 2) {
        println("FALSE2");
println("a ---> " + a.to_str());
println("b ---> " + b.to_str());
        System.exit(2);
    }

    if(true) {
        int c = 3;

        if(a != 1 || b != 2 || c != 3) {
            println("FALSE3");
            System.exit(2);
        }
    }
}

if(a != 1) {
    println("FALSE4");
    System.exit(2);
}

if(true) {
    int a = 9;
    int b = 3;

    if(a != 9 || b != 3) {
        println("FALSE5");
        System.exit(2);
    }

    if(true) {
        int c = 4;
        int d = 5;

        if(a != 9 || b != 3 || c != 4 || d != 5) {
            println("FALSE6");
            System.exit(2);
        }
        
        if(true) {
            int e = 6;

            if(a != 9 || b != 3 || c != 4 || d != 5 || e != 6) {
                println("FALSE7");
                System.exit(2);
            }
        }
    }
}

if(a != 1) {
    println("FALSE8");
    System.exit(2);
}

if(true) {
    a = 222;
}

if(a != 222) {
    println("FALSE9");
    System.exit(2);
}

println("OK");

