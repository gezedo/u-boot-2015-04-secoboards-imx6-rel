/*
 * (C) Copyright 2015 Seco
 *
 * Author: Davide Cardillo <davide.cardillo@seco.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 *
 * This code provide a tool to properly configure the boot environment
 * for the Seco Boards.
 *
 */


/*
 * Boot support
 */
#include <common.h>
#include <command.h>
#include <environment.h> 
#include <stdio_dev.h>

#include <common.h>
#include <command.h>
#include <malloc.h>
#include <stdio_dev.h>
#include <timestamp.h>
#include <version.h>
#include <net.h>
#include <serial.h>
#include <nand.h>
#include <onenand_uboot.h>
#include <mmc.h>
#include <watchdog.h>

/*  __________________________________________________________________________
 * |                                                                          |
 * |                                      FDT                                 |
 * |__________________________________________________________________________|
 */
#define ENV_FDT_SRC_MMC     \
	"mmc dev ${fdt_device_id}; fatload mmc ${fdt_device_id}:${fdt_partition} ${fdt_loadaddr} ${fdt_file}"

#define ENV_FDT_SRC_USD     \
	"mmc dev ${fdt_device_id}; fatload mmc ${fdt_device_id}:${fdt_partition} ${fdt_loadaddr} ${fdt_file}"

#define ENV_FDT_SRC_EXTSD   \
	"mmc dev ${fdt_device_id}; fatload mmc ${fdt_device_id}:${fdt_partition} ${fdt_loadaddr} ${fdt_file}"

#define ENV_FDT_SRC_SATA    \
	"sata init; fatload sata 0:${fdt_partition} ${fdt_loadaddr} ${fdt_file}"

#define ENV_FDT_SRC_USB     \
	"usb start; fatload usb 0:${fdt_partition} ${fdt_loadaddr} ${fdt_file}"

#define ENV_FDT_SRC_SPI     \
	"sf probe 0; sf ${fdt_loadaddr} ${fdt_spi_addr} ${fdt_spi_len}"

#define ENV_FDT_SRC_TFTP    \
	"tftp ${fdt_loadaddr} ${fdt_file}"


#define GET_FDT_PATH                   getenv ("fdt_file");

#define SAVE_FDT_DEVICE_ID(x)          setenv ("fdt_device_id", (x))
#define SAVE_FDT_PARTITION(x)          setenv ("fdt_partition", (x))
#define SAVE_FDT_LOADADDR(x)           setenv ("fdt_loadaddr", (x))
#define SAVE_FDT_PATH(x)               setenv ("fdt_file", (x))
#define SAVE_FDT_SPI_ADDR(x)           setenv ("fdt_spi_addr", (x))
#define SAVE_FDT_SPI_LEN(x)            setenv ("fdt_spi_len", (x))
#define SAVE_FDT_BOOT_LOAD(x)          setenv ("fdt_load", (x))



/*  __________________________________________________________________________
 * |                                                                          |
 * |                                   KERNEL                                 |
 * |__________________________________________________________________________|
 */
#define ENV_KERNEL_SRC_MMC     \
	"mmc dev ${kernel_device_id}; fatload mmc ${kernel_device_id}:${kernel_partition} ${kernel_loadaddr} ${kernel_file}"

#define ENV_KERNEL_SRC_USD     \
	"mmc dev ${kernel_device_id}; fatload mmc ${kernel_device_id}:${kernel_partition} ${kernel_loadaddr} ${kernel_file}"

#define ENV_KERNEL_SRC_EXTSD   \
	"mmc dev ${kernel_device_id}; fatload mmc ${kernel_device_id}:${kernel_partition} ${kernel_loadaddr} ${kernel_file}"

#define ENV_KERNEL_SRC_SATA    \
	"sata init; fatload sata 0:${kernel_partition} ${kernel_loadaddr} ${kernel_file}"

#define ENV_KERNEL_SRC_USB     \
	"usb start; fatload usb 0:${kernel_partition} ${kernel_loadaddr} ${kernel_file}"

#define ENV_KERNEL_SRC_SPI     \
	"sf probe 0; sf ${kernel_loadaddr} ${kernel_spi_addr} ${kernel_spi_len}"

#define ENV_KERNEL_SRC_TFTP    \
	"tftp ${kernel_loadaddr} ${kernel_file}"


#define GET_KERNEL_PATH                   getenv ("kernel_file")

#define SAVE_KERNEL_DEVICE_ID(x)          setenv ("kernel_device_id", (x))
#define SAVE_KERNEL_PARTITION(x)          setenv ("kernel_partition", (x))
#define SAVE_KERNEL_LOADADDR(x)           setenv ("kernel_loadaddr", (x))
#define SAVE_KERNEL_PATH(x)               setenv ("kernel_file", (x))
#define SAVE_KERNEL_SPI_ADDR(x)           setenv ("kernel_spi_addr", (x))
#define SAVE_KERNEL_SPI_LEN(x)            setenv ("kernel_spi_len", (x))
#define GET_ENV_KERNEL_SRC(srv)           ( ENV_KERNEL_SRC_ ## (src) )
#define SAVE_KERNEL_BOOT_LOAD(x)          setenv ("kernel_load", (x))


/*  __________________________________________________________________________
 * |                                                                          |
 * |                                 FILE SYSTEM                              |
 * |__________________________________________________________________________|
 */
#define ENV_FS_COMMON  "rootwait rw"

#define ENV_FS_SRC_MMC      \
	"root=\'/dev/mmcblk${root_device_id}p${root_partition} "ENV_FS_COMMON"\'"

#define ENV_FS_SRC_USD      \
	"root=\'/dev/mmcblk${root_device_id}p${root_partition} "ENV_FS_COMMON"\'"

#define ENV_FS_SRC_EXTSD    \
	"root=\'/dev/mmcblk${root_device_id}p${root_partition} "ENV_FS_COMMON"\'"

#define ENV_FS_SRC_SATA     \
	"root=\'/dev/sda${root_partition} "ENV_FS_COMMON"\'"

#define ENV_FS_SRC_USB      \
	"root=\'/dev/sda${root_partition} "ENV_FS_COMMON"\'"

#define ENV_FS_SRC_NFS      \
	"root=\'/dev/nfs nfsroot=${nfs_ip_server}:${nfs_path} nolock,wsize=4096,rsize=4096 ip=${ip} "ENV_FS_COMMON"\'"


#define SAVE_FILESYSTEM_DEVICE_ID(x)       setenv ("root_device_id", (x))
#define SAVE_FILESYSTEM_PARTITION(x)       setenv ("root_partition", (x))
#define SAVE_FILESYSTEM_ROOT(x)            setenv ("root_set_dev", (x))
#define GET_ENV_FS_SRC(srv)                ( ENV_FS_SRC_ ## src )



/*  __________________________________________________________________________
 * |                                                                          |
 * |                               BOOTADRGS BASE                             |
 * |__________________________________________________________________________|
 */

#define DEF_TFTP_SERVER_IP      "10.0.0.1"
#define DEF_TFTP_CLIENT_IP      "10.0.0.100"

#define CONSOLE_INTERFACE   \
	"console="CONFIG_CONSOLE_DEV "," __stringify(CONFIG_BAUDRATE) "\0"

#if defined CONFIG_TARGET_MX6SECO_Q7_928 || CONFIG_TARGET_MX6SECO_UQ7_962 || CONFIG_TARGET_MX6SECO_UQ7_J_A75
#define BOOTARGS_BASE       \
	"setenv bootargs ${console_interface} ${memory} ${cpu_freq} ${videomode} ${serial_dev} ${audio_codec}"
#else
#define BOOTARGS_BASE       \
	"setenv bootargs ${console_interface} ${memory} ${cpu_freq} ${videomode}"
#endif


/*  __________________________________________________________________________
 * |                                                                          |
 * |                               NFS PARAMETERS                             |
 * |__________________________________________________________________________|
 */

#define DEF_IP_LOCAL        "10.0.0.100"
#define DEF_IP_SERVER       "10.0.0.1"
#define DEF_NETMASK         "255.255.255.0"
#define DEF_NFS_PATH        "/targetfs/"

#define GET_IP_LOCAL        getenv ("nfs_ip_client")
#define GET_IP_SERVER       getenv ("nfs_ip_server")
#define GET_NETMASK         getenv ("nfs_netmask")
#define GET_NFS_PATH        getenv ("nfs_path")

#define SAVE_NFS_USE(x)          setenv ("run_from_nfs", (x))
#define SAVE_NFS_IP_CLIENT(x)    setenv ("nfs_ip_client", (x))
#define SAVE_NFS_IP_SERVER(x)    setenv ("nfs_ip_server", (x))
#define SAVE_NFS_NETMASK(x)      setenv ("nfs_netmask", (x))
#define SAVE_NFS_PATH(x)         setenv ("nfs_path", (x))
#define SAVE_NFS_USE_DHCP(x)     setenv ("nfs_use_dhcp", (x))


