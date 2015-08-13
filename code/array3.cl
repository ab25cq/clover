
print("array test1...");
{ 1.0f,2.0f,3.1f }.each() { |float a, int i|
    if(a != 1.0f && a != 2.0f && a != 3.1f) {
        println("FALSE");
        System.exit(2);
    }
}
println("TRUE");

Array<int> array = { 1,2,3 } + { 4,5,6 };

print("array test2...");
if(array == { 1,2,3,4,5,6 }) {
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

print("array test3.5...");
if(array.find() bool { |int item| return item % 3 == 0; } == 3) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("array test4...");
array.clear();
if(array.empty()) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

Array<int> array2 = { 111,222,333 };
array2 = array2.collect() int { |int item| return item * 2; }

print("array test5...");
if(array2[0] == 222 && array2[1] == 444 && array2[2] == 666) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

Array<int> array3 = { 777,888,999 };

array3.concat({ 111,222,333});

print("array test7...");
if(array3[3] == 111 && array3[4] == 222 && array3[5] == 333) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

array3.concat({ 111 });

print("array test8...");
if(array3.count(111) == 2) {
    println("TRUE");
}
else {
    println("FAlSE");
    System.exit(2);
}

array3.delete(111);

print("array test9...");
if(array3.length() == 5) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

array3.deleteAt(0);

print("array test10...");

if(array3.length() == 4) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

array3.deleteIf() bool {|int item|
    return item == 222;
}

print("array test11...");
if(array3.length() == 3) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

array = { 40,50,55,60,30,20,70 };

array.dropWhile() bool { |int item|
    return item < 60;
}

print("array test12...");
if(array == { 60,30,20,70}) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

array = { 1,2,3,4,5 };

Array<int> array5 = new Array<int>();

array.cycle(2) { |int item|
    array5.add(item);
}

print("array test13...");
if(array5 == { 1,2,3,4,5,1,2,3,4,5 }) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

array = { 111,222,333 };

Array<int> array6 = new Array<int>();

array.eachIndex() {|int index|
    array6.add(index);
}

print("array test14...");
if(array6 == { 0, 1, 2 }) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

Array<int> array7 = {111,222,333};

print("array test15...");
if(array7.items(100, -1) == -1) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("array test16...");
if(array7.items(100) int { |int index| return -1; } == -1) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

array7.fill(100);

print("array test17...");
if(array7 == { 100,100,100 }) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

array7.fill(200, 1..2);

print("array test18...");
if(array7 == { 100,200,100 }) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

array7.fill() int { |int index| return 111; }

print("array test19...");
if(array7 == { 111,111,111 } ) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

array7.fill(1..null) int { |int index| return 333; }

print("array test20...");
if(array7 == { 111,333,333 }) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("array test21...");
if(array7.findIndex(333) == 1) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("array test22...");
if(array7.findIndex(333, 2) == 2) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("array test22...");
if(array7.findIndex(2) bool { |int item| return item == 333; } == 2) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("array test23...");

if(array7.first() == 111) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

Array<int> array8 = array7.first(2);

print("array test24...");
if(array8 == {111,333}) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("array test25...");
if(array8.join(",") == "111,333") {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}


array8 = {111,222,333,444,555 };

print("array test26...");

if(array8.pop() == 555 && array8 == { 111,222,333,444 }) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("array test27...");

if(array8.pop(2) == { 333,444 } && array8 == { 111,222 }) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

array8 = { 111,222,333,444,555 };

print("array test28...");
if(array8.reverse() == { 555,444,333,222,111 }) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

array8 = { 111,222,333,444,555 };

print("array test29...");
if(array8.rotate(2) == { 333,444,555,111,222}) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

array8 = { 111,222,333,444,555 };

print("array test30...");
if(array8.rotate(-3) == { 333,444,555,111,222 }) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

Array<String> array9 = new Array<String>();

array9.add("AAAA");
array9.add("BBBB");
array9.add(null);
array9.add(null);
array9.add(null);
array9.add(null);
array9.add(null);
array9.add(null);

array9.compact();

print("array test31...");
if(array9.length() == 2) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}
