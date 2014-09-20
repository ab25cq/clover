
Operator<Integer> operator = new Operator<Integer>();

Integer a = new Integer(112);
Integer b = new Integer(222);

Integer c = operator.call_plus_method(a, b);

println("c --> " + c.value.to_string());

