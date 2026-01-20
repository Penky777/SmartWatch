package com.example.smartwatch_app

import android.content.ComponentName
import android.content.Context
import android.os.Build
import android.os.IBinder
import android.service.notification.NotificationListenerService
import android.service.notification.StatusBarNotification
import android.util.Log
import android.content.Intent
import org.json.JSONObject

class WatchNotificationListenerService : NotificationListenerService() {

    companion object {
        private const val TAG = "WatchNotifListener"

        fun requestRebind(context: Context) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
                val cn = ComponentName(context, WatchNotificationListenerService::class.java)
                requestRebind(cn)
                Log.d(TAG, "Rebind requested")
            }
        }
    }

    override fun onBind(intent: Intent?): IBinder? {
        Log.d(TAG, "Service BOUND")
        return super.onBind(intent)
    }

    override fun onListenerConnected() {
        super.onListenerConnected()
        Log.d(TAG, "Listener CONNECTED")
    }

    override fun onListenerDisconnected() {
        super.onListenerDisconnected()
        Log.d(TAG, "Listener DISCONNECTED - requesting rebind")
        requestRebind(this)
    }

    override fun onNotificationPosted(sbn: StatusBarNotification?) {
        sbn ?: return

        val pkg = sbn.packageName ?: return
        val extras = sbn.notification?.extras ?: return

        // Ignoruj vlastné notifikácie
        if (pkg == "com.example.smartwatch_app") return

        val title = extras.getCharSequence("android.title")?.toString()?.trim() ?: ""
        val text = extras.getCharSequence("android.text")?.toString()?.trim() ?: ""

        // Filter pre Messenger spam
        if (pkg == "com.facebook.orca") {
            val lt = title.lowercase()
            val lx = text.lowercase()

            if (lt.contains("chat heads")) return
            if (lx == "start a conversation") return
            if (lx.contains("checking for new messages")) return
            if (lx.contains("connecting")) return
            if (lx.contains("updating")) return
            if (lt == "messenger" && text.isEmpty()) return
        }

        if (title.isEmpty() && text.isEmpty()) return

        val payload = mapOf(
            "type" to "notification",
            "app" to pkg,
            "title" to title,
            "text" to text,
            "ts" to System.currentTimeMillis()
        )

        try {
            val json = JSONObject(payload).toString()
            Log.d(TAG, "Notification: $json")

            val intent = Intent(this, BleNotifForwardService::class.java).apply {
                action = BleNotifForwardService.ACTION_SEND
                putExtra(BleNotifForwardService.EXTRA_JSON, json)
            }

            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                startForegroundService(intent)
            } else {
                startService(intent)
            }

        } catch (e: Exception) {
            Log.e(TAG, "JSON build failed", e)
        }
    }
}