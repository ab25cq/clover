print("int test1...");
Clover.assert(65.toChar() == 'A');
println("TRUE");

print("int test2...");
Clover.assert(123.toString() == "123");
println("TRUE");

3.times() {
    println("HELLO");
}

3.times() {|int n|
    n.toString().println();
}

print("char test...");
Clover.assert('A'.toInt() == 65 && 'あ'.toInt() == 12354);
println("TRUE");

print("char test2...");
Clover.assert('あ'.toString() == "あ" && 'A'.toString() == "A");
println("TRUE");

print("char test3...");
Clover.assert('A' == 'A' && 'A' < 'B');
println("TRUE");

print("char test4...");
Clover.assert('a'.upcase() == 'A' && 'あ'.upcase() == 'あ' && 'A'.upcase() == 'A');
println("TRUE");

print("char test5...");
Clover.assert('A'.downcase() == 'a' && 'あ'.downcase() == 'あ' && 'a'.downcase() == 'a');
println("TRUE");

print("float test...");
Clover.assert(1.1f.toInt() == 1 && 2.2f.toInt() == 2);
println("TRUE");

print("float test...");
Clover.assert(!(1.1f.toDouble() == 1.100000 && 2.2f.toDouble() == 2.200000));
println("TRUE");

print("float test...");
Clover.assert(1.1f.toString() == "1.100000");
println("TRUE");

print("double test...");
Clover.assert(1.1.toInt() == 1 && 2.2.toInt() == 2);
println("TRUE");

print("double test2...");
Clover.assert(1.1.toFloat() == 1.1f && 2.2.toFloat() == 2.2f);
println("TRUE");

print("double test3...");
Clover.assert(1.1.toString() == "1.100000");
println("TRUE");

print("bool test...");
Clover.assert(false.toString() == "false" && true.toString() == "true");
println("TRUE");

print("bool test2...");
Clover.assert(false.toInt() == 0 && true.toInt() == 1);
println("TRUE");

print("pointer test1...");
B"ABC".toPointer().toString().println();

Bytes data = B"ABC";

print("pointer test...");
Clover.assert(data.toPointer().equals(data.toPointer()));
println("TRUE");

Bytes data2 = B"ABC";
pointer p = data2.toPointer();

print("pointer test...");
Clover.assert(p.getByte().toInt().toChar() == 'A');
p.forward(1);
Clover.assert(p.getByte().toInt().toChar() == 'B');
p.forward(1);
Clover.assert(p.getByte().toInt().toChar() == 'C');
println("TRUE");

Bytes data3 = B"ABC";
pointer p2 = data3.toPointer();

print("pointer test...");
Clover.assert(p2.getByte().toInt().toChar() == 'A');
p2.forward(1);
Clover.assert(p2.getByte().toInt().toChar() == 'B');
p2.backward(1);
Clover.assert(p2.getByte().toInt().toChar() == 'A');
println("TRUE");
//p2.backward(1);

print("OnigurumaRegex test...");
Clover.assert(/ABC/.toString() == "ABC");
println("TRUE");

print("string test...");
Clover.assert("123".toInt() == 123 && "-1".toInt() == -1 && "abc".toInt() == 0);
println("TRUE");

print("string test2...");
Clover.assert("1.1".toDouble() == 1.1 && "-1.2".toDouble() == -1.2);
println("TRUE");

print("string test3...");
Clover.assert("ABC".length() == 3 && "あいうえお".length() == 5);
println("TRUE");

print("String test4...");
Clover.assert("あいうえお".char(0) == 'あ' && "あいうえお".char(-1) == 'お' && "あいうえお".char(1000) == null);
println("TRUE");

String a = "あいうえお";
(a.replace(0, 'か') == 'か').toString().println();
a.replace(-1, 'こ');

print("String test5...");
Clover.assert(a == "かいうえこ");
println("TRUE");