#define SET_IPCONF_NO_DHCP  \
	"setenv ip \"${nfs_ip_client}:::${nfs_netmask}::eth0:off\""

#define SET_IPCONF_DHCP     \
	"setenv ip \":::::eth0:dhcp\""

#define SET_IP              \
	"if test \"${nfs_use_dhcp}\" = \"0\"; then run set_ipconf_no_dhcp; else run set_ipconf_dhcp; fi"

#define SET_NFSROOT         \
	"setenv nfsroot \"${nfs_ip_server}:${nfs_path}\""



/*  __________________________________________________________________________
 * |                                                                          |
 * |                                  COMMAND                                 |
 * |__________________________________________________________________________|
 */
#define SET_BOOT_DEV       \
	"run set_kernel_boot_dev; run set_fdt_boot_dev; run set_root_dev;"

#define LOAD_BOOT_DEV      \
	"run kernel_boot_dev; run fdt_boot_dev;"

#define LOAD_ROOT_DEV      \
	"setenv bootargs ${bootargs} ${root_dev}"

#define CMD_BOOT           \
	"run bootargs_base; run set_boot_dev; run load_boot_dev; run load_root_dev; " \
	"bootz ${kernel_loadaddr} - ${fdt_loadaddr}"


typedef enum {
	DEV_EMMC,
	DEV_U_SD,
	DEV_EXT_SD,
	DEV_NAND,
	DEV_SPI,
	DEV_SATA,
	DEV_USB,
	DEV_TFTP,
	DEV_NFS,
} device_t;


typedef struct data_boot_dev {
	device_t         dev_type;
	char             *label;
	char             *env_str;
	char             *device_id;
	char             *load_address;
	char             *def_path;
} data_boot_dev_t;


#if defined(CONFIG_TARGET_MX6SECO_Q7_928)
#define BOOT_DEV_ID_EMMC      __stringify(CONFIG_BOOT_ID_EMMC)"\0"
#define BOOT_DEV_ID_U_SD      __stringify(CONFIG_BOOT_ID_USD)"\0"
#define BOOT_DEV_ID_EXT_SD    __stringify(CONFIG_BOOT_ID_EXT_SD)"\0"
#define BOOT_DEV_ID_SPI       "0"
#define BOOT_DEV_ID_SATA      "0"
#define BOOT_DEV_ID_USB       "0"

#define ROOT_DEV_ID_EMMC      __stringify(CONFIG_ROOT_ID_EMMC)"\0"
#define ROOT_DEV_ID_U_SD      __stringify(CONFIG_ROOT_ID_USD)"\0"
#define ROOT_DEV_ID_EXT_SD    __stringify(CONFIG_ROOT_ID_EXT_SD)"\0"
#endif


#if defined(CONFIG_TARGET_MX6SECO_UQ7_962)
#define BOOT_DEV_ID_EMMC      __stringify(CONFIG_BOOT_ID_EMMC)"\0"
#define BOOT_DEV_ID_U_SD      "0"
#define BOOT_DEV_ID_EXT_SD    __stringify(CONFIG_BOOT_ID_EXT_SD)"\0"
#define BOOT_DEV_ID_SPI       "0"
#define BOOT_DEV_ID_SATA      "0"
#define BOOT_DEV_ID_USB       "0"

#define ROOT_DEV_ID_EMMC      __stringify(CONFIG_ROOT_ID_EMMC)"\0"
#define ROOT_DEV_ID_U_SD      "0"
#define ROOT_DEV_ID_EXT_SD    __stringify(CONFIG_ROOT_ID_EXT_SD)"\0"
#endif


#if defined(CONFIG_TARGET_MX6SECO_USBC_A62)
#define BOOT_DEV_ID_EMMC      __stringify(CONFIG_BOOT_ID_EMMC)"\0"
#define BOOT_DEV_ID_U_SD      "0"
#define BOOT_DEV_ID_EXT_SD    __stringify(CONFIG_BOOT_ID_EXT_SD)"\0"
#define BOOT_DEV_ID_SPI       "0"
#define BOOT_DEV_ID_SATA      "0"
#define BOOT_DEV_ID_USB       "0"

#define ROOT_DEV_ID_EMMC      __stringify(CONFIG_ROOT_ID_EMMC)"\0"
#define ROOT_DEV_ID_U_SD      __stringify(CONFIG_ROOT_ID_USD)"\0"
#define ROOT_DEV_ID_EXT_SD    "3"
#endif

#if defined(CONFIG_TARGET_MX6SECO_USBC_984)
#define BOOT_DEV_ID_EMMC      __stringify(CONFIG_BOOT_ID_EMMC)"\0" 
#define BOOT_DEV_ID_U_SD      "0" 
#define BOOT_DEV_ID_EXT_SD    "0"
#define BOOT_DEV_ID_SPI       "0"
#define BOOT_DEV_ID_SATA      "0"
#define BOOT_DEV_ID_USB       "0"

#define ROOT_DEV_ID_EMMC      __stringify(CONFIG_ROOT_ID_EMMC)"\0"
#define ROOT_DEV_ID_U_SD      "0"
#define ROOT_DEV_ID_EXT_SD    "0"
#endif

#if defined(CONFIG_TARGET_MX6SECO_UQ7_J_A75)
#define BOOT_DEV_ID_EMMC      __stringify(CONFIG_BOOT_ID_EMMC)"\0"
#define BOOT_DEV_ID_U_SD      "0"
#define BOOT_DEV_ID_EXT_SD    __stringify(CONFIG_BOOT_ID_EXT_SD)"\0"
#define BOOT_DEV_ID_SPI       "0"
#define BOOT_DEV_ID_SATA      "0"
#define BOOT_DEV_ID_USB       "0"

#define ROOT_DEV_ID_EMMC      __stringify(CONFIG_ROOT_ID_EMMC)"\0"
#define ROOT_DEV_ID_U_SD      "0"
#define ROOT_DEV_ID_EXT_SD    __stringify(CONFIG_ROOT_ID_EXT_SD)"\0"
#endif


#define LOAD_ADDR_KERNEL_LOCAL_DEV    __stringify(CONFIG_LOADADDR)"\0"
#define LOAD_ADDR_KERNEL_REMOTE_DEV   "0x12000000"

#define LOAD_ADDR_FDT_LOCAL_DEV       __stringify(CONFIG_FDT_LOADADDR)"\0"
#define LOAD_ADDR_FDT_REMOTE_DEV      "0x18000000"

#define SPI_LOAD_ADDRESS              __stringify(CONFIG_SPI_KERNEL_LOADADDR)"\0"
#define SPI_LOAD_LEN                  __stringify(CONFIG_SPI_KERNEL_LEN)"\0"




/*  __________________________________________________________________________
 * |                                                                          |
 * |                            BOARD SPECIFICATION                           |
 * |__________________________________________________________________________|
 */
/*  __________________________________________________________________________
 * |__________________________ QUADMO747_iMX6 (928) __________________________|
 */
#if defined(CONFIG_TARGET_MX6SECO_Q7_928)
static data_boot_dev_t kern_dev_list [] = {
	{ DEV_EMMC,     "eMMC onboard",   ENV_KERNEL_SRC_MMC,    BOOT_DEV_ID_EMMC,    LOAD_ADDR_KERNEL_LOCAL_DEV,   "zImage" },
	{ DEV_U_SD,     "uSD onboard",    ENV_KERNEL_SRC_USD,    BOOT_DEV_ID_U_SD,    LOAD_ADDR_KERNEL_LOCAL_DEV,   "zImage" },
	{ DEV_EXT_SD,   "external SD",    ENV_KERNEL_SRC_EXTSD,  BOOT_DEV_ID_EXT_SD,  LOAD_ADDR_KERNEL_LOCAL_DEV,   "zImage" },
	{ DEV_SPI,      "SPI onboard",    ENV_KERNEL_SRC_SPI,    BOOT_DEV_ID_SPI,     LOAD_ADDR_KERNEL_LOCAL_DEV,   "zImage" },
	{ DEV_TFTP,     "TFTP",           ENV_KERNEL_SRC_TFTP,   "",                  LOAD_ADDR_KERNEL_REMOTE_DEV,  "zImage" },
	{ DEV_USB,      "USB",            ENV_KERNEL_SRC_USB,    BOOT_DEV_ID_USB,     LOAD_ADDR_KERNEL_LOCAL_DEV,   "zImage" },
#ifdef CONFIG_MX6Q
	{ DEV_SATA,     "SATA",           ENV_KERNEL_SRC_SATA,   BOOT_DEV_ID_SATA,    LOAD_ADDR_KERNEL_LOCAL_DEV,   "zImage" },
#endif
};


