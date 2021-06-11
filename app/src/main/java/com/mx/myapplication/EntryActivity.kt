package com.mx.myapplication

import android.app.Activity
import android.content.Intent
import android.os.AsyncTask
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.widget.TextView
import com.mxtech.av.AsyncAudioConverter
import com.mxtech.av.AsyncMediaConverter

class EntryActivity : AppCompatActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_entry)

        var textView = findViewById<TextView>(R.id.convert)
        textView.setOnClickListener {
            MainActivity.start(this)
        }

        textView = findViewById<TextView>(R.id.open_gl)
        textView.setOnClickListener {
            GLActivity.start(this)
        }
    }

    companion object {

        init {
            System.loadLibrary("native-lib")
        }

        fun start(activity: Activity) {
            activity.startActivity(Intent(activity, EntryActivity::class.java));
        }
    }
}