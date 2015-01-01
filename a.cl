
print("array test1...");
{ 1.0, 2.0, 3.1 }.eachWithIndex() { |float a, int i|
    if(a != 1.0 && a != 2.0 && a != 3.1) {
        println("FALSE");
        System.exit(2);
    }
}
println("TRUE");

Array<int> array = { 1, 2, 3 } + { 4, 5, 6 };

print("array test2...");
if(array == { 1, 2, 3, 4, 5, 6 }) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("array test3...");
if(array.include(5)) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

