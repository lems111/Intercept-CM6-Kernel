

/****************************************************************************
**
** COPYRIGHT(C)	: Samsung Electronics Co.Ltd, 2005-2009 ALL RIGHTS RESERVED
**
** AUTHOR		: KyoungHOON Kim (khoonk)
**
*****************************************************************************
** 2006/07/19. khoonk	1. add touchscreen calibration convert function
** 2006/07/31. khoonk	1. remove UDP socket functions (to devmgr_api.c)
**                      2. add IOC_DCM_START function
*****************************************************************************/

/*****************************************************************************
** header files
*****************************************************************************/
#include "devmgr_main.h"
#include <linux/apm_bios.h> 

/****************************************************************************
** functions
*****************************************************************************/

char* get_event_number(char* devicename)
{
	FILE *fptr;
	char *ptr, *strptr, *cellstr;
	char buf[2048];
	int pos;
	system("cat /proc/bus/input/devices > /tmp/evnum");
	fptr = fopen("/tmp/evnum", "rt+");
	if(fptr == NULL)
	{
		printf("evnum file open failed\n");
	}

	pos = fread(buf,sizeof(char),2048,fptr);
	buf[pos] = 0;
	strptr = buf;
	ptr = strstr(strptr, devicename);
	if(ptr == NULL)
	{
		printf("failed to find %s\n", devicename);
		return ptr;
	}
	strptr = ptr + 1;
	cellstr = strstr(strptr, "event");
	strptr = cellstr;
	*(strptr+6) = 0;
	printf("%s = %s\n",devicename ,strptr);
	
	return strptr;
}


int
my_system(const char *command)
{
	int pid, status;

	if (command == 0)
		return 1;
	pid = fork();
	if (pid == -1)
		return -1;
	if (pid == 0) {
		char *argv[4];
		argv[0] = "sh";
		argv[1] = "-c";
		argv[2] = (char *)command;
		argv[3] = 0;
		execv("/bin/sh", argv);
		exit(127);
	}
	do {
		if (waitpid(pid, &status, 0) == -1) {
			if (errno != EINTR)
				return -1;
		} else
			return status;
	} while(1);
}
