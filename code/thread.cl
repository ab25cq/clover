int a = 123;

Thread thread = new Thread() {
    a = 456;

    int d = 789;
    println("d in thread1 --> " + d.to_str());

    String e = d.to_str() + d.to_str() + d.to_str();

    println("e in thread1 --> " + e);
}

String d = a.to_str() + a.to_str() + a.to_str();
println("d in main thread --> " + d);

int e = d.length() * 2 + d.length() * 3;

println("e in main thread --> " + e.to_str());

thread.join();

println("HELLO THREAD end");

