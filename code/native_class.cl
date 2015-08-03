OnigurumaRegex a = new OnigurumaRegex("AAAA", false, false, false, Encoding.Utf8);

print("Native Class test...");
Clover.assert(a.isChild(NativeClass));
println("TRUE");