static data_boot_dev_t fdt_dev_list [] = {
	{ DEV_EMMC,     "eMMC onboard",   ENV_FDT_SRC_MMC,     BOOT_DEV_ID_EMMC,    LOAD_ADDR_FDT_LOCAL_DEV,   CONFIG_DEFAULT_FDT_FILE },
	{ DEV_U_SD,     "uSD onboard",    ENV_FDT_SRC_USD,     BOOT_DEV_ID_U_SD,    LOAD_ADDR_FDT_LOCAL_DEV,   CONFIG_DEFAULT_FDT_FILE },
	{ DEV_EXT_SD,   "external SD",    ENV_FDT_SRC_EXTSD,   BOOT_DEV_ID_EXT_SD,  LOAD_ADDR_FDT_LOCAL_DEV,   CONFIG_DEFAULT_FDT_FILE },
	{ DEV_SPI,      "SPI onboard",    ENV_FDT_SRC_SPI,     BOOT_DEV_ID_SPI,     LOAD_ADDR_FDT_LOCAL_DEV,        ""  },
	{ DEV_TFTP,     "TFTP",           ENV_FDT_SRC_TFTP,    "",                  LOAD_ADDR_FDT_REMOTE_DEV,  CONFIG_DEFAULT_FDT_FILE },
	{ DEV_USB,      "USB",            ENV_FDT_SRC_USB,     BOOT_DEV_ID_USB,     LOAD_ADDR_FDT_LOCAL_DEV,   CONFIG_DEFAULT_FDT_FILE },
#ifdef CONFIG_MX6Q
	{ DEV_SATA,     "SATA",           ENV_FDT_SRC_SATA,    BOOT_DEV_ID_SATA,    LOAD_ADDR_FDT_LOCAL_DEV,   CONFIG_DEFAULT_FDT_FILE },
#endif
};


static data_boot_dev_t filesystem_dev_list [] = {
	{ DEV_EMMC,     "eMMC onboard",   ENV_FS_SRC_MMC,     ROOT_DEV_ID_EMMC,    "" },
	{ DEV_U_SD,     "uSD onboard",    ENV_FS_SRC_USD,     ROOT_DEV_ID_U_SD,    "" },
	{ DEV_EXT_SD,   "external SD",    ENV_FS_SRC_EXTSD,   ROOT_DEV_ID_EXT_SD,  "" },
	{ DEV_NFS,      "NFS",            ENV_FS_SRC_NFS,     "",                  "" },
	{ DEV_USB,      "USB",            ENV_FS_SRC_USB,     "",                  "" },
	{ DEV_SATA,     "SATA",           ENV_FS_SRC_SATA,    "",                  "" },
};
#endif   /*  defined(CONFIG_TARGET_MX6SECO_Q7_928)  */

/*  __________________________________________________________________________
 * |______________________________ uQ7_iMX6 (962) ____________________________|
 */
#if defined(CONFIG_TARGET_MX6SECO_UQ7_962)
static data_boot_dev_t kern_dev_list [] = {
	{ DEV_EMMC,     "eMMC onboard",   ENV_KERNEL_SRC_MMC,    BOOT_DEV_ID_EMMC,    LOAD_ADDR_KERNEL_LOCAL_DEV,   "zImage" },
	{ DEV_EXT_SD,   "external SD",    ENV_KERNEL_SRC_EXTSD,  BOOT_DEV_ID_EXT_SD,  LOAD_ADDR_KERNEL_LOCAL_DEV,   "zImage" },
	{ DEV_SPI,      "SPI onboard",    ENV_KERNEL_SRC_SPI,    BOOT_DEV_ID_SPI,     LOAD_ADDR_KERNEL_LOCAL_DEV,   "zImage" },
	{ DEV_TFTP,     "TFTP",           ENV_KERNEL_SRC_TFTP,   "",                  LOAD_ADDR_KERNEL_REMOTE_DEV,  "zImage" },
	{ DEV_USB,      "USB",            ENV_KERNEL_SRC_USB,    BOOT_DEV_ID_USB,     LOAD_ADDR_KERNEL_LOCAL_DEV,   "zImage" },
#ifdef CONFIG_MX6Q
	{ DEV_SATA,     "SATA",           ENV_KERNEL_SRC_SATA,   BOOT_DEV_ID_SATA,    LOAD_ADDR_KERNEL_LOCAL_DEV,   "zImage" },
#endif
};


static data_boot_dev_t fdt_dev_list [] = {
	{ DEV_EMMC,     "eMMC onboard",   ENV_FDT_SRC_MMC,     BOOT_DEV_ID_EMMC,    LOAD_ADDR_FDT_LOCAL_DEV,   CONFIG_DEFAULT_FDT_FILE },
	{ DEV_EXT_SD,   "external SD",    ENV_FDT_SRC_EXTSD,   BOOT_DEV_ID_EXT_SD,  LOAD_ADDR_FDT_LOCAL_DEV,   CONFIG_DEFAULT_FDT_FILE },
	{ DEV_SPI,      "SPI onboard",    ENV_FDT_SRC_SPI,     BOOT_DEV_ID_SPI,     LOAD_ADDR_FDT_LOCAL_DEV,        ""  },
	{ DEV_TFTP,     "TFTP",           ENV_FDT_SRC_TFTP,    "",                  LOAD_ADDR_FDT_REMOTE_DEV,  CONFIG_DEFAULT_FDT_FILE },
	{ DEV_USB,      "USB",            ENV_FDT_SRC_USB,     BOOT_DEV_ID_USB,     LOAD_ADDR_FDT_LOCAL_DEV,   CONFIG_DEFAULT_FDT_FILE },
#ifdef CONFIG_MX6Q
	{ DEV_SATA,     "SATA",           ENV_FDT_SRC_SATA,    BOOT_DEV_ID_SATA,    LOAD_ADDR_FDT_LOCAL_DEV,   CONFIG_DEFAULT_FDT_FILE },
#endif
};


static data_boot_dev_t filesystem_dev_list [] = {
	{ DEV_EMMC,     "eMMC onboard",   ENV_FS_SRC_MMC,     ROOT_DEV_ID_EMMC,    "" },
	{ DEV_EXT_SD,   "external SD",    ENV_FS_SRC_EXTSD,   ROOT_DEV_ID_EXT_SD,  "" },
	{ DEV_NFS,      "NFS",            ENV_FS_SRC_NFS,     "",                  "" },
	{ DEV_USB,      "USB",            ENV_FS_SRC_USB,     "",                  "" },
	{ DEV_SATA,     "SATA",           ENV_FS_SRC_SATA,    "",                  "" },
};
#endif   /*  defined(CONFIG_TARGET_MX6SECO_UQ7_962)  */


/*  __________________________________________________________________________
 * |_____________________________ USBC_iMX6 (A62) ____________________________|
 */
#if defined (CONFIG_TARGET_MX6SECO_USBC_A62)
static data_boot_dev_t kern_dev_list [] = {
	{ DEV_EMMC,     "eMMC onboard",   ENV_KERNEL_SRC_MMC,    BOOT_DEV_ID_EMMC,    LOAD_ADDR_KERNEL_LOCAL_DEV,   "zImage" },
	{ DEV_EXT_SD,   "external SD",  ENV_KERNEL_SRC_USD,    BOOT_DEV_ID_EXT_SD,  LOAD_ADDR_KERNEL_LOCAL_DEV,   "zImage" },
	{ DEV_SPI,      "SPI onboard",    ENV_KERNEL_SRC_SPI,    BOOT_DEV_ID_SPI,     LOAD_ADDR_KERNEL_LOCAL_DEV,   "zImage" },
	{ DEV_TFTP,     "TFTP",           ENV_KERNEL_SRC_TFTP,   "",                  LOAD_ADDR_KERNEL_REMOTE_DEV,  "zImage" },
	{ DEV_USB,      "USB",            ENV_KERNEL_SRC_USB,    BOOT_DEV_ID_USB,     LOAD_ADDR_KERNEL_LOCAL_DEV,   "zImage" },
#ifdef CONFIG_MX6Q
	{ DEV_SATA,     "SATA",           ENV_KERNEL_SRC_SATA,   BOOT_DEV_ID_SATA,    LOAD_ADDR_KERNEL_LOCAL_DEV,   "zImage" },
#endif
};


static data_boot_dev_t fdt_dev_list [] = {
	{ DEV_EMMC,     "eMMC onboard",   ENV_FDT_SRC_MMC,     BOOT_DEV_ID_EMMC,    LOAD_ADDR_FDT_LOCAL_DEV,   CONFIG_DEFAULT_FDT_FILE },
	{ DEV_EXT_SD,   "external SD",    ENV_FDT_SRC_USD,     BOOT_DEV_ID_EXT_SD,  LOAD_ADDR_FDT_LOCAL_DEV,   CONFIG_DEFAULT_FDT_FILE },
	{ DEV_SPI,      "SPI onboard",    ENV_FDT_SRC_SPI,     BOOT_DEV_ID_SPI,     LOAD_ADDR_FDT_LOCAL_DEV,        ""  },
	{ DEV_TFTP,     "TFTP",           ENV_FDT_SRC_TFTP,    "",                  LOAD_ADDR_FDT_REMOTE_DEV,  CONFIG_DEFAULT_FDT_FILE },
	{ DEV_USB,      "USB",            ENV_FDT_SRC_USB,     BOOT_DEV_ID_USB,     LOAD_ADDR_FDT_LOCAL_DEV,   CONFIG_DEFAULT_FDT_FILE },
#ifdef CONFIG_MX6Q
	{ DEV_SATA,     "SATA",           ENV_FDT_SRC_SATA,    BOOT_DEV_ID_SATA,    LOAD_ADDR_FDT_LOCAL_DEV,   CONFIG_DEFAULT_FDT_FILE },
#endif
};


