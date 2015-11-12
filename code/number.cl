void {
    byte a = 3y;
    byte b = 1y;

    print("byte test1...");
    Clover.assert(a + b == 4y && a - b == 2y && a * b == 3y && a / b == 3y && a % 2y == 1y && 1y << 1y == 2y && 2y >> 1y == 1y && (0xFFy & 0x01y) == 0x01y && ~0x01y == 0xFEy && (0x01y|0x02y) == 0x03y && (0xFFy^0xFFy) == 0x00y);
    println("TRUE");

    print("byte test1-2...");
    Clover.assert(bool { a++; a == 4y } && bool { b++; b == 2y } && bool { a--; a == 3y } && bool { b--; b == 1y } && bool { a-=2y; a == 1y } && bool { b-=1y; b == 0y } && bool { a*=2y; a == 2y } && bool { a/=2y; a == 1y } && bool { a%=2y; a == 1y } && bool { a<<=3y; a == 1y << 3y } && bool { a>>=3y; a == 1y });
    println("TRUE");
}

void {
    byte a = 3.toByte();
    byte b = 1.toByte();
    byte c = 2.toByte();

    print("byte test3...");
    Clover.assert(a + b == 4y && a - b == 2y && a * b == 3y && a / b == 3y && a % 2y == 1y && b << 1y == 2y && c >> 1y == 1y && (0xFF.toByte() & 0x01y) == 0x01y && ~0x01.toByte() == 0xFEy && (0x01y|0x02y) == 0x03y && (0xFFy^0xFFy) == 0x00y);
    println("TRUE");
}

void {
    short a = 3s;
    short b = 1s;

    print("short test1...");
    Clover.assert(a + b == 4s && a - b == 2s && a * b == 3s && a / b == 3s && a % 2s == 1s && 1s << 1s == 2s && 2s >> 1s == 1s && (0xFFs & 0x01s) == 0x01s && ~0x01s == 0xFFFEs && (0x01s|0x02s) == 0x03s && (0xFFs^0xFFs) == 0x00s);
    println("TRUE");

    print("short test1-2...");
    Clover.assert(bool { a++; a == 4s } && bool { b++; b == 2s } && bool { a--; a == 3s } && bool { b--; b == 1s } && bool { a-=2s; a == 1s } && bool { b-=1s; b == 0s } && bool { a*=2s; a == 2s } && bool { a/=2s; a == 1s } && bool { a%=2s; a == 1s } && bool { a<<=3s; a == 1s << 3s } && bool { a>>=3s; a == 1s });
    println("TRUE");
}

void {
    uint a = 3u;
    uint b = 1u;

    print("uint test1...");
    Clover.assert(a + b == 4u && a - b == 2u && a * b == 3u && a / b == 3u && a % 2u == 1u && 1u << 1u == 2u && 2u >> 1u == 1u && (0xFFu & 0x01u) == 0x01u && ~0x01u == 0xFFFFFFFEu && (0x01u|0x02u) == 0x03u && (0xFFu^0xFFu) == 0x00u);
    println("TRUE");

    print("uint test1-2...");
    Clover.assert(bool { a++; a == 4u } && bool { b++; b == 2u } && bool { a--; a == 3u } && bool { b--; b == 1u } && bool { a-=2u; a == 1u } && bool { b-=1u; b == 0u } && bool { a*=2u; a == 2u } && bool { a/=2u; a == 1u } && bool { a%=2u; a == 1u } && bool { a<<=3u; a == 1u << 3u } && bool { a>>=3u; a == 1u });
    println("TRUE");
}

void {
    long a = 3l;
    long b = 1l;

    print("long test1...");
    Clover.assert(a + b == 4l && a - b == 2l && a * b == 3l && a / b == 3l && a % 2l == 1l && 1l << 1l == 2l && 2l >> 1l == 1l && (0xFFl & 0x01l) == 0x01l && ~0x01l == 0xFFFFFFFFFFFFFFFEl && (0x01l|0x02l) == 0x03l && (0xFFl^0xFFl) == 0x00l);
    println("TRUE");

    print("long test1-2...");
    Clover.assert(bool { a++; a == 4l } && bool { b++; b == 2l } && bool { a--; a == 3l } && bool { b--; b == 1l } && bool { a-=2l; a == 1l } && bool { b-=1l; b == 0l } && bool { a*=2l; a == 2l } && bool { a/=2l; a == 1l } && bool { a%=2l; a == 1l } && bool { a<<=3l; a == 1l << 3l } && bool { a>>=3l; a == 1l });
    println("TRUE");
}


byte c = 0xFFy;

print("byte test2...");
Clover.assert(c == 255y);
println("TRUE");

byte d = 020.toByte();

print("byte test3...");
Clover.assert(d == 16.toByte());
println("TRUE");

int e = -2;

print("int test1...");
Clover.assert(e.toString() == "-2" && e == -2);
println("TRUE");

byte f = -1y;

print("byte test4...");
Clover.assert(f.toString() == "255" && f == 255y);
println("TRUE");

short g = -1s;

print("short test1...");
Clover.assert(g.toString() == "65535" && g == 65535s);
println("TRUE");

uint h = -1u;

print("uint test1...");
Clover.assert(h.toString() == "4294967295" && h == 4294967295u);
println("TRUE");

long i = -1l;

print("long test1...");
Clover.assert(i.toString() == "18446744073709551615" && i == 18446744073709551615l);
println("TRUE");

