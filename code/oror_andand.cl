int a = 123;
int b = 0;

a == 123 || (b = 2) == 2;

print("|| test...");
if(b == 0) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

int c = 234;
int d = 0;

c != 234 && (d = 2) == 2;

print("&& test...");
if(d == 0) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

int e = 345;
int f = 0;

e != 345 || (f=3) == 3;

print("|| test...");
if(f == 3) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

int g = 456;
int h = 0;

g == 456 && (h=4) == 4;

print("&& test...");
if(h == 4) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}


int i = 567;
int j = 0;
int k = 0;

i == 567 && ((j=5) == 5 || (k=6) == 6);


print("&& test...");
if(j == 5 && k == 0) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}
