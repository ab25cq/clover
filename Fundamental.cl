
class Clover {
    native static void gc();
    native static void print(String string);
    native static void compile(String string);
    native static void load(String fileName);
    native static void show_classes();
}

class void {
}

class int {
    native String to_s();
}

class float {
    native int floor();
}

class Object {
    native String class_name();
    native void show_class();
}

class String extends Object {
    native String();

    native int length();
}

class Array<T> extends Object {
    native Array();

    native void add(T item);
    native T items(int index);
    native int length();
    
    T operator[] (int index) {
        return self.items(index);
    }
}

class Hash<T> extends Object {
}
