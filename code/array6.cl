Array<int> a = { 1, 2, 3 };

a[5] = 6;

print("array6-test1...");
if(a == { 1,2,3,null,null,6 }) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

Array<int> b = { 4, 5, 6 };

b.fill(1);

print("array6-test2...");
if(b == { 1, 1, 1 }) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

Array<int> c = { 4, 5, 6 };

c.fill(1, 1..3);

print("array6-test3...");
if(c == { 4, 1, 1 }) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

Array<int> d = { 4, 5, 6 };

d.fill(1, 3..1);

print("array6-test4...");
if(d == { 4, 1, 1 }) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

Array<int> e = { 4, 5, 6 };

e.fill(1, 2..6);

print("array6-test5...");
if(e == { 4, 5, 1, 1, 1, 1}) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

Array<int> f = { 4, 5, 6 };

f.fill(1, 6..2);

print("array6-test6...");
if(f == { 4, 5, 1, 1, 1, 1}) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

Array<int> g = { 0, 1, 2 };

int obj = null;

g.insert(1, obj);

print("array6-test7...");
if(g == { 0, null, 1, 2 }) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

Array<int> h = { 0, 1, 2 };

h.insert(5, 5);

print("array6-test8...");
if(h == { 0, 1, 2, null, null, 5 }) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

Array<int> i = { 0, 1, 2 };

i.insert(5, {5,6,7});

print("array6-test9...");
if(i == { 0, 1, 2, null, null, 5 , 6, 7}) {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}

