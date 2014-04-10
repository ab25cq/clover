/// logical denial ///

int a = 0;
bool b = false;
bool c = true;
bool d = !c;

Clover.println("b --> " + b.to_s());
Clover.println("c --> " + c.to_s());
Clover.println("d --> " + d.to_s());
Clover.println("!!!d --> " + (!!!d).to_s());

Clover.print("Logical denial Test...");
if(b == false && c == true && d == false && !!!d == true) {
    Clover.println("OK");
}
else {
    Clover.println("FALSE");
    Clover.exit(2);
}

/// complement ///
int e = 11;

Clover.println("~11 --> " + (~e).to_s());
Clover.println("~12 --> " + (~12).to_s());

Clover.print("Complement Test...");
if(~e == -12 && ~12 == -13) {
    Clover.println("OK");
}
else {
    Clover.println("FALSE");
    Clover.exit(2);
}

/// int ///
Clover.println("1 + 1 -->" + (1 + 1).to_s());
Clover.println("1 - 1 -->" + (1 - 1).to_s());
Clover.println("4 * 3 -->" + (4 * 3).to_s());
Clover.println("6 / 2 -->" + (6 / 2).to_s());
Clover.println("6 % 5 -->" + (6 % 5).to_s());
Clover.println("1 << 3 -->" + (1 << 3).to_s());
Clover.println("8 >> 2 -->" + (8 >> 2).to_s());
Clover.println("8 > 2 -->" + (8 > 2).to_s());
Clover.println("8 < 2 -->" + (8 < 2).to_s());

Clover.print("int Test1...");
if((1+1) == 2 && (1 -1 ) == 0 && (4 * 3) == 12 && (6/2) == 3 && (6%5) == 1 && (1 << 3) == 8 && (8 >> 2) == 2 && (8 > 2) == true && (8 < 2) == false) 
{
    Clover.println("OK");
}
else 
{
    Clover.println("FALSE");
    Clover.exit(2);
}

Clover.println("2 >= 2 -->" + (2 >= 2).to_s());
Clover.println("1 >= 2 -->" + (1 >= 2).to_s());
Clover.println("3 >= 2 -->" + (3 >= 2).to_s());
Clover.println("8 <= 2 -->" + (8 <= 2).to_s());
Clover.println("1 <= 2 -->" + (1 <= 2).to_s());
Clover.println("2 == 2 -->" + (2 == 2).to_s());
Clover.println("2 == 3 -->" + (2 == 3).to_s());
Clover.println("2 != 2 -->" + (2 != 2).to_s());
Clover.println("2 != 3 -->" + (2 != 3).to_s());

Clover.print("int Test2...");
if((2>=2) == true && (1>=2) == false && (3>=2) == true && (8<=2) == false && (1<=2) == true && (2==2) == true && (2==3) == false && (2!=2) == false && (2!=3) == true) 
{
    Clover.println("OK");
}
else 
{
    Clover.println("FALSE");
    Clover.exit(2);
}

Clover.println("1 & 1  -->" + (1 & 1).to_s());
Clover.println("0 & 1  -->" + (0 & 1).to_s());
Clover.println("1|2|4  -->" + (1 | 2 | 4).to_s());
Clover.println("1 ^ 1  -->" + (1 ^ 1).to_s());
Clover.println("1 ^ 0  -->" + (1 ^ 0).to_s());
Clover.println("0x0f   -->" + 0x0f.to_s());
Clover.println("017    -->" + 017.to_s());

Clover.print("int Test3...");
if((1&1) == 1 && (0 & 1) == 0 && (1|2|4) == 7 && (1^1) == 0 && (1^0) == 1 && 0x0f == 15 && 017 == 15) 
{
    Clover.println("OK");
}
else 
{
    Clover.println("FALSE");
    Clover.exit(2);
}

Clover.println("true && true -->" + (true && true).to_s());
Clover.println("false && true -->" + (false && true).to_s());
Clover.println("false && false -->" + (false && false).to_s());
Clover.println("true || true -->" + (true || true).to_s());
Clover.println("false || true -->" + (false || true).to_s());
Clover.println("false || false -->" + (false || false).to_s());
Clover.println("true || true -->" + (true || true).to_s());

Clover.print("logical operator Test...");
if((true && true) == true && (false && true) == false && (false && false) == false && (true || true) == true && (false || true) == true && (false || true) == true && (false || false) == false && (true || true) == true) 
{
    Clover.println("OK");
}
else 
{
    Clover.println("FALSE");
    Clover.exit(2);
}

/// float ///
float fvalue1;
float fvalue2;
float an_error_range;

fvalue1 = 1.1 + 1.2;
Clover.println("1.1 + 1.2 -->" + fvalue1.to_s());

an_error_range = fvalue1 - 2.3;

Clover.print("float Test1...");
if(an_error_range < 0.1) {
    Clover.println("OK");
}
else {
    Clover.println("FALSE");
    Clover.exit(2);
}

fvalue1 = 1.1 * 2.0;
Clover.println("1.1 * 2.0 -->" + (1.1 * 2.0).to_s());

an_error_range = fvalue1 - 2.2;

Clover.print("float Test2...");
if(an_error_range < 0.1) {
    Clover.println("OK");
}
else {
    Clover.println("FALSE");
    Clover.exit(2);
}

fvalue1 = 4.4 / 2.0;
Clover.println("4.4 / 2.0 -->" + (4.4 / 2.0).to_s());

an_error_range = fvalue1 - 2.2;

Clover.print("float Test3...");
if(an_error_range < 0.1) {
    Clover.println("OK");
}
else {
    Clover.println("FALSE");
    Clover.exit(2);
}

