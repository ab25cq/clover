ExceptionTestClassA a = new ExceptionTestClassA();

int b = 123;
int c = 234;

print("exception test1...");
try {
    int d = 356;

    a.method();
} catch(Exception e) {
    int g = 123;
    int h = 234;

    if(e.type() == Exception && g == 123 && h == 234) {
        println("TRUE");
    }
    else {
        println("FALSE");
        System.exit(2);
    }
}
