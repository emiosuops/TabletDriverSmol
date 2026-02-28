use SendInput version to not need my modded vmulti

Change settings by opening exe in notepad, search for 056a

0X056A = VID (Vendor UD)

0X00DD = PID (Product ID)

0X000D = Usage Page

0x0001 = Usage

13440 = area in wacom counts, 134.40 mm

7560 = area in tablet counts, 75.60 mm


all code fits in 1 page on disk and in ram (sub 4 kb)

main loop fits on 2 cachelines atm

lowest cpu usage of any osu tablet driver aka 0.00% perma (11900k)


Some tablets need setfeature and wont work yet


More perf improvements to come
