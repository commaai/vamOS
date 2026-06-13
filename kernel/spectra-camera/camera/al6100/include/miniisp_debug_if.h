#ifndef _ISPDBG_IOCTL_H
#define _ISPDBG_IOCTL_H

#include <asm-generic/ioctl.h>         //_IOW(), _IOR()
#include <linux/types.h>
#include <miniISP/miniISP_ioctl.h>

struct ioctl_regRW_cmd {
    u32 RegAddr;
    u32 RegVal;
};

struct ioctl_regBulkRW_cmd {
    u32 StartAddr;
    u32 EndAddr;
};

struct ioctl_memRW_cmd {
    u32 MemAddr;
    u32 len;
};

#define IOC_REGCMD_MAGIC 'R'
#define IOC_MEMCMD_MAGIC 'M'
#define IOC_BULKCMD_MAGIC 'B'
#define IOC_INFOCMD_MAGIC 'I'
#define IOC_PREPARECMD_MAGIC 'P'

/*------------------ _IOX(TYPE 8bits, nr 8bits, size 14bits) */
#if 1
#define IOCTL_REGREAD _IOR(IOC_REGCMD_MAGIC, 1, struct ioctl_regRW_cmd)
#define IOCTL_REGWRITE _IOW(IOC_REGCMD_MAGIC, 2, struct ioctl_regRW_cmd)
#define IOCTL_REGBULKREAD _IOR(IOC_BULKCMD_MAGIC, 3, struct ioctl_regBulkRW_cmd)
#define IOCTL_REGBULKWRITE _IOR(IOC_BULKCMD_MAGIC, 4, struct ioctl_regBulkRW_cmd)
#define IOCTL_MEMREAD _IOR(IOC_MEMCMD_MAGIC, 5, struct ioctl_memRW_cmd)
#define IOCTL_MEMWRITE _IOW(IOC_MEMCMD_MAGIC, 6, struct ioctl_memRW_cmd)
#define IOCTL_MUNMAP_DONE _IOW(IOC_MEMCMD_MAGIC, 7, struct ioctl_memRW_cmd)
#define IOCTL_MEMGET _IOW(IOC_MEMCMD_MAGIC, 8, struct ioctl_memRW_cmd)
#define IOCTL_IRP_DEPTH_INFO _IOR(IOC_INFOCMD_MAGIC, 9, struct irp_and_depth_information)
#define IOCTL_REFRESH_MODULE _IOR(IOC_PREPARECMD_MAGIC, 10, u8)
#endif

#if 0
#define IOCTL_REGREAD _IOR(IOC_REGCMD_MAGIC, 1, struct ioctl_regRW_cmd)
#define IOCTL_REGWRITE _IOW(IOC_REGCMD_MAGIC, 2, struct ioctl_regRW_cmd)
#define IOCTL_MEMREAD _IOR(IOC_MEMCMD_MAGIC, 3, struct ioctl_memRW_cmd)
#define IOCTL_MEMWRITE _IOW(IOC_MEMCMD_MAGIC, 4, struct ioctl_memRW_cmd)
#define IOCTL_GETMEM _IOW(IOC_MEMCMD_MAGIC, 5, struct ioctl_memRW_cmd)
#define IOCTL_MUNMAP_DONE _IOW(IOC_MEMCMD_MAGIC, 6, struct ioctl_memRW_cmd)
#endif

#endif
