int a = 123;

Thread thread = new Thread() {
    a = 456;

    int d = 789;
    println("d in thread1 --> " + d.to_string());

    String e = d.to_string() + d.to_string() + d.to_string();

    println("e in thread1 --> " + e);

    Thread thread2 = new Thread() {
        a = 567;

        int d = 789;
        println("d in thread2 --> " + d.to_string());

        String e = d.to_string() + d.to_string() + d.to_string();

        println("e in thread2 --> " + e);

        Thread thread3 = new Thread() {
            a = 890;

            int d = 999;
            println("d in thread3 --> " + d.to_string());

            String e = d.to_string() + d.to_string() + d.to_string();

            println("e in thread3 --> " + e);
        }

        Thread thread4 = new Thread() {
            a = 1890;

            int d = 1999;
            println("d in thread4 --> " + d.to_string());

            String e = d.to_string() + d.to_string() + d.to_string();

            println("e in thread4 --> " + e);
        }

        thread3.join();
        thread4.join();
    }

    Thread thread5 = new Thread() {
        a = 3567;

        int d = 3789;
        println("d in thread5 --> " + d.to_string());

        String e = d.to_string() + d.to_string() + d.to_string();

        println("e in thread5 --> " + e);

        Thread thread6 = new Thread() {
            a = 6890;

            int d = 6999;
            println("d in thread6 --> " + d.to_string());

            String e = d.to_string() + d.to_string() + d.to_string();

            println("e in thread6 --> " + e);
        }

        Thread thread7 = new Thread() {
            a = 61890;

            int d = 71999;
            println("d in thread7 --> " + d.to_string());

            String e = d.to_string() + d.to_string() + d.to_string();

            println("e in thread7 --> " + e);
        }

        thread6.join();
        thread7.join();
    }

    thread5.join();
    thread2.join();
}

String d = a.to_string() + a.to_string() + a.to_string();
println("d in main thread --> " + d);

int e = d.length() * 2 + d.length() * 3;

println("e in main thread --> " + e.to_string());

Thread thread8 = new Thread() {
    a = 456;

    int d = 789;
    println("d in thread8 --> " + d.to_string());

    String e = d.to_string() + d.to_string() + d.to_string();

    println("e in thread8 --> " + e);

    Thread thread9 = new Thread() {
        a = 567;

        int d = 789;
        println("d in thread9 --> " + d.to_string());

        String e = d.to_string() + d.to_string() + d.to_string();

        println("e in thread9 --> " + e);

        Thread thread10 = new Thread() {
            a = 890;

            int d = 999;
            println("d in thread10 --> " + d.to_string());

            String e = d.to_string() + d.to_string() + d.to_string();

            println("e in thread10 --> " + e);
        }

        thread10.join();
    }

    thread9.join();
}

thread.join();
thread8.join();

println("HELLO THREAD end");

