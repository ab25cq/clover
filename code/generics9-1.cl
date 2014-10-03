
Generics9_1TestClassB<int, String> a = new Generics9_1TestClassB<int, String>(777, "AAA");

Operator2<int, String, Generics9_1TestClassA<String, int>> b = new Operator2<int, String, Generics9_1TestClassA<String, int>>();

b.call_method(a, "AAA");

