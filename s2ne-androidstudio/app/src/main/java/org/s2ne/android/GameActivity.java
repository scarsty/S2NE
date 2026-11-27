package org.s2ne.android;

import android.content.pm.ActivityInfo;
import android.view.InputDevice;
import android.view.MotionEvent;
import android.view.View;
import org.libsdl.app.SDLActivity;

public final class GameActivity extends SDLActivity {
    private boolean rightClickGesture;
    private boolean rightClickCancelled;

    @Override
    public boolean dispatchTouchEvent(MotionEvent event) {
        if (!event.isFromSource(InputDevice.SOURCE_TOUCHSCREEN)) {
            return super.dispatchTouchEvent(event);
        }
        int action = event.getActionMasked();
        if (action == MotionEvent.ACTION_DOWN) {
            View surface = getWindow().getDecorView();
            rightClickGesture = event.getX() >= surface.getWidth() * 0.8f
                && event.getY() >= surface.getHeight() * 0.8f;
            rightClickCancelled = false;
        }
        if (!rightClickGesture) { return super.dispatchTouchEvent(event); }
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