static data_boot_dev_t filesystem_dev_list [] = {
	{ DEV_EMMC,     "eMMC onboard",   ENV_FS_SRC_MMC,     ROOT_DEV_ID_EMMC,    "" },
	{ DEV_U_SD,     "external SD",    ENV_FS_SRC_USD,     ROOT_DEV_ID_U_SD,    "" },
	{ DEV_NFS,      "NFS",            ENV_FS_SRC_NFS,     "",                  "" },
	{ DEV_USB,      "USB",            ENV_FS_SRC_USB,     "",                  "" },
	{ DEV_SATA,     "SATA",           ENV_FS_SRC_SATA,    "",                  "" },
};
#endif   /*  defined(CONFIG_TARGET_MX6SECO_USBC_A62)  */

/*  __________________________________________________________________________
 * |______________________________ SBC_iMX6 (984) ____________________________|
 */
#if defined (CONFIG_TARGET_MX6SECO_USBC_984)
static data_boot_dev_t kern_dev_list [] = {
	{ DEV_EMMC,     "eMMC onboard",   ENV_KERNEL_SRC_MMC,    BOOT_DEV_ID_EMMC,    LOAD_ADDR_KERNEL_LOCAL_DEV,   "zImage" },
	{ DEV_SPI,      "SPI onboard",    ENV_KERNEL_SRC_SPI,    BOOT_DEV_ID_SPI,     LOAD_ADDR_KERNEL_LOCAL_DEV,   "zImage" },
	{ DEV_TFTP,     "TFTP",           ENV_KERNEL_SRC_TFTP,   "",                  LOAD_ADDR_KERNEL_REMOTE_DEV,  "zImage" },
	{ DEV_USB,      "USB",            ENV_KERNEL_SRC_USB,    BOOT_DEV_ID_USB,     LOAD_ADDR_KERNEL_LOCAL_DEV,   "zImage" },
#ifdef CONFIG_MX6Q
	{ DEV_SATA,     "SATA",           ENV_KERNEL_SRC_SATA,   BOOT_DEV_ID_SATA,    LOAD_ADDR_KERNEL_LOCAL_DEV,   "zImage" },
#endif
};


static data_boot_dev_t fdt_dev_list [] = {
	{ DEV_EMMC,     "eMMC onboard",   ENV_FDT_SRC_MMC,     BOOT_DEV_ID_EMMC,    LOAD_ADDR_FDT_LOCAL_DEV,   CONFIG_DEFAULT_FDT_FILE },
	{ DEV_SPI,      "SPI onboard",    ENV_FDT_SRC_SPI,     BOOT_DEV_ID_SPI,     LOAD_ADDR_FDT_LOCAL_DEV,        ""  },
	{ DEV_TFTP,     "TFTP",           ENV_FDT_SRC_TFTP,    "",                  LOAD_ADDR_FDT_REMOTE_DEV,  CONFIG_DEFAULT_FDT_FILE },
	{ DEV_USB,      "USB",            ENV_FDT_SRC_USB,     BOOT_DEV_ID_USB,     LOAD_ADDR_FDT_LOCAL_DEV,   CONFIG_DEFAULT_FDT_FILE },
#ifdef CONFIG_MX6Q
	{ DEV_SATA,     "SATA",           ENV_FDT_SRC_SATA,    BOOT_DEV_ID_SATA,    LOAD_ADDR_FDT_LOCAL_DEV,   CONFIG_DEFAULT_FDT_FILE },
#endif
};


static data_boot_dev_t filesystem_dev_list [] = {
	{ DEV_EMMC,     "eMMC onboard",   ENV_FS_SRC_MMC,     ROOT_DEV_ID_EMMC,    "" },
	{ DEV_NFS,      "NFS",            ENV_FS_SRC_NFS,     "",                  "" },
	{ DEV_USB,      "USB",            ENV_FS_SRC_USB,     "",                  "" },
#ifdef CONFIG_MX6Q
	{ DEV_SATA,     "SATA",           ENV_FS_SRC_SATA,    "",                  "" },
#endif
};
#endif   /* defined(CONFIG_TARGET_MX6SECO_USBC_984)  */



/*  __________________________________________________________________________
 * |______________________________ uQ7_iMX6 (A75) ____________________________|
 */
#if defined(CONFIG_TARGET_MX6SECO_UQ7_J_A75)
static data_boot_dev_t kern_dev_list [] = {
	{ DEV_EMMC,     "eMMC onboard",   ENV_KERNEL_SRC_MMC,    BOOT_DEV_ID_EMMC,    LOAD_ADDR_KERNEL_LOCAL_DEV,   "zImage" },
	{ DEV_EXT_SD,   "external SD",    ENV_KERNEL_SRC_EXTSD,  BOOT_DEV_ID_EXT_SD,  LOAD_ADDR_KERNEL_LOCAL_DEV,   "zImage" },
	{ DEV_SPI,      "SPI onboard",    ENV_KERNEL_SRC_SPI,    BOOT_DEV_ID_SPI,     LOAD_ADDR_KERNEL_LOCAL_DEV,   "zImage" },
	{ DEV_TFTP,     "TFTP",           ENV_KERNEL_SRC_TFTP,   "",                  LOAD_ADDR_KERNEL_REMOTE_DEV,  "zImage" },
	{ DEV_USB,      "USB",            ENV_KERNEL_SRC_USB,    BOOT_DEV_ID_USB,     LOAD_ADDR_KERNEL_LOCAL_DEV,   "zImage" },
#ifdef CONFIG_MX6Q
	{ DEV_SATA,     "SATA",           ENV_KERNEL_SRC_SATA,   BOOT_DEV_ID_SATA,    LOAD_ADDR_KERNEL_LOCAL_DEV,   "zImage" },
#endif
};


static data_boot_dev_t fdt_dev_list [] = {
	{ DEV_EMMC,     "eMMC onboard",   ENV_FDT_SRC_MMC,     BOOT_DEV_ID_EMMC,    LOAD_ADDR_FDT_LOCAL_DEV,   CONFIG_DEFAULT_FDT_FILE },
	{ DEV_EXT_SD,   "external SD",    ENV_FDT_SRC_EXTSD,   BOOT_DEV_ID_EXT_SD,  LOAD_ADDR_FDT_LOCAL_DEV,   CONFIG_DEFAULT_FDT_FILE },
	{ DEV_SPI,      "SPI onboard",    ENV_FDT_SRC_SPI,     BOOT_DEV_ID_SPI,     LOAD_ADDR_FDT_LOCAL_DEV,        ""  },
	{ DEV_TFTP,     "TFTP",           ENV_FDT_SRC_TFTP,    "",                  LOAD_ADDR_FDT_REMOTE_DEV,  CONFIG_DEFAULT_FDT_FILE },
	{ DEV_USB,      "USB",            ENV_FDT_SRC_USB,     BOOT_DEV_ID_USB,     LOAD_ADDR_FDT_LOCAL_DEV,   CONFIG_DEFAULT_FDT_FILE },
#ifdef CONFIG_MX6Q
	{ DEV_SATA,     "SATA",           ENV_FDT_SRC_SATA,    BOOT_DEV_ID_SATA,    LOAD_ADDR_FDT_LOCAL_DEV,   CONFIG_DEFAULT_FDT_FILE },
#endif
};


static data_boot_dev_t filesystem_dev_list [] = {
	{ DEV_EMMC,     "eMMC onboard",   ENV_FS_SRC_MMC,     ROOT_DEV_ID_EMMC,    "" },
	{ DEV_EXT_SD,   "external SD",    ENV_FS_SRC_EXTSD,   ROOT_DEV_ID_EXT_SD,  "" },
	{ DEV_NFS,      "NFS",            ENV_FS_SRC_NFS,     "",                  "" },
	{ DEV_USB,      "USB",            ENV_FS_SRC_USB,     "",                  "" },
	{ DEV_SATA,     "SATA",           ENV_FS_SRC_SATA,    "",                  "" },
};
#endif   /*  defined(CONFIG_TARGET_MX6SECO_UQ7_J_A75)  */

/*  __________________________________________________________________________
 * |__________________________________________________________________________|
 */

/*  ___________________________________________________________________________
 * |				SERIAL_DEV SPECIFICATION		      /
   /__________________________________________________________________________|
  */
 
#if defined CONFIG_TARGET_MX6SECO_Q7_928 || CONFIG_TARGET_MX6SECO_UQ7_962 || CONFIG_TARGET_MX6SECO_UQ7_J_A75
#define SERIAL_DEV_PRIMARY_UART       		1
#define SERIAL_DEV_PRIMARY_FLEXCAN        	2

