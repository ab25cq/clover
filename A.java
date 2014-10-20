
interface IC {
    int getField1();
}

class TestD<T extends Object&IC> {
}


class TestB<T extends Object&IC> extends TestD<T> {
}
