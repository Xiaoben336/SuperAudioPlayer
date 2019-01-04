package com.example.myplayer.log;

import android.util.Log;

public class JfLog {

	public static void d(String debug){
		Log.d("JF0613",debug);
	}

	public static void w(String warning){
		Log.w("JF0613",warning);
	}

	public static void e(String error){
		Log.e("JF0613",error);
	}
}
