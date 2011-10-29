#!/system/bin/sh
# establish root in common system directories for 3rd party applications
/system/bin/mount -o remount,rw -t rfs /dev/stl5 /system
/system/bin/rm /system/bin/su
/system/bin/rm /system/xbin/su
/system/bin/ln -s /sbin/su /system/bin/su
/system/bin/ln -s /sbin/su /system/xbin/su
# fix busybox DNS while system is read-write
if [ ! -f "/system/etc/resolv.conf" ]; then
  echo "nameserver 8.8.8.8" >> /system/etc/resolv.conf
  echo "nameserver 8.8.8.4" >> /system/etc/resolv.conf
fi
/system/bin/mount -o remount,ro -t rfs /dev/stl5 /system

#provide support for a shell script to protect root access
if [ -r "/system/protectsu.sh" ]; then
  /system/bin/sh /system/protectsu.sh
fi
r
