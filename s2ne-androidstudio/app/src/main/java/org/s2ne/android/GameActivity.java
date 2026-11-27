package org.s2ne.android;

import android.content.pm.ActivityInfo;
import android.view.InputDevice;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewConfiguration;
import org.libsdl.app.SDLActivity;

public final class GameActivity extends SDLActivity {
    private boolean rightClickGesture;
    private boolean rightClickCancelled;
    private final TouchSelection selection = new TouchSelection();
    private float downX;
    private float downY;
    private boolean touchCancelled;

    @Override
    public boolean dispatchTouchEvent(MotionEvent event) {
        if (!event.isFromSource(InputDevice.SOURCE_TOUCHSCREEN)) {
            selection.clear();
            return super.dispatchTouchEvent(event);
        }
        int action = event.getActionMasked();
        if (action == MotionEvent.ACTION_DOWN) {
            View surface = getWindow().getDecorView();
            rightClickGesture = event.getX() >= surface.getWidth() * 0.8f
                && event.getY() >= surface.getHeight() * 0.8f;
            rightClickCancelled = false;
            downX = event.getX();
            downY = event.getY();
            touchCancelled = false;
        }
        if (!rightClickGesture) {
            int slop = ViewConfiguration.get(this).getScaledTouchSlop();
            float dx = event.getX() - downX;
            float dy = event.getY() - downY;
            if (action == MotionEvent.ACTION_POINTER_DOWN || dx * dx + dy * dy > slop * slop) {
                touchCancelled = true;
                selection.clear();
            }
            if (action == MotionEvent.ACTION_DOWN || action == MotionEvent.ACTION_MOVE) {
                moveCursor(event);
            } else if (action == MotionEvent.ACTION_UP && !touchCancelled) {
                moveCursor(event);
                if (selection.tap(event.getX(), event.getY(), slop)) {
                    onNativeMouse(MotionEvent.BUTTON_PRIMARY, MotionEvent.ACTION_DOWN, 0, 0, true);
                    onNativeMouse(0, MotionEvent.ACTION_UP, 0, 0, true);
                }
            } else if (action == MotionEvent.ACTION_CANCEL) {
                selection.clear();
            }
            return true;
        }
        selection.clear();
        if (action == MotionEvent.ACTION_POINTER_DOWN) { rightClickCancelled = true; }
        if (action == MotionEvent.ACTION_UP) {
            if (!rightClickCancelled) {
                onNativeMouse(MotionEvent.BUTTON_SECONDARY, MotionEvent.ACTION_DOWN, 0, 0, true);
                onNativeMouse(0, MotionEvent.ACTION_UP, 0, 0, true);
            }
            rightClickGesture = false;
        } else if (action == MotionEvent.ACTION_CANCEL) {
            rightClickGesture = false;
        }
        return true;
    }

    private void moveCursor(MotionEvent event) {
        int[] location = new int[2];
        mSurface.getLocationOnScreen(location);
        onNativeMouse(0, MotionEvent.ACTION_MOVE,
            event.getRawX() - location[0], event.getRawY() - location[1], false);
    }

    @Override
    protected void onPause() {
        selection.clear();
        rightClickGesture = false;
        super.onPause();
    }

    @Override
    public void setOrientationBis(int width, int height, boolean resizable, String hint) {
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
    }

    @Override
    protected String[] getLibraries() {
        return new String[] {"SDL3", "main"};
    }

    @Override
    protected String[] getArguments() {
        return new String[] {LauncherActivity.GAME_DIRECTORY};
    }
}
