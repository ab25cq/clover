Number<int> number = new Number<int>(100);
Number2<float, int> number2 = new Number2<float, int>(200.0f, 1);
Number3<float, float, int> number3 = new Number3<float, float, int>(200.0f, 210.0f, 3);

println("number.plus(number2.value) -->" + number.plus(5).toString());
println("number.plus(number2.value) -->" + number.plus(number2.value2).toString());
println("number.plus(number2.value) -->" + number.plus(number3.get_value3()).toString());


