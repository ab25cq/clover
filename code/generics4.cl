
Operator<Integer2> operator = new Operator<Integer2>();

Integer2 a = new Integer2(112);
Integer2 b = new Integer2(222);

Integer2 c = operator.call_plus_method(a, b);

println("c --> " + c.value.to_string());

