
// for generics types
class anonymous0 {}
class anonymous1 {}
class anonymous2 {}
class anonymous3 {}
class anonymous4 {}
class anonymous5 {}
class anonymous6 {}
class anonymous7 {}

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
}

class Hash<T> extends Object {
}
