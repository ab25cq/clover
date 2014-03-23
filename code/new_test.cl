Clover.load("NewTest");

NewTest a = new NewTest(1 ,2) { |int value|
    Clover.println("HELLO NEW TEST OBJECT");
    Clover.println("self.value1 --> " + self.value1.to_s());
    Clover.println("self.value2 --> " + self.value2.to_s());
    Clover.println("value --> " + value.to_s());
}
