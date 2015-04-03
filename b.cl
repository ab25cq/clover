
/*
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

Hash<String, int> hash2 = new Hash<String, int>();

hash2.put("Apple", 0);
hash2.put("Lemmon", 1);
hash2.put("A", 1);
hash2.put("B", 1);
hash2.put("C", 1);
hash2.put("D", 1);
hash2.put("E", 1);
hash2.put("f", 1);
hash2.put("g", 1);
hash2.put("h", 1);
hash2.put("i", 1);
hash2.put("j", 1);
hash2.put("k", 1);
hash2.put("l", 1);
hash2.put("m", 1);
hash2.put("n", 3);
hash2.put("o", 1);
hash2.put("p", 1);
hash2.put("q", 1);
hash2.put("r", 1);
hash2.put("s", 1);

print("Hash test3...");
if(hash2["Lemmon"] == 1 && hash2["n"] == 3 && hash2.length() == 21) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

hash2.put("n", 5);

print("Hash test4...");
if(hash2["n"] == 5 && hash2.length() == 21) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}
*/

Hash<String, int> b = {"AAA"=>1, "BBB"=>2 };

/*
print("Hash test5...");

if((b["CCC"] = 3) == 3 && b["AAA"] == 1 && b["BBB"] == 2 && b.length() == 3) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

b.each() {|String key, int value| 
    println("key --> " + key + " value --> " + value.toString());
}

b.clear();

print("Hash test6...");

if(b.length() == 0) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}
*/

b = { "AAA"=>0, "BBB"=>1, "CCC"=>2 };

Array<Tuple<String, int>> c = b.toArray();

c.each() { |Tuple<String, int> item|
    println("item --> {" + item.get1() + "," + item.get2().toString() + "}");
}