typedef struct serial_mode {
    char *label;
    int dev_uart;
    int dev_flexcan;
    int primary;
} serial_mode_t;

static serial_mode_t serial_mode_list [] = {
    { "FLEXCAN",		 	-1,  0,  SERIAL_DEV_PRIMARY_FLEXCAN },
    { "UART",           	 	 0, -1,  SERIAL_DEV_PRIMARY_UART },
};

 /*  ___________________________________________________________________________
  * |				AUDIO_CODEC SPECIFICATION		       /
    /__________________________________________________________________________|
 */

#define AUDIO_CODEC_PRIMARY_AC97_STANDARD       	1
#define AUDIO_CODEC_PRIMARY_SGTL5000	        	2

typedef struct codec_mode {
    char *label;
    int codec_ac97_standard;
    int codec_sgtl5000;
    int primary;
} codec_mode_t;

static codec_mode_t codec_mode_list [] = {
    { "SGTL5000 AUDIO",		 		-1,  0,  AUDIO_CODEC_PRIMARY_SGTL5000 },
    { "AC97_Standard ",		           	 0, -1,  AUDIO_CODEC_PRIMARY_AC97_STANDARD },
};

#endif

/*  __________________________________________________________________________
 * |                                                                          |
 * |                            VIDEO SPECIFICATION                           |
 * |__________________________________________________________________________|
 */

#define VIDEO_PRIMARY_HDMI   1
#define VIDEO_PRIMARY_LVDS   2

typedef struct video_mode {
	char *label;
	int fb_hdmi;
	int fb_lvds1;
	int fb_lvds2;
	int primary;
} video_mode_t;


typedef struct lvds_video_spec {
	char *label;
	char *name;
	char *if_map;
	char *opt;
	int  num_channel;
} lvds_video_spec_t;


static video_mode_t video_mode_list [] = {
	{ "Primary LVDS Only",  -1,  0, -1, VIDEO_PRIMARY_LVDS },
	{ "HDMI Only",           0, -1, -1, VIDEO_PRIMARY_HDMI },
	{ "LVDS + HDMI",         1,  0, -1, VIDEO_PRIMARY_LVDS },
	{ "HDMI + LVDS",         0,  1, -1, VIDEO_PRIMARY_HDMI },
	{ "LVDS + LVDS",        -1,  0,  1, VIDEO_PRIMARY_LVDS },
	{ "LVDS + LVDS + HDMI",  2,  0,  1, VIDEO_PRIMARY_LVDS },
	{ "HDMI + LVDS + LVDS",  0,  1,  2, VIDEO_PRIMARY_HDMI },
};


static lvds_video_spec_t lvds_video_spec_list [] = {
	{ "WVGA	  [800x480]",   "LDB-WVGA",    "RGB666",      "datamap=spwg",    1 },
	{ "SVGA	  [800x600]",   "LDB-SVGA",    "RGB666",      "datamap=spwg",    1 },
	{ "XGA	  [1024x768]",  "LDB-XGA",     "RGB666",      "datamap=spwg",    1 },
	{ "WXGA	  [1368x768]",  "LDB-WXGA",    "RGB24",       "datamap=spwg",    1 },
	{ "WXGAP60[1280x800]",  "LDB-1280P60", "RGB24,bpp=32","datamap=spwg",    1 },
	{ "SXGA	  [1280x1024]", "LDB-SXGA",    "RGB24",       "datamap=spwg",    1 },
	{ "HD1080 [1920x1080]", "LDB-1080P60", "RGB24",  "ldb=spl0", 2 },
};



/*  __________________________________________________________________________
 * |__________________________________________________________________________|
 */


typedef struct var_env {
	char *name;
	char *data;
} var_env_t;


static var_env_t env_base [] = {
	/*  Basic Bootargs  */
//	{ "console_interface" , CONSOLE_INTERFACE },
//	{ "bootargs_base"     , CONFIG_BOOTARGS_BASE },
	/*  for FS booting from NFS  */
	{ "set_ipconf_no_dhcp" , SET_IPCONF_NO_DHCP },
	{ "set_ipconf_dhcp"    , SET_IPCONF_DHCP },
	{ "set_ip"             , SET_IP },
	{ "set_nfsroot"        , SET_NFSROOT },
};


int atoi (char *string)
{
	int length;
	int retval = 0;
	int i;
	int sign = 1;

	length = strlen(string);
	for (i = 0; i < length; i++) {
		if (0 == i && string[0] == '-') {
			sign = -1;
			continue;
		}
		if (string[i] > '9' || string[i] < '0') {
			break;
		}
		retval *= 10;
		retval += string[i] - '0';
	}
	retval *= sign;
	return retval;
}

int ctoi (char ch) {
	int retval = 0;
	if (ch <= '9' && ch >= '0') {
		retval = ch - '0';
	}
	return retval;
}

static char *getline (void) {
	static char buffer[100];
	char c;
	size_t i;

	i = 0;
	while (1) {
		buffer[i] = '\0';
		while (!tstc()){
			WATCHDOG_RESET();
			continue;
		}

		c = getc();
		/* Convert to uppercase */
		//if (c >= 'a' && c <= 'z')
		//	c -= ('a' - 'A');

		switch (c) {
			case '\r':	/* Enter/Return key */
			case '\n':
				puts("\n");
				return buffer;

			case 0x03:	/* ^C - break */
				return NULL;

			case 0x08:	/* ^H  - backspace */
			case 0x7F:	/* DEL - backspace */
				if (i) {
					puts("\b \b");
					i--;
				}
				break;

			default:
				/* Ignore control characters */
				if (c < 0x20)
					break;
				/* Queue up all other characters */
				buffer[i++] = c;
				printf("%c", c);
				break;
		}
	}
}


void create_environment (const var_env_t *env_list, int num_element) {
	int i;
	for ( i = 0 ; i < num_element ; i++ ) {
		setenv (env_list[i].name, env_list[i].data);
	}
}



/*  __________________________________________________________________________
 * |                                                                          |
 * |                            RAM Size Selection                            |
 * |__________________________________________________________________________|
 */
char *do_ramsize (ulong min, ulong max) {
	char *line;
	do {printf ("\n __________________________________________________");
		printf ("\n       Chose Kernel RAM size to allocate\n");
		printf (" __________________________________________________\n");
		printf ("[min size: %luM - max size %luM]\n", min, max);
		printf ("> ");
		line = getline ();
	}while ((ulong)atoi(line) < min || (ulong)atoi(line) > max);
	printf ("Will use %luM of RAM memory\n",(ulong)atoi(line));
	return line;
}



/*  __________________________________________________________________________
 * |                                                                          |
 * |                Kernel/FDT/FileSystem Source Selection                    |
 * |__________________________________________________________________________|
 */
#define MIN_PARTITION_ID 1
#define MAX_PARTITION_ID 9

#ifdef CONFIG_TARGET_MX6SECO_Q7_928
	#if defined(CONFIG_MX6Q)
	#define MAX_DEVICE 4
	#else
	#define MAX_DEVICE 3
	#endif
#elif defined CONFIG_TARGET_MX6SECO_UQ7_962
	#if defined(CONFIG_MX6Q)
	#define MAX_DEVICE 3
	#else
	#define MAX_DEVICE 2
	#endif
#elif defined CONFIG_TARGET_MX6SECO_USBC_A62
	#if defined(CONFIG_MX6Q)
	#define MAX_DEVICE 2
	#else
	#define MAX_DEVICE 1
	#endif
#elif defined CONFIG_TARGET_MX6SECO_USBC_984
	#if defined(CONFIG_MX6Q)
	#define MAX_DEVICE 2
	#else
	#define MAX_DEVICE 1
	#endif
#elif defined CONFIG_TARGET_MX6SECO_UQ7_J_A75
	#if defined(CONFIG_MX6Q)
	#define MAX_DEVICE 3
	#else
	#define MAX_DEVICE 2
	#endif

#endif


int selection_dev (char *scope, data_boot_dev_t dev_list[], int num_element) {
	char ch;
	int i, selection = 0;

	do {
		printf ("\n __________________________________________________");
		printf ("\n            Chose boot Device for %s.\n", scope);
		printf (" __________________________________________________\n");
		for ( i = 0 ; i < num_element ; i++ ) {
			printf ("%d) %s\n", i+1, dev_list[i].label);
		}
		printf ("> ");
		ch = getc ();
		printf ("%c\n", ch);
	} while ((ctoi(ch) < 1) || (ctoi(ch) > num_element));
	
	selection = ctoi(ch) - 1;

	return selection;
}


void select_partition_id (char *partition_id) {
	char ch;

	do {
		printf ("Chose the partition\n");
		printf ("> ");
		ch = getc ();
		printf ("%c\n", ch);
	} while (ctoi(ch) < MIN_PARTITION_ID || ctoi(ch) > MAX_PARTITION_ID);

	sprintf (partition_id, "%c", ch);
}


