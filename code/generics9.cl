Generics9TestClassC<int, String> a = new Generics9TestClassC<int, String>(100, "AAA");
Generics9TestOperator<String, int, Generics9TestClassA<int, String>, int> b = new Generics9TestOperator<String, int, Generics9TestClassA<int, String>, int>();

b.call_method(new Generics9TestClassC<String, int>(), 100); // no error

