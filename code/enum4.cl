
FileMode value = FileMode.O_TRUNC|FileMode.O_APPEND|FileMode.O_ASYNC;

print("enum4 test1...");
if(value & FileMode.O_ASYNC && value & FileMode.O_TRUNC) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}
