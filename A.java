class B {
    int value;

    B(int a) {
        this.value = a;
    }
}

class C {
    int value;

    C(int a) {
        this.value = a;
    }
}

public class A {
    public static void main(String[] args) {
        String a = "ABC";
        String b = "A" + "B";
        
        b = b + "C";

        if(a == b) {
            System.out.println("TRUE");
        }
        else {
            System.out.println("FALSE");
        }
    }
}
