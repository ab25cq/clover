
if(true) {
    Clover.println("TRUE1");
    Clover.println("TRUE1");
}
else {
    Clover.println("FALSE1");
    Clover.println("FALSE1");
}

Clover.println("END");

if(false) {
    Clover.println("TRUE1");
    Clover.println("TRUE1");
}
else if(false) {
    Clover.println("TRUE2");
    Clover.println("TRUE2");
}
else if(false) {
    Clover.println("TRUE3");
    Clover.println("TRUE3");
}

Clover.println("END");

if(true) {
    Clover.println("TRUE1");
    Clover.println("TRUE1");
}
else {
    Clover.println("FALSE");
    Clover.println("FALSE");
}

Clover.println("END");

if(false) {
    Clover.println("TRUE1");
    Clover.println("TRUE1");
}
else {
    Clover.println("FALSE");
    Clover.println("FALSE");
}

Clover.println("END");

Clover.println(int if(true) { 9999 }.to_s());
Clover.println(String if(false) { "AAA" } else if(true) { "BBB" } else { "CCC" });
Clover.println("END");

Clover.println("block var test");

int a = 1;
Clover.println("a --> " + a.to_s());

if(true) {
    int b = 2;

    Clover.println("a --> " + a.to_s() + " b --> " + b.to_s());

    if(true) {
        int c = 3;

        Clover.println("a --> " + a.to_s() + " b --> " + b.to_s() + " c --> " + c.to_s());
    }
}

Clover.println("a --> " + a.to_s());

if(true) {
    int b = 3;

    Clover.println("a --> " + a.to_s() + " b --> " + b.to_s());

    if(true) {
        int c = 4;
        int d = 5;

        Clover.println("a --> " + a.to_s() + " b --> " + b.to_s() + " c --> " + c.to_s() + " d --> " + d.to_s());
        
        if(true) {
            int e = 6;

            Clover.println("a --> " + a.to_s() + " b --> " + b.to_s() + " c --> " + c.to_s() + " d --> " + d.to_s() + " e --> " + e.to_s());
        }
    }
}


