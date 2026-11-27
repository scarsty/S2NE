package org.s2ne.android;

final class TouchSelection {
    private boolean selected;
    private float x;
    private float y;

    boolean tap(float nextX, float nextY, float tolerance) {
        float dx = nextX - x;
        float dy = nextY - y;
        if (selected && dx * dx + dy * dy <= tolerance * tolerance) {
            clear();
            return true;
        }
        selected = true;
        x = nextX;
        y = nextY;
        return false;
    }

    void clear() { selected = false; }
}
