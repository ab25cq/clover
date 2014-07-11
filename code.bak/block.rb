
j = 0;
while(j < 3) do
    1.upto(5) do |i|
        puts "i --> " + i.to_s();
        break;
    end

    j = j + 1;
end

def method()
    puts "block start";

    1.upto(5) do |i|
        puts "i --> " + i.to_s();
        return;
    end

    puts "block end";
end

puts "method start";
method();
puts "method end";
