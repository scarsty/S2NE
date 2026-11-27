package org.s2ne.android;

import android.Manifest;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.provider.Settings;
import java.io.File;

public final class LauncherActivity extends Activity {
    static final String GAME_DIRECTORY = "/sdcard/s2ne";
    private boolean requested;
    private boolean completed;

    @Override
    public void onCreate(Bundle state) {
        super.onCreate(state);
        requested = state != null && state.getBoolean("requested");
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (completed) { return; }
        if (hasStorageAccess()) {
            launchGame();
        } else if (!requested) {
            requested = true;
            if (Build.VERSION.SDK_INT >= 30) {
                startActivityForResult(new Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION,
                    Uri.parse("package:" + getPackageName())), 1);
            } else {
                requestPermissions(new String[] {Manifest.permission.READ_EXTERNAL_STORAGE,
                    Manifest.permission.WRITE_EXTERNAL_STORAGE}, 1);
            }
        }
    }

    @Override
    protected void onSaveInstanceState(Bundle state) {
        state.putBoolean("requested", requested);
        super.onSaveInstanceState(state);
    }

    @Override
    public void onRequestPermissionsResult(int request, String[] permissions, int[] results) {
        super.onRequestPermissionsResult(request, permissions, results);
        if (request == 1) { finishPermissionRequest(); }
    }

    @Override
    protected void onActivityResult(int request, int result, Intent data) {
        super.onActivityResult(request, result, data);
        if (request == 1) { finishPermissionRequest(); }
    }

    private void finishPermissionRequest() {
        if (completed) { return; }
        if (hasStorageAccess()) {
            launchGame();
        } else {
            showError("Storage access is required to read /sdcard/s2ne. Reopen S2NE to try again.");
        }
    }

    private boolean hasStorageAccess() {
        if (Build.VERSION.SDK_INT >= 30) {
            return Environment.isExternalStorageManager();
        }
        return checkSelfPermission(Manifest.permission.READ_EXTERNAL_STORAGE) == PackageManager.PERMISSION_GRANTED
            && checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE) == PackageManager.PERMISSION_GRANTED;
    }

    private void launchGame() {
        if (completed) { return; }
        for (String name : new String[] {"SUPERSP.PAL", "SUPERSP.BIN", "SUPERSP.CEL",
                "SUPERSP.PRG", "SUPERSP.DAT", "KINDSP.BIN", "ITEMSP.BIN", "ITEMHLSP.BIN",
                "chinese.ttf"}) {
            File file = new File(GAME_DIRECTORY, name);
            if (!file.isFile() || !file.canRead() || file.length() == 0) {
                showError("Missing or unreadable: " + file);
                return;
            }
        }
        completed = true;
        startActivity(new Intent(this, GameActivity.class));
        finish();
    }

    private void showError(String message) {
        completed = true;
        new AlertDialog.Builder(this).setTitle("S2NE").setMessage(message)
            .setPositiveButton(android.R.string.ok, (dialog, which) -> finish())
            .setOnCancelListener(dialog -> finish()).show();
    }
}
