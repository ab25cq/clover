MixinTestB a = new MixinTestB();

print("mixin test...");
Clover.assert(a.method1() == 1 && a.method2() == 2 && a.method3() == 3 && a.method4() == 4 && a.method5() == 5);
println("TRUE");
