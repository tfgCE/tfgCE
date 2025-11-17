package com.voidroom;

import android.content.Context;
import android.app.NativeActivity;

public class TeaForGodActivity extends android.app.NativeActivity
{
    @Override
    protected void onCreate(android.os.Bundle savedInstanceState)
    {
        LanguageUtil.store_system_language();

        super.onCreate(savedInstanceState);
    }

    @Override
    protected void onDestroy()
    {
        super.onDestroy(); // call it explicitly
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String permissions[], int[] grantResults)
    {
        Log.log("permissions granted");
    }

	static
	{
        Log.log("load libraries");
        Log.log("load fmod");
		System.loadLibrary("fmod");
        Log.log("load fmodstudio");
		System.loadLibrary("fmodstudio");
        Log.log("load teaForGodAndroid");
		System.loadLibrary("teaForGodAndroid");
        Log.log("libraries loaded");
	}
}