void select_source_path (char *source_path, char *subject, char *default_path) {
	char *line;

	printf ("Path of the %s (enter for default %s) > ", subject, default_path);
	line = getline ();

	strcpy (source_path, strcmp (line, "") == 0 ? default_path : line);
}


void select_spi_parameters (char *spi_load_addr, char *spi_load_len) {
	char *line;

	printf ("Load addres of the SPI device (enter for default %s) > ", SPI_LOAD_ADDRESS);
        line = getline ();
	*spi_load_addr = strcmp (line, "") == 0 ? (ulong)SPI_LOAD_ADDRESS : (ulong)atoi(line);

	printf ("Size of data to read (enter for default %s) > ", SPI_LOAD_LEN);
        line = getline ();
	*spi_load_len = strcmp (line, "") == 0 ? (ulong)SPI_LOAD_LEN : (ulong)atoi(line);

	free (line);
}


void select_tftp_parameters (char *serverip_tftp , char *ipaddr_tftp) {
	char *line;
	static char str[20];
	char *pstr;
	
	printf ("\n ________________________________________________________\n");
	printf (" Select TFTP as source for kernel/FDT, please set the net\n");
	printf (" ________________________________________________________\n");

	do {
		pstr = getenv ("serverip");
		if ( pstr == NULL ) {
			strcpy (str, DEF_TFTP_SERVER_IP);
			pstr = &str[0];
		}
		printf ("\nInsert the address ip of the tftp server (enter for current: %s)\n", 
				pstr);
		printf ("> ");
                line = getline ();
                printf ("%s\n", line);
        } while (0);
	strcpy (serverip_tftp, strcmp (line, "") == 0 ? pstr : line);

	do {
		pstr = getenv ("ipaddr");
		if ( pstr == NULL ) {
			strcpy (str, DEF_TFTP_CLIENT_IP);
			pstr = &str[0];
		}
		printf ("\nInsert the address ip of this tftp client (enter for current: %s)\n",
				pstr);
		printf ("> ");
                line = getline ();
                printf ("%s\n", line);
        } while (0);
	strcpy (ipaddr_tftp, strcmp (line, "") == 0 ? pstr : line);
}


int select_kernel_source (char *partition_id, char *file_name,
			char *spi_load_addr, char *spi_load_len, int *use_tftp) {

	char *str;
	int dev_selected = selection_dev ("Kernel", kern_dev_list, ARRAY_SIZE(kern_dev_list));

	switch ( kern_dev_list[dev_selected].dev_type ) {
		case DEV_EMMC:
		case DEV_U_SD:
		case DEV_EXT_SD:
		case DEV_SATA:
		case DEV_USB:
			select_partition_id (partition_id);
			break;
		case DEV_SPI:
			select_spi_parameters (spi_load_addr, spi_load_len);
			break;
		case DEV_TFTP:
			*use_tftp = 1;
			break;
		default:
			break;
	}

	str = GET_KERNEL_PATH;
	if ( str != NULL )
		select_source_path (file_name, "Kernel", str);
	else
		select_source_path (file_name, "Kernel", kern_dev_list[dev_selected].def_path);

	return dev_selected;
}


int select_fdt_source (char *partition_id, char *file_name,
			char *spi_load_addr, char *spi_load_len, int *use_tftp) {

	char *str;
	int dev_selected = selection_dev ("FDT", fdt_dev_list, ARRAY_SIZE(fdt_dev_list));

	switch ( fdt_dev_list[dev_selected].dev_type ) {
		case DEV_EMMC:
		case DEV_U_SD:
		case DEV_EXT_SD:
		case DEV_SATA:
		case DEV_USB:
			select_partition_id (partition_id);
			break;
		case DEV_SPI:
			select_spi_parameters (spi_load_addr, spi_load_len);
			break;
		case DEV_TFTP:
			*use_tftp = 1;
			break;
		default:
			break;
	}

	str = GET_FDT_PATH;
	if ( str != NULL )
		select_source_path (file_name, "FDT", str);
	else
		select_source_path (file_name, "FDT", fdt_dev_list[dev_selected].def_path);

	return dev_selected;
}


void select_nfs_parameters (char *ip_local, char *ip_server, char *nfs_path, char *netmask, int *dhcp_set) {
	char *line;
	char ch;
	static char str[30];
	char *pstr;


	/*  Set DHCP configuration  */
	do { 
		printf ("\nDo you want to use dynamic ip assignment (DHCP)? (y/n)\n");
		printf ("> ");
		ch = getc ();
	} while (ch != 'y' && ch != 'n');
	if (ch == 'y') {
		*dhcp_set = 1;
		printf ("\nYou have select to use dynamic ip\n");
	} else {
		*dhcp_set = 0;
		printf ("\nYou have select to use static ip\n");
	}


	/*  Set HOST IP  */
	do {
		pstr = GET_IP_SERVER;
		if ( pstr == NULL ) {
			strcpy (str, DEF_IP_SERVER);
			pstr = &str[0];
		}

		printf ("Insert the address ip of the host machine (enter for current: %s)\n",
				pstr);
		printf ("> ");
		line = getline ();
		printf ("%s\n", line);
	} while (0);
	strcpy (ip_server, strcmp (line, "") == 0 ? pstr : line);


	/* Set the NFS Path  */
	do {
		pstr = GET_NFS_PATH;
		if ( pstr == NULL ) {
			strcpy (str, DEF_NFS_PATH);
			pstr = &str[0];
		}

		printf ("Insert the nfs path of the host machine (enter for current: %s)\n",
				pstr);
		printf ("> ");
		line = getline ();
		printf ("%s\n", line);
	} while (0);
	strcpy (nfs_path, strcmp (line, "") == 0 ? pstr : line);


	if (*dhcp_set == 0) {
		/* Set CLIENT IP  */
		do {
			pstr = GET_IP_LOCAL;
			if ( pstr == NULL ) {
				strcpy (str, DEF_IP_LOCAL);
				pstr = &str[0];
			}

			printf ("Insert an address ip for this board (enter for current: %s)\n",
					 pstr);
			printf ("> ");
			line = getline ();
			printf ("%s\n", line);
		} while (0);
		strcpy (ip_local, strcmp (line, "") == 0 ? pstr : line);


		/* set NETMASK of the CLIENT  */
		do {
			pstr = GET_NETMASK;
			if ( pstr == NULL ) {
				strcpy (str, DEF_NETMASK);
				pstr = &str[0];
			}

			printf ("Insert the netmask (enter for current: %s)\n",
					pstr);
			printf ("> ");
			line = getline ();
			printf ("%s\n", line);
		} while (0);
		strcpy (netmask, strcmp (line, "") == 0 ? pstr : line);
	}

}

int select_filesystem_souce (char *partition_id, char *nfs_path, 
			char *serverip_nfs , char *ipaddr_nfs, char *netmask_nfs, int *use_dhcp) {

	int dev_selected = selection_dev ("FileSystem", filesystem_dev_list, ARRAY_SIZE(filesystem_dev_list));

	switch ( filesystem_dev_list[dev_selected].dev_type ) {
		case DEV_EMMC:
		case DEV_U_SD:
		case DEV_EXT_SD:
		case DEV_SATA:
		case DEV_USB:
			select_partition_id (partition_id);
			break;
		case DEV_NFS:
			select_nfs_parameters (ipaddr_nfs, serverip_nfs, nfs_path, netmask_nfs, use_dhcp);
			break;
		default:
			break;
	}

	return dev_selected;
}


/*  __________________________________________________________________________
 * |                                                                          |
 * |                         SERIAL_DEV Setting Selection                     |
 * |__________________________________________________________________________|
 */

#if defined CONFIG_TARGET_MX6SECO_Q7_928 || CONFIG_TARGET_MX6SECO_UQ7_962 || CONFIG_TARGET_MX6SECO_UQ7_J_A75

int selection_serial_mode(serial_mode_t  serial_mode_list[], int num_element) {
    char ch;
    int i, selection = 0;

do {
        printf ("\n __________________________________________________");
        printf ("\n               Choose Serial_dev Setting.\n");
        printf (" __________________________________________________\n");
        for ( i = 0 ; i < num_element ; i++ ) {
            printf ("%d) %s\n", i+1, serial_mode_list[i].label);
        }
        printf ("> ");
        ch = getc ();
        printf ("%c\n", ch);
    } while ((ctoi(ch) < 1) || (ctoi(ch) > num_element));

    selection = ctoi(ch) - 1;

    return selection;
}

/*  __________________________________________________________________________
 * |                                                                          |
 * |                         AUDIO_CODEC Setting Selection                    |
 * |__________________________________________________________________________|
 */