print("String test...");
Clover.assert("ABC".cmp("DEF") == -1 && "ABC".cmp("ABC") == 0 && "DEF".cmp("ABC") == 1 && "abc".cmp("ABC", true) == 0);
println("TRUE");

print("String test...");
Clover.assert("あいうえお" =~ /い/);
println("TRUE");

Array<String> group_strings = new Array<String>();


print("String test2...");
"あいうえお" =~ (/(.)(.)(.)(.)(.)/, group_strings);
Clover.assert(group_strings == { "あ", "い", "う", "え", "お" });
println("TRUE");

print("String test...");
Clover.assert("あいうえお"[0..1] == "あ");
println("TRUE");

print("String test...");
Clover.assert("あいうえお"[0..-1] == "あいうえ");
println("TRUE");

print("String test...");
Clover.assert("あいうえお"[2..null] == "うえお");
println("TRUE");

print("String test...");
Clover.assert("あいうえお"[4..2] == "えう");
println("TRUE");

print("String test...");
Clover.assert("あいうえお"[null..2] == "おえう");
println("TRUE");

"あいうえお".each() {|char c|
    c.toString().println();
}

print("String test...");
Clover.assert("abc\n".chomp() == "abc" && "abc\r".chomp() == "abc" && "abc\r\n".chomp() == "abc");
println("TRUE");

print("String test...");
Clover.assert("あいうえお".chop() == "あいうえ" && "あいうえお\r\n".chop() == "あいうえお");
println("TRUE");

print("String test...");
Clover.assert("あああああ".count('あ') == 5);
println("TRUE");

print("String test...");
Clover.assert("あいうえお".count(/[あう]/) == 2);
println("TRUE");

print("String test...");
Clover.assert("あいうえお".delete('あ') == "いうえお");
println("TRUE");

print("String test...");
Clover.assert("あいうえお".delete(/[うえ]/g) == "あいお");
println("TRUE");

print("String test...");
Clover.assert("あいうえお".sub(/[うえ]/g, "け") == "あいけけお");
println("TRUE");

print("String test...");
Clover.assert("あいうえお".sub(/[うえ]/g) {|Array<String> group_strings, String prematch, String match, String postmatch| return match.length().toString(); } == "あい11お");
println("TRUE");

print("String test...");
Clover.assert("あいうえお".sub(/[うえ]/g, { "う"=>"く", "え"=>"け" }) == "あいくけお");
println("TRUE");

print("String test...");
Clover.assert("あいうえお".sub(/[うえ]/g, { "あ"=>"か", "う"=>"く"}) == "あいくえお");
println("TRUE");

print("String test...");
Clover.assert("ABCDEFG".sub(/./g, {"A"=>"a"}) == "aBCDEFG");
println("TRUE");

print("String test....");
Clover.assert("あいうえお".include("うえ") && !"あいうえお".include("かき"));
println("TRUE");

print("String test...");
Clover.assert("あいうえお".index("い") == 1 && "あいうえお".index("か") == -1 && "あいうえおあいうえお".index("あいう", 0, 2) == 5 && "あいうえおあいうえお".index("お", 5) == 9);
println("TRUE");

print("String strip test...");
Clover.assert("   あいう   ".strip() == "あいう" && "あいう   ".strip() == "あいう" && "   あいう".strip() == "あいう" && "    ".strip() == "");
println("TRUE");

print("String lstrip test...");
Clover.assert("   あいう   ".lstrip() == "あいう   " && "あいう   ".lstrip() == "あいう   " && "   あいう".lstrip() == "あいう" && "     ".lstrip() == "");
println("TRUE");

print("String rstrip test...");
Clover.assert("   あいう   ".rstrip() == "   あいう" && "あいう   ".rstrip() == "あいう" && "   あいう".rstrip() == "   あいう" && "     ".rstrip() == "");
println("TRUE");

print("String test...");
Clover.assert("あいうえお".reverse() == "おえういあ");
println("TRUE");

print("String test...");
Clover.assert("あいうえお".scan(/./) == { "あ", "い", "う", "え", "お" } && "あいうえお123かきくけこ456".scan(/[0-9]+/) == { "123", "456" });
println("TRUE");

