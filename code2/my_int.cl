
Array<int> a = new Array<int>();

5.downTo(3) {|int n|
    a.add(n);
}

print("My int test...");
Clover.assert(a == { 5,4,3 });
println("TRUE");
