package com.voidroom;

import android.content.Context;
import android.content.pm.PackageManager;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import java.util.List;

public class BhapticsModule
{
    // partially based, mostly copied from bhaptics SDK

    private android.app.Activity activity;
    private android.content.Context context;
    private com.bhaptics.bhapticsmanger.SdkRequestHandler sdkRequestHandler;
    private String LatestDevices = "";
    private String LatestScanStatus = "";
    private String LatestPlayerResponse = "";

    public com.bhaptics.bhapticsmanger.SdkRequestHandler getHandler() { return sdkRequestHandler; }

    private String getAppName() { return "TeaForGod"; }

	public BhapticsModule(android.app.Activity _activity)
    {
        activity = _activity;
        context = _activity;
        Log.verbose("initialise bhapticsManager");
        sdkRequestHandler = new com.bhaptics.bhapticsmanger.SdkRequestHandler(context, getAppName());
        Log.verbose("bhapticsManager initialised");
    }

    //Helper Functions
    private static String DeviceToJsonString(List<com.bhaptics.service.SimpleBhapticsDevice> devices)
    {
      JSONArray jsonArray = new JSONArray();

      for (com.bhaptics.service.SimpleBhapticsDevice device : devices)
      {
          JSONObject obj = deviceToJsonObject(device);

          if (obj == null)
          {
              Log.err("toJsonString: failed to parse. " + device);
              continue;
          }
          jsonArray.put(obj);
      }

      return jsonArray.toString();
    }

    private static JSONObject deviceToJsonObject(com.bhaptics.service.SimpleBhapticsDevice device)
    {
        try
        {
            JSONObject obj = new JSONObject();
            obj.put("DeviceName", device.getDeviceName());
            obj.put("Address", device.getAddress());
            obj.put("Position", com.bhaptics.service.SimpleBhapticsDevice.positionToString(device.getPosition()));
            obj.put("IsConnected", device.isConnected());
            obj.put("IsPaired", device.isPaired());
            obj.put("Battery", device.getBattery());
            obj.put("IsAudioJackIn", device.isAudioJackIn());
            return obj;
        }
        catch (JSONException e)
        {
            Log.verbose("toJsonObject: " + e.getMessage());
            e.printStackTrace();
        }
        return null;
    }

    public void refreshPairing()
    {
        Log.verbose("refresh pairing");
        sdkRequestHandler.refreshPairing();
    }

    public void togglePosition(String address)
    {
        Log.verbose("togglePosition() " + address);
        sdkRequestHandler.togglePosition(address);
    }

    public void ping(String address)
    {
        Log.verbose("ping() " + address);
        sdkRequestHandler.ping(address);
    }
    
    public void pingAll()
    {
        List<com.bhaptics.service.SimpleBhapticsDevice> deviceList = sdkRequestHandler.getDeviceList();

        for (com.bhaptics.service.SimpleBhapticsDevice device : deviceList)
        {
            if (device.isConnected())
            {
                sdkRequestHandler.ping(device.getAddress());
            }
        }
    }

    public String getDeviceList()
    {
        Log.verbose("getDeviceList() ");
        List<com.bhaptics.service.SimpleBhapticsDevice> deviceList = sdkRequestHandler.getDeviceList();
        return DeviceToJsonString(deviceList);
    }

    public void submitRegistered(String key, String altKey,
                                 float intensity, float duration,
                                 float offsetAngleX, float offsetY)
    {
        Log.verbose("submitRegistered() " + key + " " + altKey + " int:" + intensity + " dur:" + duration + " offX:" + offsetAngleX + " offY:" + offsetY);
        sdkRequestHandler.submitRegistered(key, altKey, intensity, duration, offsetAngleX, offsetY);
    }

    public void registerSensation(String key, String fileStr)
    {
        Log.verbose("registerSensation() " + key);
        sdkRequestHandler.register(key, fileStr);
    }

    public void registerReflected(String key, String fileStr)
    {
        Log.verbose("registerReflected() " + key);
        sdkRequestHandler.registerReflected(key, fileStr);
    }

    public void turnOff(String key) // ?
    {
        sdkRequestHandler.turnOff(key);
    }

    public void turnOffAll() // ?
    {
        sdkRequestHandler.turnOffAll();
    }

    public byte[] getPositionStatus(String position) // ?
    {
        Log.verbose("getPositionStatus() " + position);
        if (sdkRequestHandler == null)
        {
            Log.verbose("getPositionStatus() null" );
            return new byte[20];
        }
        return  sdkRequestHandler.getPositionStatus(position);
    }

    public boolean isPlaying(String key)
    {
        Log.verbose("isPlaying() " + key );
        return sdkRequestHandler.isPlaying(key);
    }

    public boolean isAnythingPlaying() // ?
    {
        Log.verbose("isAnythingPlaying() " );
        return sdkRequestHandler.isAnythingPlaying();
    }
}
