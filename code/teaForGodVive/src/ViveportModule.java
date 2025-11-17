package com.voidroom;

import android.content.Context;
import android.content.pm.PackageManager;

public class ViveportModule
{
    private android.app.Activity activity;
    private android.content.Context context;

	public ViveportModule(android.app.Activity _activity)
    {
        Log.verbose("viveport : created module");
        activity = _activity;
        context = _activity;
    }

	private String appId;
	private String appKey;

    public String getVersion()
    {
        Log.verbose("viveport : get version");
        return com.htc.viveport.Api.version();
    }

    public native void init_callback(int nResult);
    //
    private com.htc.viveport.Api.StatusCallback init_callback_java = new com.htc.viveport.Api.StatusCallback() {
        @Override
        public void onResult(int statusCode, String result)
        {
            Log.verbose("viveport : init_callback");
            init_callback(statusCode);
        }
    };
    //
    public int init(String _appId)
    {
        Log.verbose("viveport : init");
        appId = _appId;
        return com.htc.viveport.Api.init(context, init_callback_java, appId);
    }

    public native void get_license_callback(int nResult);
    //
    private com.htc.viveport.Api.LicenseChecker licenseChecker = new com.htc.viveport.Api.LicenseChecker() {
        @Override
        public void onSuccess(long issueTime, long expirationTime, int latestVersion, boolean updateRequired)
        {
            Log.verbose("viveport : get_license_callback(0)");
            get_license_callback(0);
        }

        @Override
        public void onFailure(int errorCode, String errorMessage)
        {
            Log.verbose("viveport : get_license_callback(errorCode)");
            get_license_callback(errorCode != 0? errorCode : -1);
        }
    };
    //
    public void getLicense(String _appId, String _appKey)
    {
        Log.verbose("viveport : getLicense");
        appId = _appId;
        appKey = _appKey;
        com.htc.viveport.Api.getLicense(context, licenseChecker, appId, appKey);
    }

    public native void shutdown_callback(int nResult);
    //
    private com.htc.viveport.Api.StatusCallback shutdown_callback_java = new com.htc.viveport.Api.StatusCallback() {
        @Override
        public void onResult(int statusCode, String result)
        {
            Log.verbose("viveport : shutdown_callback");
            shutdown_callback(statusCode);
        }
    };
    //
    public void shutdown()
    {
        Log.verbose("viveport : shutdown");
        com.htc.viveport.Api.shutdown(shutdown_callback_java);
    }
}
