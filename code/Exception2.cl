Array<String> array = { "A", "B", "C" };

try {
    Clover.println(array[5]);
} catch(Exception e) {
    print("RangeException test...");
    if(e.class_name() == "RangeException") {
        println("TRUE");
    }
    else {
        println("FALSE");
        Clover.exit(1);
    }
}

