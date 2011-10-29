/***************************************************************************
**
** COPYRIGHT(C)	: Samsung Electronics Co.Ltd, 2007-2011 ALL RIGHTS RESERVED
**
** AUTHOR		: KyoungHOON Kim (khoonk)
**
*****************************************************************************
**
** 2007/03/12. khoonk		1. system infomation generate
**
*****************************************************************************/
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include "iniparser.h"
#include "devmgr_ext.h"

/****************************************************************************
** definition
*****************************************************************************/
#define PROC_KERNEL 		"/proc/version"
#define PROC_LOADER 		"/proc/cmdline"
#define SYSINFO_DB 			"/.info/system.info"
#define ROOTFS_DB 			"/.info/rootfs.info"
#define FACTORYFS_DB 		"/.info/factoryfs.info"
#define DATAFS_DB 			"/.info/datafs.info"

#define PLATFORM_SECTION	"PLATFORM_INFO"
#define ROOTFS_SECTION		"COMPILE_INFO"
#define VERSION_SECTION		"VERSION_INFO"
#define RELEASE_SECTION		"RELEASE_INFO"

#define SIZE_CMDLINE		256
#define TOKEN_COUNT			4			

/****************************************************************************
** variables
*****************************************************************************/
const char *platform_value[2] = 
{
	"platform_name",
	"platform_hw",
};

const char *rootfs_value[5] = 
{
	"compile_time",
	"compiled_by",
	"compile_host",
	"compiler_version",
	"busybox_version",
};

const char *version_value[5] = 
{
	"loader",
	"kernel",
	"rootfs",
	"factoryfs",
	"datafs",
};

/****************************************************************************
** functions
*****************************************************************************/
int sys_main(void)
{
	FILE *cfg_file = NULL;
#ifdef USE_CFL
	cflh_t cfg;
	cfl_value_type_t type;
	char *val;
#else
	dictionary *d;
	char val[80];
#endif

	FILE *loader = NULL;
	FILE *rfs = NULL;
	char buffer[SIZE_CMDLINE];
	char *p_version,*token,*dest;
	char delimit[]=" =\r\n";
	int num,i;
	struct utsname loader_info;
	struct utsname kernel_info;
	struct utsname rootfs_info;
	//read /proc/cmdline file and store data
	loader=fopen(PROC_LOADER,"r");	
	if (loader == NULL)
	{
		fprintf (stderr, "fopen (%s): ", PROC_LOADER);
		perror ("");
		exit(EXIT_FAILURE);
	}

	memset(&loader_info,0,sizeof(struct utsname));
	memset(buffer,0,SIZE_CMDLINE);

	num = fread(buffer,1,256,loader);
	buffer[num]=0;

	p_version = strstr(buffer,"version");
	if(p_version != NULL ) {
		token = strtok_r(p_version,delimit,&dest);
		token = strtok_r(NULL,delimit,&dest);
		strcpy(loader_info.release,token);

		for(i=0;i<TOKEN_COUNT;i++) {
			token = strtok_r(NULL,delimit,&dest);
			if(token==NULL)
				break;
			//printf("%s \n",token);
			strcat(loader_info.version,token);
			if(i<(TOKEN_COUNT-1))
				strcat(loader_info.version," ");
		}
		//printf("loader_release = %s\n",loader_info.release);
		//printf("loader_version = %s\n",loader_info.version);
	}
	rewind(loader);
	fclose(loader);

	//get kernel information by uname system call
	if (uname(&kernel_info) == -1) {
		printf("cannot get system kernel_info");
	}
	//printf("kernel_release = %s\n",kernel_info.release);
	//printf("kernel_version = %s\n",kernel_info.version);

	cfg_file = fopen(SYSINFO_DB,"w");
	if (cfg_file == NULL)
	{
		fprintf (stderr, "fopen (%s): ", SYSINFO_DB);
		perror ("");
		exit(EXIT_FAILURE);
	}
	fprintf(cfg_file,"[%s]\n",PLATFORM_SECTION);

	//read rootfs.info and store data
#ifdef USE_CFL
	if ((cfg = cfl_read(ROOTFS_DB))==NULL)
	{
		printf("can't open "ROOTFS_DB"\n");
		exit(EXIT_FAILURE);
	}
	val = cfl_value_get_by_name(cfg, PLATFORM_SECTION, platform_value[0], &type);
	fprintf(cfg_file,"%s = \"%s\"\n",platform_value[0],val);
	val = cfl_value_get_by_name(cfg, PLATFORM_SECTION, platform_value[1], &type);
	fprintf(cfg_file,"%s = \"%s\"\n",platform_value[1],val);
	val = cfl_value_get_by_name(cfg, ROOTFS_SECTION, rootfs_value[0], &type);
	strcpy(rootfs_info.version,val);
	val = cfl_value_get_by_name(cfg, ROOTFS_SECTION, rootfs_value[4], &type);
	strcpy(rootfs_info.release,val);

	//cfl_write_xml (cfg, stdout);
	cfl_free(cfg);
#else
//#if 0

	rfs=fopen(ROOTFS_DB,"r");	
	if (rfs == NULL)
	{
		fprintf (stderr, "fopen (%s): ", ROOTFS_DB);
		perror ("");
		return -1;
	}

	fclose(rfs);
	
	d = iniparser_load(ROOTFS_DB);
	sprintf(val,"%s:%s",PLATFORM_SECTION,platform_value[0]);
	fprintf(cfg_file,"%s = %s;\n",platform_value[0],iniparser_getstr(d,val));
	sprintf(val,"%s:%s",PLATFORM_SECTION,platform_value[1]);
	fprintf(cfg_file,"%s = %s;\n",platform_value[1],iniparser_getstr(d,val));
	sprintf(val,"%s:%s",ROOTFS_SECTION,rootfs_value[0]);
	strcpy(rootfs_info.version,iniparser_getstr(d,val));
	//sprintf(val,"%s:%s",ROOTFS_SECTION,rootfs_value[4]);
	//strcpy(rootfs_info.release,iniparser_getstr(d,val));

	iniparser_freedict(d);
//#endif
#endif

	fprintf(cfg_file,"[%s]\n",RELEASE_SECTION);
	fprintf(cfg_file,"%s = %s;\n",version_value[0],loader_info.release);
	fprintf(cfg_file,"%s = %s;\n",version_value[1],kernel_info.release);
	//fprintf(cfg_file,"%s = %s;\n",version_value[2],rootfs_info.release);

	fprintf(cfg_file,"[%s]\n",VERSION_SECTION);
	fprintf(cfg_file,"%s = %s;\n",version_value[0],loader_info.version);
	fprintf(cfg_file,"%s = %s;\n",version_value[1],kernel_info.version);
	fprintf(cfg_file,"%s = %s;\n",version_value[2],rootfs_info.version);

	fclose(cfg_file);
	return 0;
}
