int a = 123;

Thread thread = new Thread() {
    a = 456;

    int d = 789;
    println("d in thread1 --> " + d.to_string());

    String e = d.to_string() + d.to_string() + d.to_string();

    println("e in thread1 --> " + e);

    print("thread var test...");
    if(a == 456 && d == 789 && e == "789789789") {
        println("OK");
    }
    else {
        println("FALSE");
        System.exit(2);
    }
}

String d = a.to_string() + a.to_string() + a.to_string();
println("d in main thread --> " + d);

int e = d.length() * 2 + d.length() * 3;

println("e in main thread --> " + e.to_string());

print("thread var test2...");
if(a == 123 && d == "123123123" && e == 45) {
    println("OK");
}
else {
    println("FALSE");
    System.exit(2);
}

thread.join();

println("HELLO THREAD end");

