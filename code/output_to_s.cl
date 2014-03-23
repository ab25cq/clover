
String a = Clover.output_to_s() {
        Clover.println("あああ");
        Clover.println("いいい");
        Clover.println("ううう");
}


if(a.length() == 12)                   // including LF * 3
{
    Clover.println("output_to_s test....OK");
}
else {
    Clover.println("output_to_s test....FALSE");
}
