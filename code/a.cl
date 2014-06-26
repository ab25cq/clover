
int a = 0;

println(Clover.output_to_str() {
    a = 1;

    int b = 111;
    println("b in thread1 --> " + b.to_str());

    int c = 1111;
    println("c in thread1 --> " + c.to_str());

    int d1 = 33;

    println(Clover.output_to_str() {
        a = 2;

        int b = 222;
        println("b in thread2 --> " + b.to_str());

        int c = 2222;
        println("c in thread2 --> " + c.to_str());

        println("d1 in thread2 --> " + d1.to_str());
    });

    int d2 = 33333;

    println(Clover.output_to_str() {
        a = 3;

        int b = 333;
        println("b in thread3 --> " + b.to_str());

        int c = 3333;
        println("c in thread3 --> " + c.to_str());

        println("d1 in thread2 --> " + d1.to_str());
        println("d2 in thread3 --> " + d2.to_str());
    });
});

int b = 777;

println("b in main thread --> " + b.to_str());

println("HELLO THREAD end");

