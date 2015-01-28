
Array<int> array8 = { 111, 222, 333, 444, 555 };

array8.sample(3).toString().println();


Array<int> array9 = { 1, 2, 3, 4, 5, 6, 7 };

Array<String> array10 = array9.collect() String { |int item| return item.toString() };

print("array test2-1...");
if(array10 == { "1", "2", "3", "4", "5", "6", "7" }) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

Array<int> array11 = { 1, 2, 3, 4, 5 };

print("array test2-2...");
if(array11.shift() == 1 && array11 == { 2, 3, 4, 5 }) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("array test2-3...");

if(array11.shift(2) == { 2, 3 }) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

Array<int> array12 = { 1, 2, 3, 4, 5 };

print("array test2-4...");
if(array12.unshift(0) == { 0, 1, 2, 3, 4, 5 }) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("array test2-5...");
if(array12.unshift({ 111, 222, 333}) == { 111, 222, 333, 0, 1, 2, 3, 4, 5}) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}


println("array test2-6...");

array12 = { 1, 2, 3, 4, 5 };

array12.shuffle().toString().println();

array12 = { 1, 2, 3, 4, 5 };

print("array test2-7...");
if(array12[1..2] == { 2, 3}) {
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


