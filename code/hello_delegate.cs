using System;

delegate void SomeDelegate(int a);

class MyClass {
    public static void A(int n) {
        Console.Write("MyClass::A({0})が呼ばれました。\n", n);
    }
}

class DelegateTest
{
    static void Main() {
        SomeDelegate a = A;

        a(256);
    }

    static void A(int n) {
        Console.Write("A({0})が呼ばれました。\n", n);
    }
}
