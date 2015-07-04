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

Hash<String, int> b = {"AAA"=>1, "BBB"=>2 };

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

b = { "AAA"=>0, "BBB"=>1, "CCC"=>2 };

Array<Tuple<String, int>> c = b.toArray();

print("Hash Test7...");
if(c[0] == ("AAA",0) && c[1] == ("BBB",1) && c[2] == ("CCC",2)) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("Hash Test8...");
if({"AAA"=>1, "BBB"=>2, "CCC"=>3} == {"AAA"=>1, "BBB"=>2, "CCC"=>3}) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("Hash Test9...");
if({"AAA"=>1, "BBB"=>2, "CCC"=>3} == {"CCC"=>3, "AAA"=>1, "BBB"=>2}) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("Hash Test10...");

if({"AAA"=>1, "BBB"=>2, "CCC"=>3}.assoc("AAA") == ("AAA",1)) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("Hash Test11...");

{"AAA"=>1, "BBB"=>2, "CCC"=>3}.toString().println();

print("Hash Test12...");

Hash<String,int> aa = new Hash<String, int>();

aa.put("AAA", 111);
aa.put("BBB", 222);
aa.put("CCC", 333);

if(aa == {"AAA"=>111, "BBB"=>222, "CCC"=>333}) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("Hash Test13...");

if(aa["AAA"] == 111) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("Hash Test14...");

if({"AAA"=>111, "BBB"=>222, "CCC"=>333}.clone() == {"AAA"=>111, "BBB"=>222, "CCC"=>333})
{
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("Hash Test15...");

Hash<String, int> d = {"AAA"=>111, "BBB"=>222, "CCC"=>333};

d.erase("AAA");

if(d == {"BBB"=>222, "CCC"=>333})
{
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("Hash Test16...");

Hash<String, int> e = {"AAA"=>111, "BBB"=>222, "CCC"=>333};

if(e.erase("DDD") == null) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

Hash<String, int> f = { "AAA"=>111, "BBB"=>222, "CCC"=>333};

print("Hash Test17...");
try {
    f.fetch("DDD");
} catch(KeyNotFoundException e) {
    println("TRUE");
}

Hash<String, int> g = { "AAA"=>111, "BBB"=>222, "CCC"=>333 };

print("Hash Test18...");

int h = g.delete("AAA") int { |String key|
    return g[key];
}

if(g == {"BBB"=>222, "CCC"=>333} && h == 111) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("Hash test19...");

Hash<String, int> i = {"AAA"=>111, "BBB"=>222, "CCC"=>333 };

i.deleteIf() bool { |String key, int value|
    return key == "AAA";
}

if(i == {"BBB"=>222, "CCC"=>333}) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

Hash<String, int> j = {"AAA"=>111, "BBB"=>222, "CCC"=>333 };

print("Hash test20...");
if(j.key(111) == "AAA") {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

Hash<String, int> k = {"AAA"=>111, "BBB"=>222, "CCC"=>333 };

print("Hash test30...");

Hash<int, String> l = k.invert();

if(l == {111=>"AAA", 222=>"BBB", 333=>"CCC"}) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("Hash test31...");
if({"AAA"=>111, "BBB"=>222, "CCC"=>333}.select() 
    bool {|String key, int value| return key == "BBB" || key == "CCC"; }
    == {"BBB"=>222, "CCC"=>333}
)
{
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("Hash test32...");

Hash<String, int> m = {"AAA"=>111, "BBB"=>222 }
Hash<String, int> n = {"AAA"=>123, "CCC"=>333 }

m.merge(n) int {|String key, int value1, int value2| 
    return value1 + value2; 
}

if(m == {"AAA"=>234, "BBB"=>222, "CCC"=>333}) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}


print("Hash test33...");

Hash<String, int> o = {"AAA"=>111, "BBB"=>222 }
Hash<String, int> p = {"AAA"=>123, "CCC"=>333 }

o.merge(p);

if(o == {"AAA"=>123, "BBB"=>222, "CCC"=>333}) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

Hash<String, int> q = {"AAA"=>111, "BBB"=>222, "CCC"=>333, "DDD"=>444 };

print("Hash test34...");

if(q.valueAt("AAA", "CCC") == { 111, 333 }) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

