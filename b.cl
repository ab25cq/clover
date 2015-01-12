
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
