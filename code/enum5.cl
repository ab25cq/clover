
print("enum X test1...");
Clover.assert(EnumTestX.ConstA == 0l && EnumTestX.ConstB == 1l);
println("TRUE");

EnumTestX g = new EnumTestX(7l);

print("enum X test2...");
Clover.assert(g == 7l);
println("TRUE");

