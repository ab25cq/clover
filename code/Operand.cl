/// logical denial ///

int a = 0;
bool b = false;
bool c = true;
bool d = !c;

Clover.println("b --> " + b.to_s());
Clover.println("c --> " + c.to_s());
Clover.println("d --> " + d.to_s());
Clover.println("!!!d --> " + (!!!d).to_s());

/// complement ///
int e = 11;

Clover.println("~11 --> " + (~e).to_s());
Clover.println("~12 --> " + (~12).to_s());

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
Clover.println("2 >= 2 -->" + (2 >= 2).to_s());
Clover.println("1 >= 2 -->" + (1 >= 2).to_s());
Clover.println("3 >= 2 -->" + (3 >= 2).to_s());
Clover.println("8 <= 2 -->" + (8 <= 2).to_s());
Clover.println("1 <= 2 -->" + (1 <= 2).to_s());
Clover.println("2 == 2 -->" + (2 == 2).to_s());
Clover.println("2 == 3 -->" + (2 == 3).to_s());
Clover.println("2 != 2 -->" + (2 != 2).to_s());
Clover.println("2 != 3 -->" + (2 != 3).to_s());
Clover.println("1 & 1  -->" + (1 & 1).to_s());
Clover.println("0 & 1  -->" + (0 & 1).to_s());
Clover.println("1|2|4  -->" + (1 | 2 | 4).to_s());
Clover.println("1 ^ 1  -->" + (1 ^ 1).to_s());
Clover.println("1 ^ 0  -->" + (1 ^ 0).to_s());
Clover.println("0x0f   -->" + 0x0f.to_s());
Clover.println("017    -->" + 017.to_s());
Clover.println("true && true -->" + (true && true).to_s());
Clover.println("false && true -->" + (false && true).to_s());
Clover.println("false && false -->" + (false && false).to_s());
Clover.println("true || true -->" + (true || true).to_s());
Clover.println("false || true -->" + (false || true).to_s());
Clover.println("false || false -->" + (false || false).to_s());
Clover.println("true || true -->" + (true || true).to_s());

/// float ///
float f = 1.1 + 1.2;

Clover.println("1.1 + 1.2 -->" + f.to_s());
Clover.println("1.1 - 2.0 -->" + (1.1 - 2.0).to_s());
Clover.println("1.1 * 2.0 -->" + (1.1 * 2.0).to_s());
Clover.println("4.4 / 2.0 -->" + (4.4 / 2.0).to_s());
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

/// substitution ///
int g = 123;
Clover.println("g      -->" + g.to_s());

g++;
Clover.println("g++    -->" + g.to_s());

g+=123;
Clover.println("g+=123 -->" + g.to_s());

g-=123;
Clover.println("g-=123 -->" + g.to_s());

/// field substitution ///
Clover.load("FieldTest");

FieldTest h = new FieldTest();

h.field1 = 123;
Clover.println("h.field1               --> " + h.field1.to_s());

h.field1++;
Clover.println("h.field1++             --> " + h.field1.to_s());

h.field1--;
Clover.println("h.field1--             --> " + h.field1.to_s());

h.field1 += 123;
Clover.println("h.field1+=123          --> " + h.field1.to_s());

h.field1 -= 123;
Clover.println("h.field1-=123          --> " + h.field1.to_s());

FieldTest.static_field = 123;
Clover.println("FieldTest.static_field       --> " + FieldTest.static_field.to_s());

FieldTest.static_field++;
Clover.println("FieldTest.static_field++     --> " + FieldTest.static_field.to_s());

FieldTest.static_field += 123;
Clover.println("FieldTest.static_field+=123  --> " + FieldTest.static_field.to_s());

FieldTest.static_field -= 123;
Clover.println("FieldTest.static_field-=123  --> " + FieldTest.static_field.to_s());

/// comma ///

int i = 1, String j = "aaa", int k = 2;
Clover.println("i --> " + i.to_s());
Clover.println("j --> " + j);
Clover.println("k --> " + k.to_s());