print("String test...");
Clover.assert("あいう かきく さしす".split() == { "あいう", "かきく", "さしす" }
    && "あいう123かきく456さしす789".split(/[0-9]+/) == { "あいう", "かきく", "さしす" });
println("TRUE");


print("String test...");
Clover.assert("あああいいいううう".squeeze() == "あいう");
println("TRUE");

print("String test...");
Clover.assert("AbcDEf".swapcase() == "aBCdeF");
println("TRUE");

print("String test...");
Clover.assert("abc".tr("a-z", "A-Z") == "ABC" 
                && "ABCABC".tr("A", "X") == "XBCXBC");
println("TRUE");

print("String test...");
Clover.assert("0xFF".hex() == 255 && "10".hex() == 16);
println("TRUE");

print("String test...");
Clover.assert("010".oct() == 8 && "20".oct() == 16);
println("TRUE");

Array<String> array = new Array<String>();

array.add("ABC");
array.add("DEF");
array.add("GHI");

print("Array test...");
Clover.assert(array == {"ABC", "DEF", "GHI"});
println("TRUE");

print("Array test...");
Clover.assert({"あ", "い", "う"}[0] == "あ"
            && {"あ", "い", "う"}[1] == "い"
            && {"あ", "い", "う"}[-1] == "う"
            && {"あ", "い", "う"}[-2] == "い"
            && {"あ", "い", "う"}[100] == null);
println("TRUE");

print("Array test...");
Clover.assert({"あ", "い", "う"}.length() == 3
        && {1,2,3,4,5}.length() == 5);
println("TRUE");

Array<String> array2 = { "あ", "い", "う" };
array2[1] = "き";

print("Array test...");
Clover.assert(array2 == { "あ", "き", "う" });
println("TRUE");

array2[5] = "X";

print("Array test...");
Clover.assert(array2 == { "あ", "き", "う",null,null,"X" });
println("TRUE");

Array<String> array3 = { "あいうえお", "かきくけこ", "さしすせそ" };

Array<String> array4 = array3.clone();

print("Array test...");
Clover.assert(array3 == array4);
println("TRUE");

print("Array test...");
Clover.assert(array3.ID() != array4.ID());
for(int i=0; i<3; i++) {
    Clover.assert(array3[i].ID() != array4[i].ID());
}
println("TRUE");

array3 = { "あいうえお", "かきくけこ", "さしすせそ" };

array4 = array3.dup();

print("Array test...");
Clover.assert(array3 == array4);
println("TRUE");

print("Array test...");
Clover.assert(array3.ID() != array4.ID());
for(int i=0; i<3; i++) {
    Clover.assert(array3[i].ID() == array4[i].ID());
}
println("TRUE");

print("Array test...");
Clover.assert({"あいうえお", "123", "さしすせそ", "456", "さしすせそ" }
            .find() {|String item| return item.toInt() != 0 } == "123");
println("TRUE");

print("Array test...");
Clover.assert({"あいうえお", "123", "さしすせそ", "456", "さしすせそ" }
            .find(2) {|String item| return item.toInt() != 0 } == "456");
println("TRUE");

print("Array test...");
Clover.assert({"あいうえお", "123", "さしすせそ", "456", "さしすせそ" }
            .find(3) {|String item| return item.toInt() != 0 } == null);
println("TRUE");

print("Array test...");
Clover.assert({ "あ", "い" } * 3 == { "あ", "い", "あ", "い", "あ", "い" });
println("TRUE");

print("Array test...");
Clover.assert({ "あ", "い", "う" } + { "え", "お" } == { "あ", "い", "う", "え", "お"});
println("TRUE");

print("Array test...");
Clover.assert({ "あああ", "いいい", "ううう" } == { "あああ", "いいい", "ううう" } && {1, 2, 3 } != { 3, 4, 5 });
println("TRUE");

