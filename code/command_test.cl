
print("Command test1...");
Clover.assert(Command.echo("ABC\nDEF\nGHI").grep("A").toString() == "ABC\n");
println("TRUE");


print("Command test2...");
Clover.assert("ABC\nDEF\nGHI\n".toCommand().grep("A").toString() == "ABC\n");
println("TRUE");


