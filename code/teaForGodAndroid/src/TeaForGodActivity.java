package com.voidroom;

import android.content.Context;
import android.app.NativeActivity;

public class TeaForGodActivity extends android.app.NativeActivity
{
    protected BhapticsModule bhapticsModule = null;

    public BhapticsModule getBhapticsModule()
    {
        Log.verbose("getBhapticsModule()");
    	if (bhapticsModule == null)
    	{
            Log.verbose("bhapticsModule not initialised, initialise, if we're here, not from onCreate, this may not work!");
            bhapticsModule = new BhapticsModule(this);
    	}
        return bhapticsModule;
    }

   	@Override
    protected void onCreate(android.os.Bundle savedInstanceState)
    {
        Log.log("create TeaForGodActivity, initialise bhaptics module");
        bhapticsModule = new BhapticsModule(this);

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
