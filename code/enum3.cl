
print("native enum test...");

if(FileMode.O_APPEND.toInt() == 0x400 && FileMode.O_CREAT.toInt() == 0x40 && FileMode.O_TRUNC.toInt() == 0x200) 
{
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("native enum test2...");

if(EnumTest.method(FileMode.O_TRUNC)) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("native enum test3...");

if(FileMode.createElement(0x400) == FileMode.O_APPEND && FileMode.createElement(0x40) == FileMode.O_CREAT && FileMode.createElement(0x200) == FileMode.O_TRUNC)
{
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("native enum test4...");

if(EnumTest.method2(EnumB2.createElement(1)) && EnumTest.method3(EnumA2.ConstA)) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}
