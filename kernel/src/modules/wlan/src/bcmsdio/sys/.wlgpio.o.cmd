cmd_/home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/modules/wlan/src/bcmsdio/sys/wlgpio.o := /usr/local/arm/4.3.1-eabi-armv6/usr/bin/arm-linux-gcc -Wp,-MD,/home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/modules/wlan/src/bcmsdio/sys/.wlgpio.o.d  -nostdinc -isystem /usr/local/arm/4.3.1-eabi-armv6/usr/bin-ccache/../lib/gcc/arm-samsung-linux-gnueabi/4.3.1/include -D__KERNEL__ -Iinclude  -I/home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/linux-2.6.27/arch/arm/include -include include/linux/autoconf.h -mlittle-endian -Iarch/arm/mach-s3c6400/include -Iarch/arm/mach-s3c6410/include -Iarch/arm/plat-s3c64xx/include -Iarch/arm/plat-s3c/include -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -Werror-implicit-function-declaration -Os -marm -fno-omit-frame-pointer -mapcs -mno-sched-prolog -mabi=aapcs-linux -mno-thumb-interwork -D__LINUX_ARM_ARCH__=6 -march=armv6 -mtune=arm1136j-s -msoft-float -Uarm -fno-stack-protector -I/home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/modules/include -fno-omit-frame-pointer -fno-optimize-sibling-calls -g -Wdeclaration-after-statement -Wno-pointer-sign -I/include -I/home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/linux-2.6.27 -DBCMDBG -DBCMDBG_MEM -Dlinux -DLINUX -DBDC -DTOE -DBCMDRIVER -DBCMDONGLEHOST -DDHDTHREAD -DBCMWPA2 -DUSE_STOCK_MMC_DRIVER -DDHD_GPL -DDHD_SCHED -DBCMSDIO -DDHD_GPL -DBCMLXSDMMC -DBCMPLATFORM_BUS -DDHD_BCMEVENTS -DSHOW_EVENTS -DDHD_DEBUG -DSRCBASE=\"/home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/modules/wlan/src\" -DANDROID_SPECIFIC -DMAX_HDR_LEN=64 -DDHD_FIRSTREAD=64 -DSDIO_ISR_THREAD -DENABLE_DEEP_SLEEP -DWRITE_MACADDR -DCONFIG_BRCM_GPIO_INTR -DBCM_HOSTWAKE -DBCMHOSTWAKE_IRQ -DBCM_PKTFILTER -DBCM_ARPO -I/home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/modules/wlan/src/include/ -I/home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/modules/wlan/src/dhd/sys/ -I/home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/modules/wlan/src/dongle/ -I/home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/modules/wlan/src/bcmsdio/sys/ -I/home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/modules/wlan/src/wl/sys/ -I/home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/modules/wlan/src/shared/ -DMODULE -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(wlgpio)"  -D"KBUILD_MODNAME=KBUILD_STR(dhd)" -c -o /home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/modules/wlan/src/bcmsdio/sys/wlgpio.o /home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/modules/wlan/src/bcmsdio/sys/wlgpio.c

