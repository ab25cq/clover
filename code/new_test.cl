
NewTest a = new NewTest(1 ,2) { |int value|
    Clover.println("HELLO NEW TEST OBJECT");
    Clover.println("value --> " + value.toString());
}
