int a = (true ? 1: 2);

Clover.println("a --> " + a.to_string());

if(a == 1) {
    Clover.println("? operator test 1....OK");
}
else {
    Clover.println("? operator test 1....FALSE");
}

int b = (false ? 1: 2);

Clover.println("b --> " + b.to_string());

if(b == 2) {
    Clover.println("? operator test 2....OK");
}
else {
    Clover.println("? operator test 2....FALSE");
}
