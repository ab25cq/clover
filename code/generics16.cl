
GenericsTest16_A<GenericsTest16_B> b = new GenericsTest16_A<GenericsTest16_B>(new GenericsTest16_B(345));

print("GenericsTest16 test2...");
if(b.getField1().getField() == 345 && b.getField2() == 345) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

