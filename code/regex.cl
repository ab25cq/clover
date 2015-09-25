Regex a = new OnigurumaRegex("");

print("regex test...");
Clover.assert("AAAA" =~ r//);
println("TRUE");

print("regex test2...");
Clover.assert("AAA" =~ a);
println("TRUE");
