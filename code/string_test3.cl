
print("string test1...");

if("ABC" * 2 == "ABCABC") {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

Regex regex = new OnigurumaRegex("XXX", false, false, false, Encoding.Utf8);

print("Regex test...");
if(regex.source() == "XXX" && regex.ignoreCase() == false && regex.multiLine() == false && regex.encode() == Encoding.Utf8)
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

print("string test3.5...");

if(!("AXXX" =~ /^b/i)) {
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
if("ABC"[0].toString() == "A") {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("string test10...");
if("ABCDEFGHIJKL"[0..2] == "AB") {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("string test11...");
if("ABCDEFGHIJKL"[2..-1] == "CDEFGHIJK") {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("string test12...");
if("ABCDEFGHIJKL"[2..-2] == "CDEFGHIJ") {
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
if("A"[0].toInt() == 65) {
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
if("ABC".chop() == "AB" && "あいうえお".chop() == "あいうえ" && "ABC\r\n".chop() == "ABC") {
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
if("ABCABC".count(/A/) == 2) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("string test24...");
if("ABCABC".count(/[AB]/) == 4) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

Clover.runTest("string test25") bool { 
    return "あいうえおあいうえお".count(/あ/) == 2;
}

Clover.runTest("string test26") bool { 
    return "ABCABC".delete('A') == "BCBC";
}

Clover.runTest("string test26.5") bool { 
    return "ABCABC".delete(/[AB]/) == "BCABC";
}

Clover.runTest("string test27") bool { 
    return "ABCABD".delete(/[AB]/g) == "CD";
}

Clover.runTest("string test28") bool { 
    return "あいうえお".delete('あ') == "いうえお";
}

Clover.runTest("string test29") bool { 
    return "あいうえお".delete(/[あい]/g) == "うえお";
}

Clover.runTest("string test30") bool { 
    return "あいうえおHELLO".downcase() == "あいうえおhello";
}

Clover.runTest("string test30.5") bool {
    Array<String> array = new Array<String>();

    "aaa\nbbb\nccc\nddd\n".eachLine() {|String line|
        array.add(line);
    }

    return array == {"aaa\n", "bbb\n", "ccc\n", "ddd\n"};
}

Clover.runTest("string test30.7") bool {
    Array<String> array = new Array<String>();

    "aaa\nbbb\nccc\nddd".eachLine() {|String line|
        array.add(line);
    }

    return array == {"aaa\n", "bbb\n", "ccc\n", "ddd"};
}

Clover.runTest("string test31") bool {
    Array<String> array = new Array<String>();

    "aaa\r\nbbb\r\nccc\r\nddd".eachLine("\r\n") {|String line|
        array.add(line);
    }

    return array == {"aaa\r\n", "bbb\r\n", "ccc\r\n", "ddd"};
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
    return "123ABC123ABC".sub(/ABC/g) String { 
        |Array<String> group_strings, String prematch, String match, String postmatch| 
        return match.downcase(); 
    } == "123abc123abc";
}

Clover.runTest("string test38") {
    return "123ABC123ABC".sub(/A(.)(.)/) {
        |Array<String> group_strings, String prematch, String match, String postmatch| 
        return group_strings.length().toString(); 
    } == "1232123ABC";
} 

Clover.runTest("string test38.5") {
    "123ABC123ABC".sub(/A(.)(.)/) {
        |Array<String> group_strings, String prematch, String match, String postmatch| 
        return group_strings.collect() {|String group_string| return group_string.downcase(); }.join("");
    }.println();
    return "123ABC123ABC".sub(/A(.)(.)/) {
        |Array<String> group_strings, String prematch, String match, String postmatch| 
        return group_strings.collect() {|String group_string| return group_string.downcase(); }.join("");
    } == "123bc123ABC";
} 

Clover.runTest("string test39") {
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

    "あいうえおかきくけこさしすせそ".match(/かき(く)け(こ)/g, 0, 1) {|int begin2, int end2, Array<Range> group_strings2|
        begin = begin2;
        end = end2;
        group_strings = group_strings2;
        break;
    }

    return begin == 5 && end == 10 && group_strings == { 7..8, 9..10 };
}

Clover.runTest("string test51") {
    return "あいうえお".reverse() == "おえういあ";
}

Clover.runTest("string test52") {
    return "あいうえお".rindex("うえ") == 2;
}

Clover.runTest("string test53") {
    return "あいうえおあいうえお".rindex("うえ") == 7;
}

Clover.runTest("string test54") {
    return "あいうえおあいうえお".rindex("うえ", 5) == 2;
}

Clover.runTest("string test55") {
    return "あいうえおあいうえお".rindex("うえ", null, 2) == 2;
}

Clover.runTest("string test56") {
    return "あいうえお".rindex(/うえ/) == 2;
}

Clover.runTest("string test57") {
    return "あいうえおあいうえお".rindex(/うえ/) == 7;
}

Clover.runTest("string test58") {
    return "あいうえおあいうえお".rindex(/うえ/, 5) == 2;
}

Clover.runTest("string test59") {
    return "あいうえおあいうえお".rindex(/うえ/g, null, 2) == 2;
}

Clover.runTest("string test60") {
    return "あいうえおあいうえお".rindex(/い/) == 6;
}

Clover.runTest("string test61") {
    return "あいうえお\n    \n  \t".rstrip() == "あいうえお";
}

Clover.runTest("string test62") {
    return "北海道:札幌, 青森:青森, 岩手:盛岡".scan(/(\w+):(\w+)/) == {{"北海道","札幌"}, {"青森","青森"}, {"岩手","盛岡"}};
}

Clover.runTest("string test63") {
    return "of the people, by the people, for the people".scan(/\w+/) == {"of", "the", "people", "by", "the", "people", "for", "the", "people"};
}

Clover.runTest("string test64") {
    return "of the people, by the people, for the people".scan(/people/) == {"people", "people", "people"};
}

Clover.runTest("string test65") {
    return "of the people, by the people, for the people".scan(/people/) == {"people", "people", "people"};
}

Clover.runTest("string test66") {
    return "hello,world,clover".split(/,/) == {"hello", "world", "clover"};
}

Clover.runTest("string test67") {
    return "hello, world   ;    clover".split(/\s*(,|;)\s*/) == {"hello", ",", "world", ";", "clover"};
}

Clover.runTest("string test68") {
    return "    hello    world      clover\r\n  ".split() == {"hello", "world", "clover" };
}

Clover.runTest("string test69") {
    return "abcあいうえお".split(//) == {"a", "b", "c", "あ", "い", "う", "え", "お" };
}

Clover.runTest("string test70") {
    return "he    llo".split(/\s*/) == { "h", "e", "l", "l", "o" };
}

Clover.runTest("string test71") {
    return "hello,world,clover,,".split(/,/) == { "hello", "world", "clover" };
}

Clover.runTest("string test72") {
    return "hello,world,clover,,".split(/,/, -1) == { "hello", "world", "clover", "", "" };
}

Clover.runTest("string test73") {
    return "hello,world,clover".split(/,/, 2) == { "hello", "world,clover" };
}

Clover.runTest("string test74") {
    return "hello,world,clover,,".split(/,/, 3) == { "hello", "world", "clover,," };
}

Clover.runTest("string test75") {
    return "hello,world,clover,,".split(/,/, 4) == { "hello", "world", "clover", "," };
}

Clover.runTest("string test76") {
    return "hello,world,clover,,".split(/,/, 6) == { "hello", "world", "clover", "", "" };
}

Clover.runTest("string test77") {
    return ",,hello,world,clover,,".split(/,/, -1) == { "", "", "hello", "world", "clover", "", "" };
}

/*
Clover.runTest("string test77") {
    return ",,hello,world,clover,,".split(/,/) == { "", "", "hello", "world", "clover" };
}
*/

Clover.runTest("string test78") {
    return "ccccllllloooovvvveeer".squeeze() == "clover";
}

Clover.runTest("string test79") {
    return "あいうえおClOvErかきくけこ".swapcase() == "あいうえおcLoVeRかきくけこ";
}

Clover.runTest("string test80") {
    return "abcdefghi".tr("abcde", "あいうえお") == "あいうえおfghi";
}

Clover.runTest("string test81") {
    return "8429503671".tr("1-9", "A-I") == "HDBIE0CFGA";
}

Clover.runTest("string test82") {
    return "8429503671".tr("1-9", "ABC#") == "##B##0C##A";
}

Clover.runTest("string test83") {
    return "8429503671".tr("^1-9", "@") == "84295@3671";
}

Clover.runTest("string test84") {
    return "0144".oct() == 100 && "-2322".oct() == -1234 && "377".oct() == 255;
}

Clover.runTest("string test85") {
    return "0x4d2".hex() == 1234 && "-10e1".hex() == -4321 && "ff is 255".hex() == 255;
}

Clover.runTest("string test86") {
    return "111.222".toDouble() == 111.222 && "-123.2".toDouble() == -123.2;
}