deps_/home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/modules/wlan/src/bcmsdio/sys/wlgpio.o := \
  /home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/modules/wlan/src/bcmsdio/sys/wlgpio.c \
    $(wildcard include/config/mach/spica.h) \
    $(wildcard include/config/mach/saturn.h) \
    $(wildcard include/config/mach/cygnus.h) \
  include/linux/delay.h \
  include/linux/kernel.h \
    $(wildcard include/config/lbd.h) \
    $(wildcard include/config/preempt/voluntary.h) \
    $(wildcard include/config/debug/spinlock/sleep.h) \
    $(wildcard include/config/printk.h) \
    $(wildcard include/config/numa.h) \
  /usr/local/arm/4.3.1-eabi-armv6/usr/bin-ccache/../lib/gcc/arm-samsung-linux-gnueabi/4.3.1/include/stdarg.h \
  include/linux/linkage.h \
  include/linux/compiler.h \
    $(wildcard include/config/enable/must/check.h) \
    $(wildcard include/config/enable/warn/deprecated.h) \
  include/linux/compiler-gcc4.h \
  include/linux/compiler-gcc.h \
    $(wildcard include/config/arch/supports/optimized/inlining.h) \
    $(wildcard include/config/optimize/inlining.h) \
  /home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/linux-2.6.27/arch/arm/include/asm/linkage.h \
  include/linux/stddef.h \
  include/linux/types.h \
    $(wildcard include/config/uid16.h) \
    $(wildcard include/config/lsf.h) \
    $(wildcard include/config/resources/64bit.h) \
  include/linux/posix_types.h \
  /home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/linux-2.6.27/arch/arm/include/asm/posix_types.h \
  /home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/linux-2.6.27/arch/arm/include/asm/types.h \
  include/asm-generic/int-ll64.h \
  include/linux/bitops.h \
    $(wildcard include/config/generic/find/first/bit.h) \
    $(wildcard include/config/generic/find/next/bit.h) \
  /home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/linux-2.6.27/arch/arm/include/asm/bitops.h \
    $(wildcard include/config/smp.h) \
  /home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/linux-2.6.27/arch/arm/include/asm/system.h \
    $(wildcard include/config/cpu/cp15.h) \
    $(wildcard include/config/cpu/xsc3.h) \
    $(wildcard include/config/cpu/xscale.h) \
    $(wildcard include/config/cpu/sa1100.h) \
    $(wildcard include/config/cpu/sa110.h) \
  /home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/linux-2.6.27/arch/arm/include/asm/memory.h \
    $(wildcard include/config/mmu.h) \
    $(wildcard include/config/dram/size.h) \
    $(wildcard include/config/dram/base.h) \
    $(wildcard include/config/discontigmem.h) \
    $(wildcard include/config/sparsemem.h) \
  arch/arm/mach-s3c6400/include/mach/memory.h \
  /home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/linux-2.6.27/arch/arm/include/asm/sizes.h \
  include/asm-generic/memory_model.h \
    $(wildcard include/config/flatmem.h) \
    $(wildcard include/config/sparsemem/vmemmap.h) \
    $(wildcard include/config/out/of/line/pfn/to/page.h) \
  include/linux/stringify.h \
  include/linux/irqflags.h \
    $(wildcard include/config/trace/irqflags.h) \
    $(wildcard include/config/irqsoff/tracer.h) \
    $(wildcard include/config/preempt/tracer.h) \
    $(wildcard include/config/trace/irqflags/support.h) \
    $(wildcard include/config/x86.h) \
  include/linux/typecheck.h \
  /home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/linux-2.6.27/arch/arm/include/asm/irqflags.h \
  /home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/linux-2.6.27/arch/arm/include/asm/ptrace.h \
    $(wildcard include/config/arm/thumb.h) \
  /home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/linux-2.6.27/arch/arm/include/asm/hwcap.h \
  include/asm-generic/cmpxchg-local.h \
  include/asm-generic/cmpxchg.h \
  include/asm-generic/bitops/non-atomic.h \
  include/asm-generic/bitops/fls64.h \
  include/asm-generic/bitops/sched.h \
  include/asm-generic/bitops/hweight.h \
  include/asm-generic/bitops/lock.h \
  include/linux/log2.h \
    $(wildcard include/config/arch/has/ilog2/u32.h) \
    $(wildcard include/config/arch/has/ilog2/u64.h) \
  include/linux/ratelimit.h \
  include/linux/param.h \
  /home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/linux-2.6.27/arch/arm/include/asm/param.h \
    $(wildcard include/config/hz.h) \
  /home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/linux-2.6.27/arch/arm/include/asm/byteorder.h \
  include/linux/byteorder/little_endian.h \
  include/linux/byteorder/swab.h \
  include/linux/byteorder/generic.h \
  /home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/linux-2.6.27/arch/arm/include/asm/bug.h \
    $(wildcard include/config/bug.h) \
    $(wildcard include/config/debug/bugverbose.h) \
  include/asm-generic/bug.h \
    $(wildcard include/config/generic/bug.h) \
  /home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/linux-2.6.27/arch/arm/include/asm/delay.h \
  arch/arm/mach-s3c6400/include/mach/gpio.h \
    $(wildcard include/config/s3c/gpio/space.h) \
  include/asm-generic/gpio.h \
    $(wildcard include/config/gpiolib.h) \
    $(wildcard include/config/gpio/sysfs.h) \
    $(wildcard include/config/have/gpio/lib.h) \
  include/linux/errno.h \
  /home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/linux-2.6.27/arch/arm/include/asm/errno.h \
  include/asm-generic/errno.h \
  include/asm-generic/errno-base.h \
  arch/arm/plat-s3c64xx/include/plat/gpio-bank-a.h \
  arch/arm/plat-s3c64xx/include/plat/gpio-bank-b.h \
  arch/arm/plat-s3c64xx/include/plat/gpio-bank-c.h \
  arch/arm/plat-s3c64xx/include/plat/gpio-bank-d.h \
  arch/arm/plat-s3c64xx/include/plat/gpio-bank-e.h \
  arch/arm/plat-s3c64xx/include/plat/gpio-bank-f.h \
  arch/arm/plat-s3c64xx/include/plat/gpio-bank-g.h \
  arch/arm/plat-s3c64xx/include/plat/gpio-bank-h.h \
  arch/arm/plat-s3c64xx/include/plat/gpio-bank-i.h \
  arch/arm/plat-s3c64xx/include/plat/gpio-bank-j.h \
  arch/arm/plat-s3c64xx/include/plat/gpio-bank-k.h \
  arch/arm/plat-s3c64xx/include/plat/gpio-bank-l.h \
  arch/arm/plat-s3c64xx/include/plat/gpio-bank-m.h \
  arch/arm/plat-s3c64xx/include/plat/gpio-bank-n.h \
  arch/arm/plat-s3c64xx/include/plat/gpio-bank-o.h \
  arch/arm/plat-s3c64xx/include/plat/gpio-bank-p.h \
  arch/arm/plat-s3c64xx/include/plat/gpio-bank-q.h \
  arch/arm/plat-s3c/include/plat/gpio-cfg.h \
  arch/arm/plat-s3c/include/plat/sdhci.h \
    $(wildcard include/config/s3c6410/setup/sdhci.h) \
    $(wildcard include/config/s3c/dev/hsmmc.h) \
    $(wildcard include/config/s3c/dev/hsmmc1.h) \
    $(wildcard include/config/s3c/dev/hsmmc2.h) \
  arch/arm/mach-s3c6400/include/mach/spica.h \
    $(wildcard include/config/spica/rev00.h) \
    $(wildcard include/config/spica/rev01.h) \
    $(wildcard include/config/spica/rev02.h) \
    $(wildcard include/config/spica/rev03.h) \
    $(wildcard include/config/spica/rev04.h) \
    $(wildcard include/config/spica/rev05.h) \
    $(wildcard include/config/spica/rev06.h) \
    $(wildcard include/config/spica/rev07.h) \
    $(wildcard include/config/spica/rev08.h) \
    $(wildcard include/config/spica/rev09.h) \
    $(wildcard include/config/spica/test/rev00.h) \
    $(wildcard include/config/spica/test/rev01.h) \
    $(wildcard include/config/spica/test/rev02.h) \
    $(wildcard include/config/spica/test/rev03.h) \
    $(wildcard include/config/board/revision.h) \
    $(wildcard include/config/spica/rev.h) \
  arch/arm/mach-s3c6400/include/mach/spica_rev02.h \
    $(wildcard include/config/reserved/mem/cmm/jpeg/mfc/post/camera.h) \
  arch/arm/plat-s3c/include/plat/devs.h \
    $(wildcard include/config/s3c6410/tvout.h) \
  include/linux/platform_device.h \
  include/linux/device.h \
    $(wildcard include/config/debug/devres.h) \
  include/linux/ioport.h \
  include/linux/kobject.h \
    $(wildcard include/config/hotplug.h) \
  include/linux/list.h \
    $(wildcard include/config/debug/list.h) \
  include/linux/poison.h \
  include/linux/prefetch.h \
  /home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/linux-2.6.27/arch/arm/include/asm/processor.h \
  /home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/linux-2.6.27/arch/arm/include/asm/cache.h \
  include/linux/sysfs.h \
    $(wildcard include/config/sysfs.h) \
  /home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/linux-2.6.27/arch/arm/include/asm/atomic.h \
  include/asm-generic/atomic.h \
  include/linux/spinlock.h \
    $(wildcard include/config/debug/spinlock.h) \
    $(wildcard include/config/generic/lockbreak.h) \
    $(wildcard include/config/preempt.h) \
    $(wildcard include/config/debug/lock/alloc.h) \
  include/linux/preempt.h \
    $(wildcard include/config/debug/preempt.h) \
    $(wildcard include/config/preempt/notifiers.h) \
  include/linux/thread_info.h \
    $(wildcard include/config/compat.h) \
  /home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/linux-2.6.27/arch/arm/include/asm/thread_info.h \
    $(wildcard include/config/arm/thumbee.h) \
  /home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/linux-2.6.27/arch/arm/include/asm/fpstate.h \
    $(wildcard include/config/vfpv3.h) \
    $(wildcard include/config/iwmmxt.h) \
  /home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/linux-2.6.27/arch/arm/include/asm/domain.h \
    $(wildcard include/config/io/36.h) \
  include/linux/bottom_half.h \
  include/linux/spinlock_types.h \
  include/linux/spinlock_types_up.h \
  include/linux/lockdep.h \
    $(wildcard include/config/lockdep.h) \
    $(wildcard include/config/lock/stat.h) \
    $(wildcard include/config/generic/hardirqs.h) \
    $(wildcard include/config/prove/locking.h) \
  include/linux/spinlock_up.h \
  include/linux/spinlock_api_up.h \
  include/linux/kref.h \
  include/linux/wait.h \
  /home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/linux-2.6.27/arch/arm/include/asm/current.h \
  include/linux/klist.h \
  include/linux/completion.h \
  include/linux/module.h \
    $(wildcard include/config/modules.h) \
    $(wildcard include/config/modversions.h) \
    $(wildcard include/config/unused/symbols.h) \
    $(wildcard include/config/kallsyms.h) \
    $(wildcard include/config/markers.h) \
    $(wildcard include/config/module/unload.h) \
  include/linux/stat.h \
  /home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/linux-2.6.27/arch/arm/include/asm/stat.h \
  include/linux/time.h \
  include/linux/cache.h \
    $(wildcard include/config/arch/has/cache/line/size.h) \
  include/linux/seqlock.h \
  include/linux/math64.h \
  /home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/linux-2.6.27/arch/arm/include/asm/div64.h \
  include/linux/kmod.h \
  include/linux/gfp.h \
    $(wildcard include/config/zone/dma.h) \
    $(wildcard include/config/zone/dma32.h) \
    $(wildcard include/config/highmem.h) \
  include/linux/mmzone.h \
    $(wildcard include/config/force/max/zoneorder.h) \
    $(wildcard include/config/memory/hotplug.h) \
    $(wildcard include/config/arch/populates/node/map.h) \
    $(wildcard include/config/flat/node/mem/map.h) \
    $(wildcard include/config/have/memory/present.h) \
    $(wildcard include/config/need/node/memmap/size.h) \
    $(wildcard include/config/need/multiple/nodes.h) \
    $(wildcard include/config/have/arch/early/pfn/to/nid.h) \
    $(wildcard include/config/sparsemem/extreme.h) \
    $(wildcard include/config/nodes/span/other/nodes.h) \
    $(wildcard include/config/holes/in/zone.h) \
  include/linux/threads.h \
    $(wildcard include/config/nr/cpus.h) \
    $(wildcard include/config/base/small.h) \
  include/linux/numa.h \
    $(wildcard include/config/nodes/shift.h) \
  include/linux/init.h \
  include/linux/nodemask.h \
  include/linux/bitmap.h \
  include/linux/string.h \
  /home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/linux-2.6.27/arch/arm/include/asm/string.h \
  include/linux/pageblock-flags.h \
    $(wildcard include/config/hugetlb/page.h) \
    $(wildcard include/config/hugetlb/page/size/variable.h) \
  include/linux/bounds.h \
  /home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/linux-2.6.27/arch/arm/include/asm/page.h \
    $(wildcard include/config/cpu/copy/v3.h) \
    $(wildcard include/config/cpu/copy/v4wt.h) \
    $(wildcard include/config/cpu/copy/v4wb.h) \
    $(wildcard include/config/cpu/copy/feroceon.h) \
    $(wildcard include/config/cpu/copy/v6.h) \
    $(wildcard include/config/aeabi.h) \
  /home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/linux-2.6.27/arch/arm/include/asm/glue.h \
    $(wildcard include/config/cpu/arm610.h) \
    $(wildcard include/config/cpu/arm710.h) \
    $(wildcard include/config/cpu/abrt/lv4t.h) \
    $(wildcard include/config/cpu/abrt/ev4.h) \
    $(wildcard include/config/cpu/abrt/ev4t.h) \
    $(wildcard include/config/cpu/abrt/ev5tj.h) \
    $(wildcard include/config/cpu/abrt/ev5t.h) \
    $(wildcard include/config/cpu/abrt/ev6.h) \
    $(wildcard include/config/cpu/abrt/ev7.h) \
    $(wildcard include/config/cpu/pabrt/ifar.h) \
    $(wildcard include/config/cpu/pabrt/noifar.h) \
  include/asm-generic/page.h \
  include/linux/memory_hotplug.h \
    $(wildcard include/config/have/arch/nodedata/extension.h) \
    $(wildcard include/config/memory/hotremove.h) \
  include/linux/notifier.h \
  include/linux/mutex.h \
    $(wildcard include/config/debug/mutexes.h) \
  include/linux/mutex-debug.h \
  include/linux/rwsem.h \
    $(wildcard include/config/rwsem/generic/spinlock.h) \
  include/linux/rwsem-spinlock.h \
  include/linux/srcu.h \
  include/linux/topology.h \
    $(wildcard include/config/sched/smt.h) \
    $(wildcard include/config/sched/mc.h) \
  include/linux/cpumask.h \
    $(wildcard include/config/hotplug/cpu.h) \
  include/linux/smp.h \
    $(wildcard include/config/use/generic/smp/helpers.h) \
  /home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/linux-2.6.27/arch/arm/include/asm/topology.h \
  include/asm-generic/topology.h \
  include/linux/elf.h \
  include/linux/elf-em.h \
  /home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/linux-2.6.27/arch/arm/include/asm/elf.h \
  /home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/linux-2.6.27/arch/arm/include/asm/user.h \
  include/linux/moduleparam.h \
    $(wildcard include/config/alpha.h) \
    $(wildcard include/config/ia64.h) \
    $(wildcard include/config/ppc64.h) \
  include/linux/marker.h \
  /home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/linux-2.6.27/arch/arm/include/asm/local.h \
  include/asm-generic/local.h \
  include/linux/percpu.h \
  include/linux/slab.h \
    $(wildcard include/config/slab/debug.h) \
    $(wildcard include/config/debug/objects.h) \
    $(wildcard include/config/slub.h) \
    $(wildcard include/config/slob.h) \
    $(wildcard include/config/debug/slab.h) \
    $(wildcard include/config/slabinfo.h) \
  include/linux/slab_def.h \
  include/linux/kmalloc_sizes.h \
  /home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/linux-2.6.27/arch/arm/include/asm/percpu.h \
  include/asm-generic/percpu.h \
    $(wildcard include/config/have/setup/per/cpu/area.h) \
  include/linux/hardirq.h \
    $(wildcard include/config/virt/cpu/accounting.h) \
    $(wildcard include/config/preempt/rcu.h) \
    $(wildcard include/config/no/hz.h) \
  include/linux/smp_lock.h \
    $(wildcard include/config/lock/kernel.h) \
  /home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/linux-2.6.27/arch/arm/include/asm/hardirq.h \
  /home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/linux-2.6.27/arch/arm/include/asm/irq.h \
  arch/arm/mach-s3c6400/include/mach/irqs.h \
  arch/arm/plat-s3c64xx/include/plat/irqs.h \
  include/linux/irq_cpustat.h \
  /home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/linux-2.6.27/arch/arm/include/asm/module.h \
  include/linux/pm.h \
    $(wildcard include/config/pm/sleep.h) \
  include/linux/semaphore.h \
  /home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/linux-2.6.27/arch/arm/include/asm/device.h \
    $(wildcard include/config/dmabounce.h) \
  include/linux/pm_wakeup.h \
    $(wildcard include/config/pm.h) \
  include/linux/mmc/host.h \
    $(wildcard include/config/mmc/debug.h) \
    $(wildcard include/config/leds/triggers.h) \
    $(wildcard include/config/mmc/embedded/sdio.h) \
  include/linux/leds.h \
    $(wildcard include/config/leds/trigger/ide/disk.h) \
  include/linux/mmc/core.h \
  include/linux/interrupt.h \
    $(wildcard include/config/high/res/timers.h) \
    $(wildcard include/config/generic/irq/probe.h) \
    $(wildcard include/config/proc/fs.h) \
  include/linux/irqreturn.h \
  include/linux/sched.h \
    $(wildcard include/config/sched/debug.h) \
    $(wildcard include/config/detect/softlockup.h) \
    $(wildcard include/config/split/ptlock/cpus.h) \
    $(wildcard include/config/keys.h) \
    $(wildcard include/config/bsd/process/acct.h) \
    $(wildcard include/config/taskstats.h) \
    $(wildcard include/config/audit.h) \
    $(wildcard include/config/inotify/user.h) \
    $(wildcard include/config/posix/mqueue.h) \
    $(wildcard include/config/user/sched.h) \
    $(wildcard include/config/schedstats.h) \
    $(wildcard include/config/task/delay/acct.h) \
    $(wildcard include/config/fair/group/sched.h) \
    $(wildcard include/config/rt/group/sched.h) \
    $(wildcard include/config/blk/dev/io/trace.h) \
    $(wildcard include/config/cc/stackprotector.h) \
    $(wildcard include/config/sysvipc.h) \
    $(wildcard include/config/security.h) \
    $(wildcard include/config/auditsyscall.h) \
    $(wildcard include/config/rt/mutexes.h) \
    $(wildcard include/config/task/xacct.h) \
    $(wildcard include/config/cpusets.h) \
    $(wildcard include/config/cgroups.h) \
    $(wildcard include/config/futex.h) \
    $(wildcard include/config/fault/injection.h) \
    $(wildcard include/config/latencytop.h) \
    $(wildcard include/config/have/unstable/sched/clock.h) \
    $(wildcard include/config/preempt/bkl.h) \
    $(wildcard include/config/tracing.h) \
    $(wildcard include/config/group/sched.h) \
    $(wildcard include/config/mm/owner.h) \
  include/linux/capability.h \
  include/linux/timex.h \
  /home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/linux-2.6.27/arch/arm/include/asm/timex.h \
  arch/arm/plat-s3c/include/mach/timex.h \
  include/linux/jiffies.h \
  include/linux/rbtree.h \
  include/linux/mm_types.h \
    $(wildcard include/config/cgroup/mem/res/ctlr.h) \
    $(wildcard include/config/mmu/notifier.h) \
  include/linux/auxvec.h \
  /home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/linux-2.6.27/arch/arm/include/asm/auxvec.h \
  include/linux/prio_tree.h \
  /home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/linux-2.6.27/arch/arm/include/asm/mmu.h \
    $(wildcard include/config/cpu/has/asid.h) \
  /home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/linux-2.6.27/arch/arm/include/asm/cputime.h \
  include/asm-generic/cputime.h \
  include/linux/sem.h \
  include/linux/ipc.h \
  /home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/linux-2.6.27/arch/arm/include/asm/ipcbuf.h \
  /home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/linux-2.6.27/arch/arm/include/asm/sembuf.h \
  include/linux/rcupdate.h \
    $(wildcard include/config/classic/rcu.h) \
  include/linux/rcuclassic.h \
  include/linux/signal.h \
  /home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/linux-2.6.27/arch/arm/include/asm/signal.h \
  include/asm-generic/signal.h \
  /home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/linux-2.6.27/arch/arm/include/asm/sigcontext.h \
  /home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/linux-2.6.27/arch/arm/include/asm/siginfo.h \
  include/asm-generic/siginfo.h \
  include/linux/fs_struct.h \
  include/linux/path.h \
  include/linux/pid.h \
  include/linux/proportions.h \
  include/linux/percpu_counter.h \
  include/linux/seccomp.h \
    $(wildcard include/config/seccomp.h) \
  include/linux/rtmutex.h \
    $(wildcard include/config/debug/rt/mutexes.h) \
  include/linux/plist.h \
    $(wildcard include/config/debug/pi/list.h) \
  include/linux/resource.h \
  /home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/linux-2.6.27/arch/arm/include/asm/resource.h \
  include/asm-generic/resource.h \
  include/linux/timer.h \
    $(wildcard include/config/timer/stats.h) \
    $(wildcard include/config/debug/objects/timers.h) \
  include/linux/ktime.h \
    $(wildcard include/config/ktime/scalar.h) \
  include/linux/debugobjects.h \
    $(wildcard include/config/debug/objects/free.h) \
  include/linux/hrtimer.h \
  include/linux/task_io_accounting.h \
    $(wildcard include/config/task/io/accounting.h) \
  include/linux/latencytop.h \
  include/linux/cred.h \
  include/linux/aio.h \
  include/linux/workqueue.h \
  include/linux/aio_abi.h \
  include/linux/uio.h \
  include/linux/i2c/pmic.h \
    $(wildcard include/config/pmic/max8906.h) \
    $(wildcard include/config/pmic/max8698.h) \
  include/linux/i2c/max8698.h \

/home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/modules/wlan/src/bcmsdio/sys/wlgpio.o: $(deps_/home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/modules/wlan/src/bcmsdio/sys/wlgpio.o)

$(deps_/home/jithu/SPICA_OCT5/BR_200812/buildroot/project_build_arm/spica/modules/wlan/src/bcmsdio/sys/wlgpio.o):
