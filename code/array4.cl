
Array<int> array8 = { 111,222,333,444,555 };

array8.sample(3).toString().println();

Array<int> array9 = { 1,2,3,4,5,6,7 };

Array<String> array10 = array9.collect() String { |int item| return item.toString() };

print("array test2-1...");
if(array10 == { "1","2","3","4","5","6","7" }) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

/*
Array<int> array11 = { 1,2,3,4,5 };

print("array test2-2...");
if(array11.shift() == 1 && array11 == { 2,3,4,5 }) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}
*/

/*
print("array test2-3...");

if(array11.shift(2) == { 2,3 }) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

Array<int> array12 = { 1,2,3,4,5 };

print("array test2-4...");
if(array12.unshift(0) == { 0,1,2,3,4,5 }) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("array test2-5...");
if(array12.unshift({ 111,222,333}) == { 111,222,333,0,1,2,3,4,5}) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}
*/


println("array test2-6...");

Array<int> array12 = { 1,2,3,4,5 };

array12.shuffle().toString().println();

array12 = { 1,2,3,4,5 };

print("array test2-7...");
if(array12[1..3] == { 2,3}) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

SortableArray<SortTestObject> array15 = new SortableArray<SortTestObject>();

array15.add(new SortTestObject(5));
array15.add(new SortTestObject(3));
array15.add(new SortTestObject(4));
array15.add(new SortTestObject(7));
array15.add(new SortTestObject(1));

array15.sort();

print("array test2-8...");
if(array15.collect() int {|SortTestObject item| return item.getValue(); } 
    == { 1,3,4,5,7 }) 
{
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

SortableArray<SortTestObject> array16 = new SortableArray<SortTestObject>();

array16.add(new SortTestObject(5));
array16.add(new SortTestObject(3));
array16.add(new SortTestObject(4));
array16.add(new SortTestObject(7));
array16.add(new SortTestObject(1));

array16.sort() int { |SortTestObject left, SortTestObject right|
    if(left.getValue() < right.getValue()) {
        return -1;
    }
    else if(left.getValue() == right.getValue()) {
        return 0;
    }
    else {
        return 1;
    }
}

print("array test2-9...");
if(array16.collect() int {|SortTestObject item| return item.getValue(); } 
    == { 1,3,4,5,7 }) 
{
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}


SortableArray<String> array17 = new SortableArray<String>({ "apple","banana","strawberry","pineapple","pear" });

array17.sortBy() int {|String item|
    return item.length();
}

print("array test2-10...");
if(array17 == { "pear","apple","banana","pineapple","strawberry" })
{
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

Array<int> array18 = { 12,25,31,55,47,28,9 };

print("array test2-11...");
if(array18.takeWhile() bool {|int item| return item < 50; } == { 12,25,31 }) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

Array<int> array19 = { 1,2,3,4,5 };

print("array test2-12...");
if(array19.take(3) == { 1,2,3 }) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

Array<int> array20 = { 1,2,5,4,1,3,1,2,4,3 };

print("array test2-13...");
if(array20.uniq() == { 1,2,5,4,3 }) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

Array<String> array21 = { "Cat","dog","cat","mouse","Dog" };

print("array test2-14...");
if(array21.uniq() String {|String item| return item.downcase(); } == { "Cat","dog","mouse" })
{
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

/*
Array<String> array22 = { "dog","cat","mouse"}

print("array test2-15...");
if(array22.unshift("pig") == { "pig","dog","cat","mouse" }) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("array test2-16...");
if(array22.unshift({"zebra","giraffe"}) == {"zebra","giraffe","pig","dog","cat","mouse" })
{
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}
*/


Array<String> array23 = { "dog","cat","mouse","pig","zebra","giraffe" };

print("array test2-17...");
if(array23.valueAt(0, 0) == { "dog","dog" }) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("array test2-18...");
if(array23.valueAt(0..2, 3..6, 0) == { "dog","cat","pig","zebra","giraffe","dog"})
{
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}
