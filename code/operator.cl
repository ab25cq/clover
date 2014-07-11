
Point a = new Point(1, 2);
Point b = new Point(2, 3);
Point c = a + b;

String d = Clover.output_to_string() {
    c.show();
}

print("OperatorMethod Test...");
if(d == "(x, y) == (3,5)\n") {
    println("OK");
}
else {
    println("FLASE");
    System.exit(2);
}

