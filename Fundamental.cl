/*
class Object {
    native static void show();
}
*/


class void {
}

class int {
    native String to_s();
}

class float {
    native int floor();
}

class String {
    native int length();
}

class Clover {
    native static void gc();

    native static void print(String string);
    
    native static void compile(String string);

    native static void load(String fileName);

    native static void show_classes();

    native static void show_heap();
}

