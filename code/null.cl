print("null test...");
if(null.to_string() == "null" || null.to_int() == 0 || null.to_bool() == false) {
    println("OK");
}
else {
    println("FALSE");
    System.exit(2);
}
