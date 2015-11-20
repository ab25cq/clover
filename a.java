class IntClass implements INumber {
    int value;

    IntClass(int value) {
        this.value = value;
    }

    public IntClass plus(IntClass value) {
        return new IntClass(this.value + value.value);
    }
}

interface INumber {
    INumber plus(INumber value);
}
