class Point {
    private int mX;
    private int mY;

    Point() {
        self.mX = 0;
        self.mY = 0;
    }

    Point(int x, int y) {
        self.mX = x;
        self.mY = y;
    }

    Point operator + (Point value2) {
        Point result = new Point();

        result.mX = self.mX + value2.mX;
        result.mY = self.mY + value2.mY;

        return result;
    }

    void show() {
        Clover.println("(x, y) == (" + self.mX.to_s() + "," + self.mY.to_s() + ")");
    }
}
