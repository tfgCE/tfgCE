@ECHO OFF
ECHO ..................................................
ECHO .                                                .
ECHO .          Copying common android files          .
ECHO .                                                .
ECHO ..................................................

CALL teaForGodAndroid_copy_to_other teaForGodPico
CALL teaForGodAndroid_copy_to_other teaForGodQuest
CALL teaForGodAndroid_copy_to_other teaForGodVive

@ECHO OFF
ECHO ..................................................
ECHO .                                                .
ECHO . Don't forget to get tea_for_god_open.keystore  .
ECHO .                                                .
ECHO ..................................................

PAUSE