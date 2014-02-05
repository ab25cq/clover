
class void {
}

class int {
    native String to_s();
}

class float {
    native int floor();
}

class Object {
    native void show_class();
}

class String extends Object {
    native int length();
}

class Clover extends Object {
    native static void gc();
    native static void print(String string);
    native static void compile(String string);
    native static void load(String fileName);
    native static void show_classes();
    native static void show_heap();
}

class Array<T> extends Object {
    native Array();

    native void add(T object);
    native T get(int index);
}

class Hash<T> extends Object {
}
