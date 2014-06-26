int a = 0;

Thread thread = new Thread() {
    a = 1;

    println("a in thread1 --> " + a.to_str());

    int b = 111;
    println("b in thread1 --> " + b.to_str());

    int c = 1111;
    println("c in thread1 --> " + c.to_str());

    Thread thread_a = new Thread() {
    }

    thread_a.join();

    Thread thread2 = new Thread() {
        a = 2;

        println("a in thread2 --> " + a.to_str());

        int b = 222;
        println("b in thread2 --> " + b.to_str());

        int c = 2222;
        println("c in thread2 --> " + c.to_str());
    }

    Thread thread3 = new Thread() {
        a = 3;

        println("a in thread3 --> " + a.to_str());

        int b = 333;
        println("b in thread3 --> " + b.to_str());

        int c = 3333;
        println("c in thread3 --> " + c.to_str());
    }

    thread2.join();
    thread3.join();
}

Thread thread4 = new Thread() {
    Thread thread_b = new Thread() {
    }

    thread_b.join();

    a = 4;

    println("a in thread4 --> " + a.to_str());

    int b = 444;
    println("b in thread4 --> " + b.to_str());

    int c = 4444;
    println("c in thread4 --> " + c.to_str());

    Thread thread5 = new Thread() {
        a = 5;

        println("a in thread5 --> " + a.to_str());

        int b = 555;
        println("b in thread5 --> " + b.to_str());

        int c = 5555;
        println("c in thread5 --> " + c.to_str());
    }

    Thread thread6 = new Thread() {
        a = 6;

        println("a in thread6 --> " + a.to_str());

        int b = 666;
        println("b in thread6 --> " + b.to_str());

        int c = 6666;
        println("c in thread6 --> " + c.to_str());
    }

    thread5.join();
    thread6.join();
}

int b = 777;

println("a in main thread --> " + a.to_str());
println("b in main thread --> " + b.to_str());

thread.join();
thread4.join();

println("HELLO THREAD end");

