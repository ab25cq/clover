
class ClassA {
    static int fieldA = 123;

    static void methodA() {
        System.out.println("ClassA.methodA");
    }
}

class ClassB extends ClassA {
    static int fieldB = 234;

    static void methodB() {
        System.out.println("ClassB.methodB");
    }
}

class A {
    public static void main(String[] args) {
        System.out.println("Test...");
        if(ClassA.fieldA == 123 && ClassB.fieldB == 234 && ClassB.fieldA == 123) {
            System.out.println("TRUE");
        }
        else {
            System.out.println("FALSE");
        }

        ClassB.methodA();
    }
}
