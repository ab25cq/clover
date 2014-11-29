print("null test...");
if(null.toString() == "null" || null.toInt() == 0 || null.toBool() == false) {
    println("OK");
}
else {
    println("FALSE");
    System.exit(2);
}
