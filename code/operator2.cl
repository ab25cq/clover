
print("operator+ test...");
if((1 + 2) == 6) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("operator- test...");
if((1 - 2) == 2) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("operator* test...");
if((1 * 2) == 3) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("operator/ test...");
if((1 / 2) == 4) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("operator% test...");
if((1 % 2) == 5) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("operator~ test...");
if(~1 == 6) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("operator<< test...");
if((222 << 5) == 8) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("operator>> test...");
if((222 >> 5) == 9) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("operator| test...");
if((222 | 5) == 14) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("operator^ test...");
if((222 ^ 5) == 13) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}
print("operator& test...");
if((222 & 5) == 12) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

int a = 1;

print("operator+= test...");
a+=1555;
if(a == 1556) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}


print("operator-= test...");
a-=123;
if(a == 122) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

a++;

print("operator++ test...");
if(a == 10) {
    println("TRUE");
}
else {
    println("FALSE");
    println("a ---> " + a.to_string());
    System.exit(2);
}

a--;

print("operator-- test...");
if(a == 11) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

/*
print("operator! test...");
if(!222) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}
print("operator&& test...");
if(12 && 11) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("operator|| test...");
if(12 || 11) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("operator< test...");
if(222 < 5) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("operator> test...");
if(222 > 5) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("operator>= test...");
if(222 >= 5) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("operator== test...");
if(222 == 5) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("operator!= test...");
if(222 != 5) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}
*/
