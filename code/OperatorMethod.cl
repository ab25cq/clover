
Point a = new Point(1, 2);
Point b = new Point(2, 3);
Point c = a + b;

String d = Clover.output_to_s() {
    c.show();
}

Clover.print("OperatorMethod Test...");
if(d == "(x, y) == (3,5)\n") {
    Clover.println("OK");
}
else {
    Clover.println("FLASE");
    Clover.exit(2);
}
