print("enum Test1...");
if(EnumA.toHash() == { "ConstA"=>0, "ConstB"=>1, "ConstC"=>2})
{
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

print("enum Test2...");
if(EnumB.toHash() == { "ConstA"=>0, "ConstB"=>1, "ConstC"=>2, "ConstD"=>3, "ConstE"=>4 })
{
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}
