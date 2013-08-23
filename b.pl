open(IN, "ls -l|");

while($a = <IN>) {
    print $a;
}

print "AAA\n";

close(IN);
