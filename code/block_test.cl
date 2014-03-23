void {
    Clover.println("HELLO BLOCK");
    Clover.println("I'm a block");
}

int a = int {
    Clover.println("HELLO BLOCK2");
    Clover.println("I'm a block with result");
    111;
}

if(a == 111) {
    Clover.println("Result test...OK");
}
else {
    Clover.println("Result test...FALSE");
    Clover.exit(2);
}
