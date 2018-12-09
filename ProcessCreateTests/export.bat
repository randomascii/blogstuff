wpaexporter -i "%userprofile%\documents\etwtraces\2018-11-30_08-02-17 usercrit processcreatetests.etl" -range 2.0s 4.0s -profile UserCritEvents.wpaProfile
move Generic_Events_Randomascii_UserCrit_Events.csv create_processes.csv
wpaexporter -i "%userprofile%\documents\etwtraces\2018-11-30_08-02-17 usercrit processcreatetests.etl" -range 4.4s 6.7s -profile UserCritEvents.wpaProfile
move Generic_Events_Randomascii_UserCrit_Events.csv destroy_processes.csv
