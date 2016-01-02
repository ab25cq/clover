
MethodTest a = new MethodTest();

Method method = MethodTest->toClass().getMethodFromNameAndParametorTypes("method", { String, String});

print("method test...");
Clover.assert(method.class() == MethodTest && method.resultType() == int && method.name() == "method" && method.isPublicMethod() && method.parametors() == { String, String } && !method.blockExists());
println("TRUE");
