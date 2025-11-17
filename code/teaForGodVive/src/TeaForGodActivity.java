package com.voidroom;

import android.content.Context;
import android.app.NativeActivity;
//import com.htc.vr.sdk.VRActivity;

public class TeaForGodActivity extends android.app.NativeActivity
//public class TeaForGodActivity extends VRActivity
{
    protected ViveportModule viveportModule = null;

    public ViveportModule getViveportModule()
    {
        Log.verbose("getViveportModule()");
    	if (viveportModule == null)
    	{
            Log.verbose("viveportModule not initialised, initialise, if we're here, not from onCreate, this may not work!");
            viveportModule = new ViveportModule(this);
    	}
        return viveportModule;
    }

    @Override
    protected void onCreate(android.os.Bundle savedInstanceState)
    {
        Log.log("onCreate TeaForGodActivity");
        
        // this doesn't do anything right now
        //Log.log("onCreate, initWaveVR");
        //initWaveVR();

        Log.log("initialise viveport module");
        viveportModule = new ViveportModule(this);
        Log.log("viveport module initialised");

        LanguageUtil.store_system_language();

        super.onCreate(savedInstanceState);
    }

   	@Override
    protected void onDestroy()
    {
        super.onDestroy(); // call it explicitly
    }

    @SuppressWarnings("unused")
    public native void initWaveVR();

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
        // this is now loaded from code
        //Log.log("load wvr_api");
        //System.loadLibrary("wvr_api");
        Log.log("load teaForGodAndroid");
        System.loadLibrary("teaForGodAndroid");
        Log.log("libraries loaded");
	}
}
