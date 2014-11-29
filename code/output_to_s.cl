
String a = Clover.outputToString() {
        Clover.println("あああ");
        Clover.println("いいい");
        Clover.println("ううう");
}


if(a.length() == 12)                   // including LF * 3
{
    Clover.println("output_to_str test....OK");
}
else {
    Clover.println("output_to_str test....FALSE");
}
