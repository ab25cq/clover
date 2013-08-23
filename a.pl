open(OUT, "|less");

print OUT "AAA\n";
print OUT "BBB\n";

print "Hello World\n";
sleep(3);

close(OUT);
