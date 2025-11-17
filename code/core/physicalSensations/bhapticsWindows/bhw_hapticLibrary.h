#include "..\physicalSensationsSettings.h"
#ifdef PHYSICAL_SENSATIONS_BHAPTICS_WINDOWS
/**
 * bhaptics is as provided with slight modifications to be packed as a part of "core" library
 */

#ifndef HAPTIC_LIBRARY_H
#define HAPTIC_LIBRARY_H

#include "bhw_model.h"
#include <vector>

namespace bhapticsLibrary
{
    struct point {
        float x, y;
        int intensity;
        int motorCount;
    };

    struct status {
        int values[20];
    };

    void ChangeUrl(const char* url);

    bool TryGetExePath(char* buf, int maxSize, int& size);

    const char* GetExePath();

    // Initialises a connection to the bHaptics Player. Should only be called once: when the game starts.
    void Initialise(const char* appId, const char* appName);
    void InitialiseSync(const char* appId, const char* appName);

    // End the connecection to the bHaptics Player. Should only be called once: when the game ends.
    void Destroy();

    // Register a preset .tact feedback file, created using the bHaptics Designer.
    // Registered files can then be called and played back using the given key.
    // File is submitted as an already processed JSON string of the Project attribute.
    void RegisterFeedback(const char* key, const char* projectJson);

    void RegisterFeedbackFromTactFile(const char* key, const char* tactFileStr);

    void RegisterFeedbackFromTactFileReflected(const char* key, const char* tactFileStr);

    // Register a preset .tact feedback file, created using the bHaptics Designer.
    // Registered files can then be called and played back using the given key.
    // File Path is given, and feedback file is parsed and processed by the SDK.
    void LoadAndRegisterFeedback(const char* Key, const char* FilePath);

    // Submit a request to play a registered feedback file using its Key.
    void SubmitRegistered(const char* key);

    void SubmitRegisteredStartMillis(const char* key, int startMillis);

    // Submit a request to play a registered feedback file, with additional options.
    // ScaleOption scales the intensity and duration of the feedback by some factor.
    // RotationOption uses cylindrical projection to rotate a Vest feedback file, as well as the vertical position.
    // AltKey provides a unique key to play this custom feedback under, as opposed to the original feedback Key.
    void SubmitRegisteredAlt(const char* Key, const char* AltKey, bhaptics::ScaleOption ScaleOpt, bhaptics::RotationOption RotOption);

    void SubmitRegisteredWithOption(const char* Key, const char* AltKey, float intensity, float duration, float offsetAngleX, float offsetAngleY);

    void SubmitByteArray(const char* key, bhaptics::PositionType Pos, unsigned char* buf, size_t length,
        int durationMillis);

    void SubmitPathArray(const char* key, bhaptics::PositionType Pos, point* points, size_t length,
        int durationMillis);

    // Submit an array of 20 integers, representing the strength of each motor vibration, ranging from 0 to 100.
    // Specify the Position (playback device) as well as the duration of the feedback effect in milliseconds.
    void Submit(const char* Key, bhaptics::PositionType Pos, std::vector<uint8_t>& MotorBytes, int DurationMillis);

    // Submit an array of DotPoints, representing the motor's Index and Intensity.
    // Specify the Position (playback device) as well as the duration of the feedback effect in milliseconds.
    void SubmitDot(const char* Key, bhaptics::PositionType Pos, std::vector<bhaptics::DotPoint>& Points, int DurationMillis);

    // Submit an array of PathPoints, representing the xy-position of the feedback on a unit square.
    // Based off the intensity and position, specific motors are chosen by the bHaptics Player and vibrated.
    // Specify the Position (playback device) as well as the duration of the feedback effect in milliseconds.
    void SubmitPath(const char* Key, bhaptics::PositionType Pos, std::vector<bhaptics::PathPoint>& Points, int DurationMillis);

    // Boolean to check if a Feedback has been registered or not under the given Key.
    bool IsFeedbackRegistered(const char* key);

    // Boolean to check if there are any Feedback effects currently playing.
    bool IsPlaying();

    // Boolean to check if a feedback effect under the given Key is currently playing.
    bool IsPlayingKey(const char* Key);

    // Turn off all currently playing feedback effects.
    void TurnOff();

    // Turn off the feedback effect specified by the Key.
    void TurnOffKey(const char* Key);

    // Enable Haptic Feedback calls to the bHaptics Player (on by default)
    void EnableFeedback();

    // Disable Haptic Feedback calls to the bHaptics Player
    void DisableFeedback();

    // Toggle between Enabling/Disabling haptic feedback.
    void ToggleFeedback();

    // Boolean to check if a specific device is connected to the bHaptics Player.
    bool IsDevicePlaying(bhaptics::PositionType Pos);

    // Returns an array of the current status of each device.
    // Used for UI to ensure that haptic feedback is playing.
    //void GetResponseStatus(std::vector<bhaptics::HapticFeedback>& retValues);

    // Returns the current motor values for a given device.
    // Used for UI to ensure that haptic feedback is playing.
    // return value is 20
    bool TryGetResponseForPosition(bhaptics::PositionType Pos, status& s);
};

#endif
#endif //PHYSICAL_SENSATIONS_BHAPTICS_WINDOWS