int selection_codec_mode(codec_mode_t  codec_mode_list[], int num_element) {
    char ch;
    int i, selection = 0;

do {
        printf ("\n __________________________________________________");
        printf ("\n               Choose AUDIO_CODEC Setting.\n");
        printf (" __________________________________________________\n");
        for ( i = 0 ; i < num_element ; i++ ) {
            printf ("%d) %s\n", i+1, codec_mode_list[i].label);
        }
        printf ("> ");
        ch = getc ();
        printf ("%c\n", ch);
    } while ((ctoi(ch) < 1) || (ctoi(ch) > num_element));

    selection = ctoi(ch) - 1;

    return selection;
}
#endif

/*  __________________________________________________________________________
 * |                                                                          |
 * |                         VIDEO Settings Selection                         |
 * |__________________________________________________________________________|
 */


int selection_video_mode (video_mode_t  video_mode_list[], int num_element) {
	char ch;
	int i, selection = 0;

	do {
		printf ("\n __________________________________________________");
		printf ("\n               Chose Video Setting.\n");
		printf (" __________________________________________________\n");
		for ( i = 0 ; i < num_element ; i++ ) {
			printf ("%d) %s\n", i+1, video_mode_list[i].label);
		}
		printf ("> ");
		ch = getc ();
		printf ("%c\n", ch);
	} while ((ctoi(ch) < 1) || (ctoi(ch) > num_element));

	selection = ctoi(ch) - 1;

	return selection;
}


int selection_lvds_spec (char *scope, lvds_video_spec_t  video_list[], int num_element) {
	char ch;
	int i, selection = 0;

	do {
		printf ("\n __________________________________________________");
		printf ("\n        Chose LVDS resolution for %s.\n", scope);
		printf (" __________________________________________________\n");
		for ( i = 0 ; i < num_element ; i++ ) {
			printf ("%d) %s\n", i+1, video_list[i].label);
		}
		printf ("> ");
		ch = getc ();
		printf ("%c\n", ch);
	} while ((ctoi(ch) < 1) || (ctoi(ch) > num_element));

	selection = ctoi(ch) - 1;

	return selection;
}


int create_lvds_video_args (char *video_args, int list_idx, int fb_idx,
			lvds_video_spec_t  video_list[], int num_element) {

	if ( (list_idx < 0) && (list_idx > (num_element - 1)) )
			return -1;

	sprintf (video_args, "video=mxcfb%d:dev=ldb,%s,if=%s %s",
			fb_idx, lvds_video_spec_list[list_idx].name,
			lvds_video_spec_list[list_idx].if_map,
			lvds_video_spec_list[list_idx].opt);
	return 0;
}




