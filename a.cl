
print("string test1...");

if("ABC" * 2 == "ABCABC") {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

Regex regex = new OnigurumaRegex("XXX", false, false, false, new Encoding(Encodings.Utf8));

print("Regex test...");
if(regex.source() == "XXX" && regex.ignoreCase() == false && regex.multiLine() == false && regex.encode().encode == Encodings.Utf8)
{
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("string test2...");

if("AXXX" =~ regex) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("string test3...");

if("AXXX" =~ /^a/i) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("string test4...");

Array<String> group_string = new Array<String>();

if("ABCDEFGHI" =~ (/^(.)(.)(.)/, group_string) && group_string == {"A", "B", "C" }) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("string test5...");

if("ABCDEFGHI" =~ (/^(.)(.)(.)/, null)) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("string test6...");
if("AAA".asciiOnly()) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("string test7...");
if(!"あああ".asciiOnly()) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("string test8...");
if("あああ".toBytes().length() == 9) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("string test9...");
if("ABC"[0].toCharacter() == "A") {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("string test10...");
if("ABCDEFGHIJKL"[0..1] == "AB") {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("string test11...");
if("ABCDEFGHIJKL"[2..-1] == "CDEFGHIJKL") {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("string test12...");
if("ABCDEFGHIJKL"[2..-2] == "CDEFGHIJK") {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("string test13...");
if("abc".capitalize() == "Abc") {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("string test14...");
if("abc".cmp("bcd") == -1) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("string test15...");
if("abc".cmp("BCD", true) == -1) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("string test16...");
if("123".toInt() == 123) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("string test17...");
if("A".toCharacterCode() == 65) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("string test18...");
if("ABC\n".chomp() == "ABC" && "ABC\r\n".chomp() == "ABC" && "ABC\r".chomp() == "ABC") 
{
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("string test19...");
if("ABC:".chomp(":") == "ABC" && "".chomp() == "" && "\r".chomp() == "" && "\n".chomp() == "" && "\r\n".chomp() == "" && "a\r".chomp() == "a" && "a\r\n".chomp() == "a" && "a\n".chomp() == "a" && "ABC:!".chomp(":!") == "ABC") {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("string test20...");
if("ABC".chop() == "AB" && "あいうえお".chop() == "あいうえ") {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("string test21...");
if(""[0..-2] == "") {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("string test22...");
if("ABC"[1..5] == "BC") {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("string test23...");
if("ABCABC".count(/A/g) == 2) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("string test24...");
if("ABCABC".count(/[AB]/g) == 4) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

Clover.runTest("string test25") bool { 
    return "あいうえおあいうえお".count(/あ/g) == 2;
}

Clover.runTest("string test26") bool { 
    return "ABCABC".delete('A') == "BCBC";
}

/*
Clover.runTest("string test27") bool { 
    return "ABCABC".delete(/[AB]/) == "CC";
}
*/

Clover.runTest("string test28") bool { 
    return "あいうえお".delete('あ') == "いうえお";
}

/*
Clover.runTest("string test29") bool { 
    return "あいうえお".delete(/[あい]/) == "うえお";
}
*/

Clover.runTest("string test30") bool { 
    return "あいうえおHELLO".downcase() == "あいうえおhello";
}

Clover.runTest("string test31") bool {
    Array<String> array = new Array<String>();

    "aaa\r\nbbb\r\nccc\r\nddd".eachLine("\r\n") {|String line|
        array.add(line);
    }

    return array == {"aaa\r\n", "bbb\r\n", "ccc\r\n", "ddd"};
}

Clover.runTest("string test32") bool {
    String str = "";

    return str.empty();
}

Clover.runTest("string test33") bool {
    return "123abcabc123".sub(/abc/, "def") == "123defabc123"
}

Clover.runTest("string test34") bool {
    return "123abcabc123".sub(/abc/g, "def") == "123defdef123"
}

Clover.runTest("string test35") bool {
    return "abcabc".sub(/abc/g, "def") == "defdef";
}

Clover.runTest("string test36") bool {
    return "abcabc".sub(/abc/, "def") == "defabc";
}

Clover.runTest("string test37") bool {
    return "123ABC123ABC".sub(/ABC/g) String { |Array<String> group_strings, String prematch, String match, String postmatch| return match.downcase(); } == "123abc123abc";
}

Clover.runTest("string test38") bool {
    return "123ABC123ABC".sub(/A(.)(.)/) String {|Array<String> group_strings, String prematch, String match, String postmatch| return group_strings.length().toString(); } == "1232123ABC";
} 
Clover.runTest("string test39") bool {
    return "123ABC123ABC".sub(/[ABC]/g, { "A" => "D", "B" => "E", "C" => "F" }) == "123DEF123DEF";
}

Clover.runTest("string test40") bool {
    return "123ABC123ABC".include("ABC");
}

Clover.runTest("string test41") bool {
    return !"123ABC123ABC".include("D");
}

Clover.runTest("string test42") bool {
    return "123ABC123ABC".index("ABC") == 3;
}

Clover.runTest("string test43") bool {
    return "123ABC123ABC".index("ABC", 6) == 9;
}

Clover.runTest("string test44") bool {
    return "123ABC123ABC".index("ABC", 0, 2) == 9;
}

Clover.runTest("string test45") bool {
    return "123ABC123ABC".index(/ABC/g, 0, 2) == 9;
}

Clover.runTest("string test46") bool {
    return "123ABC123ABC".index(/ABC/g, 6) == 9;
}

Clover.runTest("string test47") bool {
    return "123ABC123ABC".index(/ABC/g) == 3;
}

Clover.runTest("string test48") bool {
    return "あいうえお123かきくけこ245".index(/かきくけこ/g) == 8;
}

Clover.runTest("string test49") bool {
    return "  \t   ABC DEF GHI".lstrip() == "ABC DEF GHI";
}

Clover.runTest("string test50") bool {
    int begin = 0;
    int end = 0;
    Array<Range> group_strings = null;

    "あいうえおかきくけこさしすせそ".match(/かき(く)け(こ)/g, 0, 1) bool {|int begin2, int end2, Array<Range> group_strings2|
        begin = begin2;
        end = end2;
        group_strings = group_strings2;

        return true;
    }

    return begin == 5 && end == 9 && group_strings == { 7..7, 9..9 };
}

