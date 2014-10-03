Generics9TestClassC<int, String> a = new Generics9TestClassC<int, String>(100, "AAA");
Operator<String, int, Generics9TestClassA<int, String>, int> b = new Operator<String, int, Generics9TestClassA<int, String>, int>();

b.call_method(new Generics9TestClassC<String, int>(), 100); // no error

