using System;

delegate void ShowMessage();

class Person
{
    string name;

    public Person(string name) {
        this.name = name;
    }

    public void ShowName() {
        Console.Write("名前: {0}\n", this.name);
    }
};

class DelegateTest 
{
    static void Main() {
        Person p = new Person("鬼灯");

        ShowMessage show = p.ShowName;

        show();
    }
}
