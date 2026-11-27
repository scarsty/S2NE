package org.s2ne.android;

public final class TouchSelectionTest {
    public static void main(String[] args) {
        TouchSelection input = new TouchSelection();
        check(!input.tap(20, 20, 8));
        check(input.tap(24, 23, 8));
        check(!input.tap(24, 23, 8));
        check(!input.tap(80, 20, 8));
        check(input.tap(80, 20, 8));
        check(!input.tap(80, 20, 8));
        input.clear();
        check(!input.tap(80, 20, 8));
        check(input.tap(80, 20, 8));
        System.out.println("PASS select, confirm, reselect, cancellation and confirmation reset");
    }

    private static void check(boolean condition) {
        if (!condition) { throw new AssertionError("Unexpected touch confirmation"); }
    }
}
