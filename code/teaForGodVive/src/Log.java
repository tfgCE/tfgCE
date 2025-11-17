package com.voidroom;

class Log
{
    public static void log(String _text)
    {
        android.util.Log.i("voidroom", _text);
    }
    public static void verbose(String _text)
    {
        android.util.Log.v("voidroom", _text);
    }
    public static void err(String _text)
    {
        android.util.Log.e("voidroom", _text);
    }
    public static void wrn(String _text)
    {
        android.util.Log.w("voidroom", _text);
    }
}
