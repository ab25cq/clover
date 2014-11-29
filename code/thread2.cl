int a = 0;

Thread thread = new Thread() {
    a = 1;

    println("a in thread1 --> " + a.toString());

    int b = 111;
    println("b in thread1 --> " + b.toString());

    int c = 1111;
    println("c in thread1 --> " + c.toString());

    Thread thread_a = new Thread() {
    }

/*
    print("thread1 var test...");
    if(a == 1 && b == 111 && c == 1111) {
        println("OK");
    }
    else {
        println("FALSE");
        System.exit(2);
    }
*/

    thread_a.join();

    Thread thread2 = new Thread() {
        a = 2;

        println("a in thread2 --> " + a.toString());

        int b = 222;
        println("b in thread2 --> " + b.toString());

        int c = 2222;
        println("c in thread2 --> " + c.toString());

/*
        print("thread2 var test...");
        if(a == 2 && b == 222 && c == 2222) {
            println("OK");
        }
        else {
            println("FALSE");
            System.exit(2);
        }
*/
    }

    Thread thread3 = new Thread() {
        a = 3;

        println("a in thread3 --> " + a.toString());

        int b = 333;
        println("b in thread3 --> " + b.toString());

        int c = 3333;
        println("c in thread3 --> " + c.toString());

/*
        print("thread3 var test...");
        if(a == 3 && b == 333 && c == 3333) {
            println("OK");
        }
        else {
            println("FALSE");
            System.exit(2);
        }
*/
    }

    thread2.join();
    thread3.join();
}

Thread thread4 = new Thread() {
    Thread thread_b = new Thread() {
    }

    thread_b.join();

    a = 4;

    println("a in thread4 --> " + a.toString());

    int b = 444;
    println("b in thread4 --> " + b.toString());

    int c = 4444;
    println("c in thread4 --> " + c.toString());

/*
    print("thread4 var test...");
    if(a == 4 && b == 444 && c == 4444) {
        println("OK");
    }
    else {
        println("FALSE");
        System.exit(2);
    }
*/

    Thread thread5 = new Thread() {
        a = 5;

        println("a in thread5 --> " + a.toString());

        int b = 555;
        println("b in thread5 --> " + b.toString());

        int c = 5555;
        println("c in thread5 --> " + c.toString());

/*
        print("thread5 var test...");
        if(a == 5 && b == 555 && c == 5555) {
            println("OK");
        }
        else {
            println("FALSE");
            System.exit(2);
        }
*/
    }

    Thread thread6 = new Thread() {
        a = 6;

        println("a in thread6 --> " + a.toString());

        int b = 666;
        println("b in thread6 --> " + b.toString());

        int c = 6666;
        println("c in thread6 --> " + c.toString());

/*
        print("thread6 var test...");
        if(a == 6 && b == 666 && c == 6666) {
            println("OK");
        }
        else {
            println("FALSE");
            System.exit(2);
        }
*/
    }

    thread5.join();
    thread6.join();
}

int b = 777;

println("a in main thread --> " + a.toString());
println("b in main thread --> " + b.toString());

/*
print("thread2 var test1...");
if(a == 0 && b == 777) {
    println("OK");
}
else {
    println("FALSE");
    System.exit(2);
}
*/

thread.join();
thread4.join();

println("HELLO THREAD end");

