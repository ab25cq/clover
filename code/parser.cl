Parser parser =  "あいうえお".toParser();

while(!parser.end()) {
    parser.getChar().toString().println();
    parser.forward();
}

Parser parser2 = "あいうえお".toParser();

parser2.forward();
print("Parser test...");
Clover.assert(parser2.getString(2) == "いう");
println("TRUE");

parser2.setPoint(3.toLong());

print("Parser test...");
Clover.assert(parser2.getChar() == 'え');
println("TRUE");