Array<String> array5 = { "あああ", "いいい", "ううう" };

array5.deleteAt(1);

print("Array test....");
Clover.assert(array5 == { "あああ", "ううう" });
println("TRUE");

Array<String> array6 = { "あああ", "いいい", "ううう" };

String str = array6.pop();

print("Array test...");
Clover.assert(str == "ううう" && array6 == {"あああ", "いいい"});
println("TRUE");

print("Array test...");
Clover.assert({1,2,3,4,5}.include(5) && !{ 1,2,3 }.include(6));
println("TRUE");

print("Array test....");
Clover.assert({1,2,2,3,3,3,4,4,4,4,5,5,5,5,5}.count(4) == 4);
println("TRUE");

Array<int> array7 = { 1,2,2,3,3,3,4,4,4,4 };

array7.delete(2);

print("Array test...");
Clover.assert(array7 == { 1,3,3,3,4,4,4,4 });
println("TRUE");

print("Array test...");
Clover.assert({1,2,1,2}.index(2) == 1 && {1,2,1,2}.index(2, 2) == 3);
println("TRUE");

print("Array test...");
Clover.assert({1,2,3,1,2,3,1,2,3}.index() { |int n| return n == 3} == 2);
println("TRUE");

print("Array test...");
Clover.assert({ "あ", "い", "う" }.join(" ") == "あ い う" && { 1,2,3 }.join("+") == "1+2+3");
println("TRUE");

print("Array test...");
Clover.assert({1,2,3,1,2,3,1,2,3}.rindex(2) == 7 && {1,2,3,1,2,3,1,2,3}.rindex(2, 2) == 4);
println("TRUE");

Array<String> array8 = { "あ", "い", "う" };

array8.each() { |String element|
    element.println();
}

Array<String> array9 = { "あ", "い", "う" };

array9.each() { |String element, int index|
    printf("%d: %ls\n", index, element);
}

print("Array test...");
Clover.assert({ 1, 2, 3 }.collect() {|int n| return n * 2 } == { 2, 4, 6 });
println("TRUE");

print("Array test...");
Clover.assert({1,2,3}.concat({4,5}) == { 1,2,3,4,5 });
println("TRUE");

print("Array test...");
Clover.assert({111,222,333,222}.deleteIf() {|int n| return n == 222 } == { 111,333 });
println("TRUE");

Array<String> array10 = { "あああ", "いいい", "ううう" };

array10.fill("あいう");

print("Array test...");
Clover.assert(array10 == { "あいう", "あいう", "あいう" });
println("TRUE");

Array<int> array11 = { 1,2,3,4,5,6,7,8,9 };

array11.fill(0, 1..3);

print("Array test...");
Clover.assert(array11 == { 1,0,0,4,5,6,7,8,9 });
println("TRUE");

Array<int> array12 = { 1,2,3,4,5 };

array12.insert(3, 0);

print("Array insert test...");
Clover.assert(array12 == { 1,2,3,0,4,5 });
println("TRUE");

array12 = { 1,2,3,4,5 };

array12.insert(7, 0);

print("Array insert test...");
Clover.assert(array12 == { 1,2,3,4,5,null,null,0 });
println("TRUE");

Array<int> array13 = { 1,2,3,4,5 };

array13.insert(2, { 0, 0, 0 });

print("Array insert test...");
Clover.assert(array13 == { 1,2,0,0,0,3,4,5 });
println("TRUE");

array13 = { 1,2,3,4,5 };

array13.insert(7, { 0, 0, 0 });

print("Array insert test...");
Clover.assert(array13 == { 1,2,3,4,5,null,null, 0,0,0});
println("TRUE");

Array<int> array14 = { 1,2,3 };

array14.reverse();

print("Array reverse test...");
Clover.assert(array14 == { 3, 2, 1 });
println("TRUE");

print("Array select...");
Clover.assert({1,2,3,4,5,6,7,8,9}.select() {|int num| return num > 5; } == { 6,7,8,9 });
println("TRUE");
