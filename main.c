#include <fcntl.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>

typedef struct module_s {
    char *cp_name;
    int found;

} module_t;



module_t loadedModules[32] = {
    { .cp_name = "vfio-pci", },
    { .cp_name = "uio_pci_generic", },
   };
int loadedModuleCnt = 2;

int sys_show_modules_state();
int sys_check_module(char *module_name);
int sys_show_device_status(char *device_name);
int unbind_device(char *device_name);
int bind_device(char *device_name, char *module_name);



int main(int argc, char *argv[]){


    if (argc < 2) {
        sys_show_modules_state();
        return 0;
    }
    if (argv[1][0] == 's') {
        //show device status
        sys_show_device_status(argv[2]);
    }
    else if (argv[1][0] == 'u') {
        //unbind the device
        unbind_device(argv[2]);
    }
    else if (argv[1][0] == 'b' && argc == 4) {
        //bind the device
        bind_device(argv[2], argv[3]);
    }
    else{
        //usage
        printf("Usage:  bind                          ,list modules and state\n");
        printf("        bind s  B:D.f                 ,device status\n");
        printf("        bind u  B:D.f                 ,unbind device from current module\n");
        printf("        bind u  B:D.f module_name     ,bind device to module\n");
    }

    return 0;
}


int sys_show_modules_state(){
    int i;

    printf("Kernel bypass modules currently supported:\n");
    for (i = 0; i < loadedModuleCnt; i++) {
        printf("\t%24s ::", loadedModules[i].cp_name);
        if (sys_check_module(loadedModules[i].cp_name)) {
            printf("Not loaded - sudo modprobe %s\n", loadedModules[i].cp_name);
        }
        else {
            printf("Found\n");
        }
    }


    return 0;
}

int sys_check_module(char *module_name)
{
	char sysfs_mod_name[PATH_MAX];
	struct stat st;
	int n;

	if (NULL == module_name)
		return -1;

	/* Check if there is sysfs mounted */
	if (stat("/sys/module", &st) != 0) {
		printf( "sysfs is not mounted! error %i (%s)\n",
			errno, strerror(errno));
		return -1;
	}

	/* A module might be built-in, therefore try sysfs */
	n = snprintf(sysfs_mod_name, PATH_MAX, "/sys/module/%s", module_name);
	if (n < 0 || n > PATH_MAX) {
		printf( "Could not format module path\n");
		return -1;
	}

	if (stat(sysfs_mod_name, &st) != 0) {
		//printf("Module %s not found! error %i (%s)\n",
		//        sysfs_mod_name, errno, strerror(errno));
		return -1;
	}

	/* Module has been found */
	return 0;
}




int sys_show_device_status(char *device_name){
  FILE *fp;
  char path[1035];
  char cmd[256];

  sprintf(cmd,"lspci -Dvmmnnk -s %s", device_name );

  /* Open the command for reading. */
  fp = popen(cmd, "r");
  if (fp == NULL) {
    printf("Failed to run command %s\n" , cmd);
    exit(1);
  }

  /* Read the output a line at a time - output it. */
  while (fgets(path, sizeof(path), fp) != NULL) {
    printf("%s", path);
  }

  /* close */
  pclose(fp);

	return 0;
}

int get_driver(char *device_name, char *driver){
    FILE *fp;
    char path[1035];
    char cmd[256];

    sprintf(cmd,"lspci -Dvmmnnk -s %s", device_name );
    driver[0] = 0;

    /* Open the command for reading. */
    fp = popen(cmd, "r");
    if (fp == NULL) {
      printf("Failed to run command %s\n" , cmd);
      exit(1);
    }

    /* Read the output a line at a time - output it. */
    while (fgets(path, sizeof(path), fp) != NULL) {
      //printf("%s", path);
        if (strncmp(path, "Driver:", 7) == 0) {
            strcpy(driver, &path[8]);
            printf("driver %s\n", driver);
        }
    }

    /* close */
    pclose(fp);
    return 0;
}



int unbind_device(char *device_name){

    FILE *fp;
    char file[1035];
    char driver[32];
    int i;

    get_driver(device_name, &driver[0]);
    i = strlen(&driver[0]);
    driver[i - 1] = 0;
    sprintf(file, "/sys/bus/pci/drivers/%s/unbind", &driver[0]);
    /* Open the command for reading. */
    fp = fopen(file, "a");
    if (fp == NULL) {
      printf("Failed to open %s\n" , file);
      exit(1);
    }
    fprintf(fp, "%s", device_name);
    //fwrite(fp, device_name, strlen(device_name));
    /* close */
    fclose(fp);

	return 0;
}

int bind_device(char *device_name, char *module_name){
    FILE *fp;
    char file[1035];

    sprintf(file, "/sys/bus/pci/devices/%s/driver_override", device_name);
    /* Open the command for reading. */
    fp = fopen(file, "w");
    if (fp == NULL) {
      printf("Failed to open %s\n" , file);
      exit(1);
    }
    fprintf(fp, "%s", module_name);
    //fwrite(fp, device_name, strlen(device_name));
    /* close */
    fclose(fp);

    sprintf(file, "/sys/bus/pci/drivers/%s/bind", module_name);
    /* Open the command for reading. */
    fp = fopen(file, "a");
    if (fp == NULL) {
      printf("Failed to open %s\n" , file);
      exit(1);
    }
    fprintf(fp, "%s", device_name);
    //fwrite(fp, device_name, strlen(device_name));
    /* close */
    fclose(fp);

	return 0;
}
