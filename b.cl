StringBuffer a = new StringBuffer();

a.append("あ").append("い").append("う");

print("StringBuffer test...");
Clover.assert(a.toString() == "あいう");
println("TRUE");

