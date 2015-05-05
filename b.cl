print("enum Test1...");
if(EnumA.ConstA == 0 && EnumA.ConstB == 1 && EnumA.ConstC == 2) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("enum Test2...");
if(EnumB.ConstA == 0 && EnumB.ConstB == 1 && EnumB.ConstC == 2 && EnumB.ConstD == 3 && EnumB.ConstE == 4) 
{
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("enum Test3...");
if(EnumB.toHash() == { "ConstA"=>0, "ConstB"=>1, "ConstC"=>2, "ConstD"=>3, "ConstE"=>4 })
{
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}
