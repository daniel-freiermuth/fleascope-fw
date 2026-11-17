cd cpustick
if [ "`uname 2>/dev/null`" = Linux ]; then
  BASIC=../stickos/obj.linux.MK0512GPK.DEBUG/stickos
else
  BASIC=../stickos/PIC32/basic.exe
fi
"$BASIC" help <basic0.c >help.c
