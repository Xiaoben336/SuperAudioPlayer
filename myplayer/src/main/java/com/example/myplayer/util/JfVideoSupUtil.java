package com.example.myplayer.util;

import android.media.MediaCodecList;

import java.util.HashMap;
import java.util.Map;

public class JfVideoSupUtil {
	private static Map<String,String> codecMap = new HashMap<>();

	static {
		codecMap.put("h264","video/avc");

	}


	public static String findVideoCodecName(String ffcodecname){
		if (codecMap.containsKey(ffcodecname)){
			return codecMap.get(ffcodecname);
		}
		return "";
	}


	//@RequiresApi(api = Build.VERSION_CODES.JELLY_BEAN)
	public static boolean isSupCodec(String ffcodecname){
		boolean supVideo = false;

		int count = MediaCodecList.getCodecCount();
		for (int i = 0;i < count;i++){
			String[] types = MediaCodecList.getCodecInfoAt(i).getSupportedTypes();
			for (int j = 0;j < types.length;j++){
				if (types[j].equals(findVideoCodecName(ffcodecname))) {
					supVideo = true;
					break;
				}
			}
			if (supVideo){
				break;
			}
		}
		return supVideo;
	}
}
