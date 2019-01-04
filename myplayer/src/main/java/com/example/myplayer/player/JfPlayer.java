package com.example.myplayer.player;

import android.text.TextUtils;

import com.example.myplayer.listener.JfOnPreparedListener;
import com.example.myplayer.log.JfLog;

public class JfPlayer {
	static {
		System.loadLibrary("avutil-55");
		System.loadLibrary("avcodec-57");
		System.loadLibrary("avformat-57");
		System.loadLibrary("avdevice-57");
		System.loadLibrary("swresample-2");
		System.loadLibrary("swscale-4");
		System.loadLibrary("postproc-54");
		System.loadLibrary("avfilter-6");
		System.loadLibrary("native-lib");
	}

	private String source;
	private JfOnPreparedListener jfOnPreparedListener;
	public JfPlayer(){

	}

	/**
	 * 设置数据源
	 * @param source
	 */
	public void setSource(String source) {
		this.source = source;
	}

	public void setJfOnPreparedListener(JfOnPreparedListener jfOnPreparedListener) {
		this.jfOnPreparedListener = jfOnPreparedListener;
	}

	public void prepared(){
		if (TextUtils.isEmpty(source)){
			JfLog.w("SOURCE IS EMPTY");
			return;
		}

		new Thread(new Runnable() {
			@Override
			public void run() {
				n_prepared(source);
			}
		}).start();
	}


	public void start(){
		if (TextUtils.isEmpty(source)){
			JfLog.w("SOURCE IS EMPTY");
			return;
		}

		new Thread(new Runnable() {
			@Override
			public void run() {
				n_start();
			}
		}).start();
	}
	/**
	 * C++层n_prepare()完成后要调用JfOnPreparedListener
	 */
	public void onCallPrepared(){
		if (jfOnPreparedListener != null)
		{
			jfOnPreparedListener.onPrepared();
		}
	}
	public native void n_prepared(String source);
	public native void n_start();
}
