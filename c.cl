
int a = 123;

println("BBB");
Thread thread = new Thread() {
    a = 777;
    int b = 456;
    int c = 789;
    println("AAA");
}
println("CCC");

int b = 456;
int c = 789;

thread.join();
