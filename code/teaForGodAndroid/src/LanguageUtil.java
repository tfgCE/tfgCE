package com.voidroom;

import android.content.Context;

public class LanguageUtil
{
    public static native void store_system_language(String _language);

    public static void store_system_language()
    {
        store_system_language(java.util.Locale.getDefault().getLanguage());
    }
}
