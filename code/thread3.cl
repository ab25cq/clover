int a = 0;

Thread thread = new Thread() {
    a = 1;

    int d = 111;
    println("d in thread1 --> " + d.to_str());

    String e = d.to_str() + d.to_str() + d.to_str();

    println("e in thread1 --> " + e);

    Thread thread2 = new Thread() {
        a = 2;

        int d = 222;
        println("d in thread2 --> " + d.to_str());

        String e = d.to_str() + d.to_str() + d.to_str();

        println("e in thread2 --> " + e);

        Thread thread3 = new Thread() {
            a = 3;

            int d = 333;
            println("d in thread3 --> " + d.to_str());

            String e = d.to_str() + d.to_str() + d.to_str();

            println("e in thread3 --> " + e);
        }

        Thread thread4 = new Thread() {
            a = 4;

            int d = 444;
            println("d in thread4 --> " + d.to_str());

            String e = d.to_str() + d.to_str() + d.to_str();

            println("e in thread4 --> " + e);
        }

        thread3.join();
        thread4.join();
    }

    thread2.join();
}

String d = a.to_str() + a.to_str() + a.to_str();
println("d in main thread --> " + d);

int e = d.length() * 2 + d.length() * 3;

println("e in main thread --> " + e.to_str());

thread.join();

println("HELLO THREAD end");