Clover.println("8.0 > 2.0 -->" + (8.0 > 2.0).to_s());
Clover.println("8.0 < 2.0 -->" + (8.0 < 2.0).to_s());
Clover.println("2.1 >= 2.0 -->" + (2.1 >= 2.0).to_s());
Clover.println("1.1 >= 2.0 -->" + (1.1 >= 2.0).to_s());
Clover.println("3.1 >= 2.0 -->" + (3.1 >= 2.0).to_s());
Clover.println("8.1 <= 2.0 -->" + (8.1 <= 2.0).to_s());
Clover.println("1.1 <= 2.0 -->" + (1.1 <= 2.0).to_s());
Clover.println("2.1 == 2.1 -->" + (2.1 == 2.1).to_s());
Clover.println("2.1 == 3.1 -->" + (2.1 == 3.1).to_s());
Clover.println("2.1 != 2.1 -->" + (2.1 != 2.1).to_s());
Clover.println("2.1 != 3.1 -->" + (2.1 != 3.1).to_s());

Clover.print("float Test4...");
if((8.0 > 2.0) == true && (8.0 < 2.0) == false && (2.1 >= 2.0) == true && (1.1 >= 2.0) == false && (3.1 >= 2.0) == true && (8.1 <= 2.0) == false && (1.1 <= 2.0) == true && (2.1 < 2.1) == false && (2.1 == 3.1) == false && (2.1 != 2.1) == false && (2.1 != 3.1) == true) 
{
    Clover.println("OK");
}
else 
{
    Clover.println("FALSE");
    Clover.exit(2);
}

/// substitution ///
int g = 123;
Clover.println("g      -->" + g.to_s());

Clover.print("substitution Test1...");
if(g == 123) 
{
    Clover.println("OK");
}
else 
{
    Clover.println("FALSE");
    Clover.exit(2);
}

g++;
Clover.println("g++    -->" + g.to_s());

Clover.print("substitution Test2...");
if(g == 124) 
{
    Clover.println("OK");
}
else 
{
    Clover.println("FALSE");
    Clover.exit(2);
}

g+=123;
Clover.println("g+=123 -->" + g.to_s());

Clover.print("substitution Test3...");
if(g == 247) 
{
    Clover.println("OK");
}
else 
{
    Clover.println("FALSE");
    Clover.exit(2);
}

g-=123;
Clover.println("g-=123 -->" + g.to_s());

Clover.print("substitution Test4...");
if(g == 124) 
{
    Clover.println("OK");
}
else 
{
    Clover.println("FALSE");
    Clover.exit(2);
}

/// field substitution ///

FieldTest h = new FieldTest();

h.field1 = 123;
Clover.println("h.field1               --> " + h.field1.to_s());

Clover.print("field substitution Test...");
if(h.field1 == 123) 
{
    Clover.println("OK");
}
else 
{
    Clover.println("FALSE");
    Clover.exit(2);
}

h.field1++;
Clover.println("h.field1++             --> " + h.field1.to_s());

Clover.print("field substitution Test2...");
if(h.field1 == 124) 
{
    Clover.println("OK");
}
else 
{
    Clover.println("FALSE");
    Clover.exit(2);
}

h.field1--;
Clover.println("h.field1--             --> " + h.field1.to_s());

Clover.print("field substitution Test3...");
if(h.field1 == 123) 
{
    Clover.println("OK");
}
else 
{
    Clover.println("FALSE");
    Clover.exit(2);
}

h.field1 += 123;
Clover.println("h.field1+=123          --> " + h.field1.to_s());

Clover.print("field substitution Test4...");
if(h.field1 == 246) 
{
    Clover.println("OK");
}
else 
{
    Clover.println("FALSE");
    Clover.exit(2);
}

h.field1 -= 123;
Clover.println("h.field1-=123          --> " + h.field1.to_s());

Clover.print("field substitution Test5...");
if(h.field1 == 123) 
{
    Clover.println("OK");
}
else 
{
    Clover.println("FALSE");
    Clover.exit(2);
}

FieldTest.static_field = 123;
Clover.println("FieldTest.static_field       --> " + FieldTest.static_field.to_s());

Clover.print("static field substitution Test1...");
if(FieldTest.static_field == 123) 
{
    Clover.println("OK");
}
else 
{
    Clover.println("FALSE");
    Clover.exit(2);
}

FieldTest.static_field++;
Clover.println("FieldTest.static_field++     --> " + FieldTest.static_field.to_s());

Clover.print("static field substitution Test3...");
if(FieldTest.static_field == 124)
{
    Clover.println("OK");
}
else 
{
    Clover.println("FALSE");
    Clover.exit(2);
}

FieldTest.static_field += 123;
Clover.println("FieldTest.static_field+=123  --> " + FieldTest.static_field.to_s());

Clover.print("static field substitution Test4...");
if(FieldTest.static_field == 247) 
{
    Clover.println("OK");
}
else 
{
    Clover.println("FALSE");
    Clover.exit(2);
}

FieldTest.static_field -= 123;
Clover.println("FieldTest.static_field-=123  --> " + FieldTest.static_field.to_s());

Clover.print("static field substitution Test5...");
if(FieldTest.static_field == 124) 
{
    Clover.println("OK");
}
else 
{
    Clover.println("FALSE");
    Clover.exit(2);
}

/// comma ///

int i = 1, String j = "aaa", int k = 2;
Clover.println("i --> " + i.to_s());
Clover.println("j --> " + j);
Clover.println("k --> " + k.to_s());

Clover.print("comman test...");
if(i == 1 && j == "aaa" && k == 2) 
{
    Clover.println("OK");
}
else 
{
    Clover.println("FALSE");
    Clover.exit(2);
}

