
int a = 0;

println(Clover.output_toString() {
    a = 1;

    int b = 111;
    println("b in thread1 --> " + b.toString());

    int c = 1111;
    println("c in thread1 --> " + c.toString());

    int d1 = 33;

    println(Clover.output_toString() {
        a = 2;

        int b = 222;
        println("b in thread2 --> " + b.toString());

        int c = 2222;
        println("c in thread2 --> " + c.toString());

        println("d1 in thread2 --> " + d1.toString());
    });

    int d2 = 33333;

    println(Clover.output_toString() {
        a = 3;

        int b = 333;
        println("b in thread3 --> " + b.toString());

        int c = 3333;
        println("c in thread3 --> " + c.toString());

        println("d1 in thread2 --> " + d1.toString());
        println("d2 in thread3 --> " + d2.toString());
    });
});

int b = 777;

println("b in main thread --> " + b.toString());

println("HELLO THREAD end");

