package com.example.zjf.superaudioplayer;

import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.TextView;
import android.widget.Toast;

import com.example.myplayer.bean.JfTimeInfoBean;
import com.example.myplayer.listener.JfOnLoadingListener;
import com.example.myplayer.listener.JfOnPauseResumeListener;
import com.example.myplayer.listener.JfOnPreparedListener;
import com.example.myplayer.listener.JfOnTimeInfoListener;
import com.example.myplayer.log.JfLog;
import com.example.myplayer.player.JfPlayer;
import com.example.myplayer.util.JfTimeUtil;

import java.io.File;

public class MainActivity extends AppCompatActivity {
	private static final String TAG = "MainActivity";
	private TextView tv_time;
	JfPlayer jfPlayer;
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		tv_time = (TextView) findViewById(R.id.tv_time);
		jfPlayer = new JfPlayer();
		jfPlayer.setJfOnPreparedListener(new JfOnPreparedListener() {
			@Override
			public void onPrepared() {
				JfLog.d("播放声音吧");
				jfPlayer.start();
			}
		});

		jfPlayer.setJfOnLoadingListener(new JfOnLoadingListener() {
			@Override
			public void onLoading(boolean loading) {
				if (loading) {
					JfLog.d("加载中。。。");
				} else {
					JfLog.d("播放中。。。");
				}
			}
		});

		jfPlayer.setJfOnPauseResumeListener(new JfOnPauseResumeListener() {
			@Override
			public void onPause(boolean pause) {
				if (pause) {
					JfLog.d("暂停中。。。");
				} else {
					JfLog.d("继续播放中。。。");
				}

			}
		});

		jfPlayer.setJfOnTimeInfoListener(new JfOnTimeInfoListener() {
			@Override
			public void onTimeInfo(JfTimeInfoBean timeInfoBean) {
				//JfLog.d(timeInfoBean.toString());
				Message message = Message.obtain();
				message.what = 1;
				message.obj = timeInfoBean;
				handler.sendMessage(message);
			}
		});
	}

	public void begin(View view) {
		jfPlayer.setSource("http://mpge.5nd.com/2015/2015-11-26/69708/1.mp3");
		//jfPlayer.setSource(Environment.getExternalStorageDirectory() + File.separator + "Charlie Puth - Look At Me Now.mp3");
		Log.d(TAG,"Environment.getExternalStorageDirectory() + File.separator  === " + Environment.getExternalStorageDirectory() + File.separator + "");
		jfPlayer.prepared();
	}


	public void pause(View view) {
		jfPlayer.pause();
	}

	public void resume(View view) {
		jfPlayer.resume();
	}


	Handler handler = new Handler(){
		@Override
		public void handleMessage(Message msg) {
			super.handleMessage(msg);
			if (msg.what == 1) {
				JfTimeInfoBean jfTimeInfoBean = (JfTimeInfoBean) msg.obj;
				tv_time.setText(JfTimeUtil.secdsToDateFormat(jfTimeInfoBean.getTotalTime(),jfTimeInfoBean.getTotalTime()) +
								"/" + JfTimeUtil.secdsToDateFormat(jfTimeInfoBean.getCurrentTime(),jfTimeInfoBean.getTotalTime()));
			}
		}
	};
}
