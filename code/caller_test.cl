
{"A"=>1, "B"=>2, "C"=>3}.keys().each() {|String key|
    Clover.assert(caller.length() == 3);
    Clover.assert({ "A", "B", "C" }.include(key));
}
