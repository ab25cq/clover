Number<int> number = new Number<int>(100);
Number2<float, int> number2 = new Number2<float, int>(200.0, 1);
Number3<float, float, int> number3 = new Number3<float, float, int>(200.0, 210.0, 3);

println("number.plus(number2.value) -->" + number.plus(5).to_string());
println("number.plus(number2.value) -->" + number.plus(number2.value2).to_string());
println("number.plus(number2.value) -->" + number.plus(number3.get_value3()).to_string());


