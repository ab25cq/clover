
Hash<String, String> a = { "Apple" => "red", "Lemmon" => "yellow" };

print("Hash test1...");

if(a.get("Apple") == "red") {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("Hash test2...");

if({ "Apple" => 0, "Lemmon" => 1 }.get("Apple") == 0) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}