static int do_seco_config (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]) {

	/* generic  */
	int use_tftp = 0;
	char serverip_tftp[25];
	char ipaddr_tftp[25];

	int set_memory = 0;
	int set_kernel = 0;
	int set_fdt = 0;
	int set_filesystem = 0;
	int set_video = 0;

	/*  memory RAM  */
	ulong min_size = 512;
	ulong max_size;
	char memory_buff[50];
	char *line;

	/*  for kernel  */
	int kernel_selected_device;
	char kernel_part_id[2];
	char kernel_filename[100];
	char kernel_spi_load_address[20];
	char kernel_spi_load_length[20];

	/*  for fdt  */
	int fdt_selected_device;
	char fdt_part_id[2];
	char fdt_filename[100];
	char fdt_spi_load_address[20];
	char fdt_spi_load_length[20];

	/*  for filesystem  */
	int filesystem_selected_device;
	char filesystem_part_id[2];
	char filesystem_server_nfs[25];
	char filesystem_ipaddr_nfs[25];
	char filesystem_netmask_nfs[25];
	int  filesystem_use_dhcp;
	char filesystem_path_nfs[100];
	char filesystem_boot_string[300];

	/*  for video  */
	int video_mode_selection;
	int video_lvds1_selection;
	int video_lvds2_selection;
	char video_hdmi_video[80];
	char video_lvds1_video[80];
	char video_lvds2_video[80];
	char video_mode[300];

#if defined CONFIG_TARGET_MX6SECO_Q7_928 || CONFIG_TARGET_MX6SECO_UQ7_962 || CONFIG_TARGET_MX6SECO_UQ7_J_A75
 	
	/*  for serial_dev  */
	int set_serial_dev = 0;
	int serial_mode_selection;
	char serial_uart_dev[80];
	char serial_flexcan_dev[80];
	char serial_mode[300];

	/*  for audio_codec  */
	int set_audio_codec = 0;
	int codec_mode_selection;
	char codec_ac97_standard_audio[80];
	char codec_sgtl5000_audio[80];
	char codec_mode[300];

#endif

	if (argc > 2)
		return cmd_usage (cmdtp);


	if (argc == 2 && strcmp(argv[1], "help") == 0)
		return cmd_usage (cmdtp);


	if (argc == 2) {

		if (strcmp(argv[1], "default") == 0) {
			set_default_env ("## Resetting to default environment");
		}

		if (strcmp(argv[1], "memory") == 0) {
			set_memory = 1;
		}

		if (strcmp(argv[1], "ksrc") == 0) {
			set_kernel = 1;
		}

		if (strcmp(argv[1], "fdtsrc") == 0) {
			set_fdt = 1;
		}

		if (strcmp(argv[1], "fssrc") == 0) {
			set_filesystem = 1;
		}

		if (strcmp(argv[1], "video") == 0) {
			set_video = 1;
		}
#if defined CONFIG_TARGET_MX6SECO_Q7_928 || CONFIG_TARGET_MX6SECO_UQ7_962 || CONFIG_TARGET_MX6SECO_UQ7_J_A75
		
		if (strcmp(argv[1], "serial_dev") ==0) {
			set_serial_dev = 1;
		}

		if (strcmp(argv[1], "audio_codec") ==0) {
			set_audio_codec = 1;
		}

#endif

	}


	if (argc == 1) {
		set_memory = 1;
		set_kernel = 1;
		set_fdt = 1;
		set_filesystem = 1;
		set_video = 1;
#if defined CONFIG_TARGET_MX6SECO_Q7_928 || CONFIG_TARGET_MX6SECO_UQ7_962 || CONFIG_TARGET_MX6SECO_UQ7_J_A75
		set_serial_dev = 1;
		set_audio_codec = 1;
#endif
	}


/*  __________________________________________________________________________
 * |______________________________ MEMORY RAM ________________________________|
 */
#define DEFAULT_CMA_VALUE 396
	if ( set_memory ) {
		ulong size = PHYS_SDRAM_SIZE;
		max_size = size / (1 << 20); // get size in MB
		if (max_size > 2100)
			max_size = 3800;
		if ( min_size > max_size )
			min_size = max_size;
		line = do_ramsize (min_size, max_size);
		if ( (ulong)atoi(line) < DEFAULT_CMA_VALUE ) {
			sprintf (memory_buff, "mem=%sM cma=96M", line);
		} else {
			sprintf (memory_buff, "mem=%sM", line);
		}
		setenv ("memory", memory_buff);
	}

/*  __________________________________________________________________________
 * |________________________________ KERNEL __________________________________|
 */
	if ( set_kernel) {
		kernel_selected_device = select_kernel_source (kernel_part_id,
				kernel_filename, kernel_spi_load_address,
				kernel_spi_load_length, &use_tftp);

		SAVE_KERNEL_DEVICE_ID(kern_dev_list[kernel_selected_device].device_id);
		SAVE_KERNEL_LOADADDR(kern_dev_list[kernel_selected_device].load_address);

		if ( kern_dev_list[kernel_selected_device].dev_type != DEV_TFTP ) {
			SAVE_KERNEL_PATH(kernel_filename);
		}

		switch (kern_dev_list[kernel_selected_device].dev_type) {
			case DEV_EMMC:
			case DEV_U_SD:
			case DEV_EXT_SD:
			case DEV_SATA:
			case DEV_USB:
				SAVE_KERNEL_PARTITION(kernel_part_id);
				break;
			case DEV_SPI:
				SAVE_KERNEL_SPI_ADDR(kernel_spi_load_address);
				SAVE_KERNEL_SPI_LEN(kernel_spi_load_length);
				break;
			case DEV_TFTP:
				break;
			default:
				break;
		}

		SAVE_KERNEL_BOOT_LOAD(kern_dev_list[kernel_selected_device].env_str);
	}

/*  __________________________________________________________________________
 * |__________________________________ FDT ___________________________________|
*/
	if ( set_fdt ) {
		fdt_selected_device = select_fdt_source (fdt_part_id,
				fdt_filename, fdt_spi_load_address,
				fdt_spi_load_length, &use_tftp);

		SAVE_FDT_DEVICE_ID(fdt_dev_list[fdt_selected_device].device_id);
		SAVE_FDT_LOADADDR(fdt_dev_list[fdt_selected_device].load_address);

		if ( fdt_dev_list[fdt_selected_device].dev_type != DEV_TFTP ) {
			SAVE_FDT_PATH(fdt_filename);
		}

		switch (fdt_dev_list[fdt_selected_device].dev_type) {
			case DEV_EMMC:
			case DEV_U_SD:
			case DEV_EXT_SD:
			case DEV_SATA:
			case DEV_USB:
				SAVE_FDT_PARTITION(fdt_part_id);
				break;
			case DEV_SPI:
				SAVE_FDT_SPI_ADDR(fdt_spi_load_address);
				SAVE_FDT_SPI_LEN(fdt_spi_load_length);
				break;
			case DEV_TFTP:
				break;
			default:
				break;
		}

		SAVE_FDT_BOOT_LOAD(fdt_dev_list[fdt_selected_device].env_str);
	}

/*  __________________________________________________________________________
 * |_______________________________ NET OF TFTP ______________________________|
 */
	if ( set_kernel || set_fdt ) {
		if ( use_tftp ) {
			select_tftp_parameters (serverip_tftp , ipaddr_tftp);
			setenv ("serverip", serverip_tftp);
			setenv ("ipaddr", ipaddr_tftp);
		}
	}

/*  __________________________________________________________________________
 * |______________________________ FILE SYSTEM _______________________________|
 */
	if ( set_filesystem ) {
		filesystem_selected_device = select_filesystem_souce (filesystem_part_id,
				filesystem_path_nfs, filesystem_server_nfs, filesystem_ipaddr_nfs,
				filesystem_netmask_nfs, &filesystem_use_dhcp);

		switch (filesystem_dev_list[filesystem_selected_device].dev_type) {
			case DEV_EMMC:
			case DEV_U_SD:
			case DEV_EXT_SD:
				SAVE_FILESYSTEM_DEVICE_ID(filesystem_dev_list[filesystem_selected_device].device_id);
				SAVE_FILESYSTEM_PARTITION(filesystem_part_id);
				break;
			case DEV_SATA:
			case DEV_USB:
				SAVE_FILESYSTEM_PARTITION(filesystem_part_id);
				break;
			case DEV_SPI:
				SAVE_FDT_SPI_ADDR(fdt_spi_load_address);
				SAVE_FDT_SPI_LEN(fdt_spi_load_length);
				break;
			case DEV_NFS:
				SAVE_NFS_USE("1");
				SAVE_NFS_IP_CLIENT(filesystem_ipaddr_nfs);
				SAVE_NFS_IP_SERVER(filesystem_server_nfs);
				SAVE_NFS_NETMASK(filesystem_netmask_nfs);
				SAVE_NFS_PATH(filesystem_path_nfs);
				if ( filesystem_use_dhcp == 1 ) {
					SAVE_NFS_USE_DHCP("1");
				} else {
					SAVE_NFS_USE_DHCP("0");
				}
				printf ("--- %s   %s   %s    %s\n", filesystem_path_nfs, filesystem_server_nfs, filesystem_ipaddr_nfs, filesystem_netmask_nfs);
				break;
			case DEV_TFTP:
				break;
			default:
				break;
		}

		if ( filesystem_dev_list[filesystem_selected_device].dev_type != DEV_NFS )
			SAVE_NFS_USE("0");

		sprintf (filesystem_boot_string, "setenv root_dev \'%s\'",
				filesystem_dev_list[filesystem_selected_device].env_str);

		SAVE_FILESYSTEM_ROOT(filesystem_boot_string);
	}


	/*  _______________________________________________________________________________
	 * |_____________________________ SERIAL_DEV SETTING ______________________________|
	 */
	
#if defined CONFIG_TARGET_MX6SECO_Q7_928 || CONFIG_TARGET_MX6SECO_UQ7_962 || CONFIG_TARGET_MX6SECO_UQ7_J_A75

	if ( set_serial_dev ) {
			serial_mode_selection =  selection_serial_mode (serial_mode_list,
					ARRAY_SIZE(serial_mode_list));	

		if ( serial_mode_list [serial_mode_selection].dev_uart >= 0 ) {
			sprintf (serial_uart_dev, "serial_dev=uart");

	
		} else
			sprintf (serial_uart_dev, " ");

		if (serial_mode_list [serial_mode_selection].dev_flexcan >= 0 ) {
			sprintf (serial_flexcan_dev, "serial_dev=flexcan");

		} else
			sprintf (serial_flexcan_dev, " ");
		
		sprintf (serial_mode, "%s%s", serial_uart_dev, serial_flexcan_dev);

		if (strcmp (serial_uart_dev, " ") ) {
			setenv ("serial_uart", serial_uart_dev);
		}
		
		if (strcmp (serial_flexcan_dev, " ") ) {
			setenv ("serial_flexcan", serial_flexcan_dev);
		}

		setenv ("serial_dev", serial_mode);
	} 

	/*  ________________________________________________________________________________
	 * |_____________________________ AUDIO_CODEC SETTING ______________________________|
	 */

	if ( set_audio_codec ) {
			codec_mode_selection =  selection_codec_mode (codec_mode_list,
					ARRAY_SIZE(codec_mode_list));	

		if ( codec_mode_list [codec_mode_selection].codec_ac97_standard >= 0 ) {
			sprintf (codec_ac97_standard_audio, "audio_codec=ac97_standard");

	
		} else
			sprintf (codec_ac97_standard_audio, " ");

		if (codec_mode_list [codec_mode_selection].codec_sgtl5000 >= 0 ) {
			sprintf (codec_sgtl5000_audio, "audio_codec=sgtl5000");

		} else
			sprintf (codec_sgtl5000_audio, " ");
		
		sprintf (codec_mode, "%s %s", codec_ac97_standard_audio, codec_sgtl5000_audio);

		if (strcmp (codec_ac97_standard_audio, " ") ) {
			setenv ("codec_ac97_standard", codec_ac97_standard_audio);
		}
		
		if (strcmp (codec_sgtl5000_audio, " ") ) {
			setenv ("codec_sgtl5000", codec_sgtl5000_audio);
		}

		setenv ("audio_codec", codec_mode);
	} 

#endif

	/*  __________________________________________________________________________
	 * |_____________________________ VIDEO SETTING ______________________________|
	 */
	if ( set_video ) {
		video_mode_selection = selection_video_mode (video_mode_list,
				ARRAY_SIZE(video_mode_list));

		if ( video_mode_list[video_mode_selection].fb_hdmi >= 0 ) {
			sprintf (video_hdmi_video, "video=mxcfb%d:dev=hdmi,1920x1080M@60,if=RGB24",
					video_mode_list[video_mode_selection].fb_hdmi);
		} else
			sprintf (video_hdmi_video, " ");


		if ( video_mode_list[video_mode_selection].fb_lvds1 >= 0 ) {
			video_lvds1_selection = selection_lvds_spec ("LVDS1",
					lvds_video_spec_list, ARRAY_SIZE(lvds_video_spec_list));

			create_lvds_video_args (video_lvds1_video, video_lvds1_selection,
					video_mode_list[video_mode_selection].fb_lvds1,
					lvds_video_spec_list, ARRAY_SIZE(lvds_video_spec_list));
		} else
			sprintf (video_lvds1_video, " ");


		if ( video_mode_list[video_mode_selection].fb_lvds2 >= 0 ) {
			video_lvds2_selection = selection_lvds_spec ("LVDS2",
					lvds_video_spec_list, ARRAY_SIZE(lvds_video_spec_list));

			create_lvds_video_args (video_lvds2_video, video_lvds2_selection,
					video_mode_list[video_mode_selection].fb_lvds2,
					lvds_video_spec_list, ARRAY_SIZE(lvds_video_spec_list));
		} else
			sprintf (video_lvds2_video, " ");


		sprintf (video_mode, "%s %s %s", video_hdmi_video, video_lvds1_video,
				video_lvds2_video);

		if ( strcmp (video_hdmi_video, " ") ) {
			setenv ("video_hdmi", video_hdmi_video);
		}

		if ( strcmp (video_lvds1_video, " ") ) {
			setenv ("video_lvds1", video_lvds1_video);
		}

		if ( strcmp (video_lvds2_video, " ") ) {
			setenv ("video_lvds2", video_lvds2_video);
		}

		setenv ("videomode", video_mode);
	}

/*  __________________________________________________________________________
 * |__________________________________________________________________________|
*/

	create_environment (env_base, ARRAY_SIZE(env_base));

	printf ("\n\n");
	saveenv ();
	printf ("\n\n");

	return 0;
}


/***************************************************/

U_BOOT_CMD(
	seco_config, 3, 1, do_seco_config,
	"Interactive setup for seco configuration.",
	"          - set whole environment\n"
	"seco_config [default] - resetting to default environment\n"
	"seco_config [memory]  - set kernel RAM size.\n"
	"seco_config [ksrc]    - set Kernel source device.\n"
	"seco_config [fdtsrc]  - set FDT source device.\n"
	"seco_config [fssrc]   - set FileSystem source device.\n"
	"seco_config [video]   - set video usage mode\n"
#if defined CONFIG_TARGET_MX6SECO_Q7_928 || CONFIG_TARGET_MX6SECO_UQ7_962 || CONFIG_TARGET_MX6SECO_UQ7_J_A75
	"seco_config [serial_dev] - select the serial_dev function.\n"
	"seco_config [audio_codec] - select the audio_codec function.\n"
#endif
);
