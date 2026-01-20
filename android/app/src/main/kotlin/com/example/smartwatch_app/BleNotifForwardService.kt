package com.example.smartwatch_app

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.Service
import android.content.Intent
import android.os.Build
import android.os.IBinder
import android.util.Log
import io.flutter.embedding.engine.FlutterEngineCache
import io.flutter.plugin.common.MethodChannel

class BleNotifForwardService : Service() {

    companion object {
        private const val TAG = "BleNotifForward"
        const val ACTION_SEND = "com.example.smartwatch_app.ACTION_SEND_NOTIF"
        const val EXTRA_JSON = "json"

        private const val CHANNEL_ID = "ble_notif_channel"
        private const val NOTIFICATION_ID = 1001
        private const val FLUTTER_CHANNEL = "com.example.smartwatch_app/ble_notif"
    }

    override fun onCreate() {
        super.onCreate()
        createNotificationChannel()
        startForeground(NOTIFICATION_ID, buildNotification())
        Log.d(TAG, "Service CREATED")
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        if (intent?.action == ACTION_SEND) {
            val json = intent.getStringExtra(EXTRA_JSON)
            if (json != null) {
                Log.d(TAG, "Forwarding to Flutter: $json")
                sendToFlutter(json)
            }
        }
        return START_STICKY
    }

    private fun sendToFlutter(json: String) {
        try {
            val engine = FlutterEngineCache.getInstance().get("main_engine")
            if (engine != null) {
                MethodChannel(engine.dartExecutor.binaryMessenger, FLUTTER_CHANNEL)
                    .invokeMethod("onNotification", json)
                Log.d(TAG, "Sent to Flutter OK")
            } else {
                Log.w(TAG, "FlutterEngine not cached - notification not sent")
            }
        } catch (e: Exception) {
            Log.e(TAG, "Failed to send to Flutter", e)
        }
    }

    private fun createNotificationChannel() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val channel = NotificationChannel(
                CHANNEL_ID,
                "BLE Notification Service",
                NotificationManager.IMPORTANCE_LOW
            ).apply {
                description = "Forwards notifications to smartwatch"
            }
            val manager = getSystemService(NotificationManager::class.java)
            manager?.createNotificationChannel(channel)
        }
    }

    private fun buildNotification(): Notification {
        val builder = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            Notification.Builder(this, CHANNEL_ID)
        } else {
            @Suppress("DEPRECATION")
            Notification.Builder(this)
        }

        return builder
            .setContentTitle("SmartWatch")
            .setContentText("Listening for notifications...")
            .setSmallIcon(android.R.drawable.stat_sys_data_bluetooth)
            .setOngoing(true)
            .build()
    }

    override fun onBind(intent: Intent?): IBinder? = null

    override fun onDestroy() {
        super.onDestroy()
        Log.d(TAG, "Service DESTROYED")
    }
}