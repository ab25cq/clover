

enum EnumA {
    ConstA, ConstB, ConstC
}

class A {
    static void method(EnumA num) {
        System.out.println(num.toString());
    }
    public static void main(String[] args) {
        method(0);
    }
}
