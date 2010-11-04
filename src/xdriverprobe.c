/*
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Author: Paulo Zanoni <pzanoni@mandriva.com>
 */

#include <sys/types.h>

#include <assert.h>
#include <dirent.h>
#include <dlfcn.h>
#include <regex.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <pciaccess.h>
#include <xorg/xf86Module.h>
#include <xorg/xf86PciInfo.h>
#include <xorg/xf86str.h>

#include "config.h"
#include "utils.h"

/* These are the symbols that actually matter */
_X_EXPORT void xf86AddDriver(DriverPtr driver, pointer module, int flags);
_X_EXPORT int xf86MatchPciInstances(const char *driverName, int vendorID,
				    SymTabPtr chipsets,
				    PciChipsets *PCIchipsets,
				    GDevPtr *devList, int numDevs,
				    DriverPtr driver, int **foundEntities);
_X_EXPORT pointer xf86LoadDrvSubModule(DriverPtr drv, const char *name);
_X_EXPORT int xf86MatchDevice(const char *driverName, GDevPtr **driverSectList);


/* Global variables changed by program arguments: */
const char *driverToUse = NULL;
char *defaultModulePath = MODULE_DIR "/drivers";
#ifdef EXTRA_MODULE_DIR
char *extraModulePath = EXTRA_MODULE_DIR "/drivers";
#else
char *extraModulePath = NULL;
#endif
int verbose = 0;
enum { PRINT_DEVS_PRESENT, PRINT_DEVS_SUPPORTED } mode = PRINT_DEVS_PRESENT;


int findModulesAndProcess(Bool lookingForSubModules);

int xf86MatchDevice(const char *driverName, GDevPtr **driverSectList)
{

    /* This means: there are 42 driver sections for you in xorg.conf, but your
     * list is NULL! */
    *driverSectList = NULL;
    return 42;
}

pointer xf86LoadDrvSubModule(DriverPtr drv, const char *name)
{
    const char *backup = driverToUse;
    print_log("Loading submodule: %s\n", name);
    driverToUse = name;
    findModulesAndProcess(TRUE);
    driverToUse = backup;
    return NULL;
}

void detected(uint32_t vendor_id, uint32_t device_id, char *driverName)
{
    printf("%s %.4x:%.4x\n", driverName, vendor_id, device_id);
}

/* Checks if a device is present in the machine running the program
 * The "driverName" argument is only used when printing.
 * XXX: cache the libpciaccess information so we don't need to call it many
 *      times? */
Bool findDeviceOnMachine(const struct pci_id_match *match, char *driverName)
{
    struct pci_device *dev;
    struct pci_device_iterator *iter;
    Bool ret = FALSE;

    iter = pci_id_match_iterator_create(match);
    while ((dev = pci_device_next(iter)) != NULL) {
	detected(dev->vendor_id, dev->device_id, driverName);
	ret = TRUE;
	break;
    }
    pci_iterator_destroy(iter);
    return ret;
}

/* XXX: find a better name for this function */
void diagnose(const struct pci_id_match *match, char *driverName)
{
    if (mode == PRINT_DEVS_PRESENT) {
	findDeviceOnMachine(match, driverName);
    } else { /* PRINT_DEVS_SUPPORTED */
	printf("%s ", driverName);
	if (match->vendor_id == (uint32_t) PCI_MATCH_ANY)
	    printf("any:");
	else
	    printf("%.4x:", match->vendor_id);
	if (match->device_id == (uint32_t) PCI_MATCH_ANY)
	    printf("any\n");
	else
	    printf("%.4x\n", match->device_id);
    }
}

void probeUsingSupportedDevices(DriverPtr driver)
{
    const struct pci_id_match *supported_devs = driver->supported_devices;
    int i;

    for (i = 0; ; i++) {
	if (supported_devs[i].vendor_id == 0 &&
	    supported_devs[i].device_id == 0 &&
	    supported_devs[i].subvendor_id == 0)
	    break;
	diagnose(&supported_devs[i], driver->driverName);
    }
}


int xf86MatchPciInstances(const char *driverName, int vendorID,
			  SymTabPtr chipsets, PciChipsets *PCIchipsets,
			  GDevPtr *devList, int numDevs, DriverPtr driver,
			  int **foundEntities)
{
    /* According to comments inside Xorg's xf86MatchPciInstances, some drivers
     * might work even if the device_ids don't match (but vendor_id has to
     * match) */
    int i;

    struct pci_id_match match;

    for(i = 0; PCIchipsets[i].PCIid != -1; i++) {
	if (vendorID == PCI_VENDOR_GENERIC) {
	    match.vendor_id = PCI_MATCH_ANY;
	    match.device_id = PCI_MATCH_ANY;
	    /* XXX: what about devices with class 0x00000101? */
	    match.device_class = 0x00030000 | PCIchipsets[i].PCIid;
	    match.device_class_mask = 0xFFFFFFFF;
	} else {
	    if (vendorID != 0) {
		match.vendor_id = vendorID;
	    } else {
		match.vendor_id = (PCIchipsets[i].PCIid & 0xFFFF0000) >> 16;
	    }
	    match.device_id = (PCIchipsets[i].PCIid & 0x0000FFFF);
	    match.device_class = 0;
	    match.device_class_mask = 0;
	}
	match.subvendor_id = PCI_MATCH_ANY;
	match.subdevice_id = PCI_MATCH_ANY;

	diagnose(&match, driver->driverName);
    }

    *foundEntities = NULL;
    return 0;
}

void xf86AddDriver(DriverPtr driver, pointer module, int flags)
{
    if (!driver)
	return;

    if (driver->supported_devices) {
	probeUsingSupportedDevices(driver);
    } else {
	print_log("Driver %s doesn't have supported_devices in DriverRec. "
		  "Using \"Probe()\"\n", driver->driverName);
	(*driver->Probe)(driver, PROBE_DETECT);
    }
}

void printModuleData(XF86ModuleData *moduleData)
{
    print_log("modname:      %s\n", moduleData->vers->modname);
    print_log("vendor:       %s\n", moduleData->vers->vendor);
    print_log("xf86version:  %u\n", moduleData->vers->xf86version);
    print_log("majorversion: %hhd\n", moduleData->vers->majorversion);
    print_log("minorversion: %hhd\n", moduleData->vers->minorversion);
    print_log("patchlevel:   %hd\n", moduleData->vers->patchlevel);
    print_log("abiclass:     %s\n", moduleData->vers->abiclass);
    print_log("abiversion:   %hu.%hu\n",
	      GET_ABI_MAJOR(moduleData->vers->abiversion),
	      GET_ABI_MINOR(moduleData->vers->abiversion));
    print_log("moduleclass:  %s\n", moduleData->vers->moduleclass);
}

/* This function opens the driver and then calls its "setup" function */
int findCardsForDriver(char *driverCanonicalName, char *driverDir,
		       Bool isSubModule)
{
    int rc;
    const int bufferSize = 128;
    char driverPath[bufferSize];
    char driverData[bufferSize];
    const char *error;
    void *handle;
    XF86ModuleData *moduleData;
    int errmaj, errmin;
    void *setupRc;

    if (isSubModule)
	rc = snprintf(driverPath, bufferSize, "%s/%s.so",
		      driverDir, driverCanonicalName);
    else
	rc = snprintf(driverPath, bufferSize, "%s/%s_drv.so",
		      driverDir, driverCanonicalName);
    assert(rc != bufferSize && rc >= 0);
    rc = snprintf(driverData, bufferSize, "%sModuleData", driverCanonicalName);
    assert(rc != bufferSize && rc >= 0);

    /* RTLD_GLOBAL is required for loading subModules */
    handle = dlopen(driverPath, RTLD_LAZY | RTLD_GLOBAL);
    if (!handle) {
	fprintf(stderr, "Error opening driver: %s\n", dlerror());
	return 1;
    }

    moduleData = dlsym(handle, driverData);
    error = dlerror();
    if (error) {
	fprintf(stderr, "Error retrieving driver data: %s\n", error);
	return 1;
    }

    printModuleData(moduleData);

    if (strcmp(moduleData->vers->abiclass, ABI_CLASS_VIDEODRV) != 0) {
	fprintf(stderr, "Error: abiclass is not %s\n", ABI_CLASS_VIDEODRV);
	return 1;
    }

    if (GET_ABI_MAJOR(moduleData->vers->abiversion) < 6) {
	fprintf(stderr, "Error: video driver ABI is too old: %u\n",
		GET_ABI_MAJOR(moduleData->vers->abiversion));
	return 1;
    }

    /* fbdev has NULL moduleclass
    if (strcmp(moduleData->vers->moduleclass, MOD_CLASS_VIDEODRV) != 0) {
	fprintf(stderr, "Error: moduleclass is not %s\n", MOD_CLASS_VIDEODRV);
	return 1;
    } */

    setupRc = moduleData->setup(NULL, NULL, &errmaj, &errmin);
    if (!setupRc) {
	fprintf(stderr, "Error: !setupRc (errmaj=%d errmin=%d)\n",
		errmaj, errmin);
	return 1;
    }
    if (!moduleData->setup) {
	fprintf(stderr, "Error: module doesn't have a setup function\n");
	return 1;
    }

    return 0;
}

Bool ignoredDriver(char *name)
{
    if (driverToUse) {
	if (strcmp(name, driverToUse) == 0)
	    return FALSE;
	else
	    return TRUE;
    }

    if (strcmp(name, "vmware") == 0 || strcmp(name, "v4l") == 0 ||
	strcmp(name, "sisusb") == 0 || strcmp(name, "ati") == 0 ||
	strcmp(name, "nvidia") == 0) {
	print_log("Ignoring driver %s\n", name);
	return TRUE;
    }
    return FALSE;
}

/* This function looks in defaultModulePath and extraModulePath for files whose
 * name end with "_drv.so" (or just ".so" in case of submodules) and don't
 * return true for the "ignoredDriver" function. Then it calls
 * "findCardsForDriver" to each match. */
int findModulesAndProcess(Bool lookingForSubModules)
{
    DIR *driversDir;
    struct dirent *entry;
    regex_t driversRegex;
    regmatch_t matches[2];
    int rc;
    char driverCanonicalName[NAME_MAX];
    char *modulePaths[3] = { defaultModulePath, extraModulePath, NULL };
    int i;
    int ret = 0;

    /* For some reason the cirrus submodules don't have "_drv" in their names */
    if (lookingForSubModules)
	rc = regcomp(&driversRegex, "^(.*)\\.so$", REG_EXTENDED);
    else
	rc = regcomp(&driversRegex, "^(.*)_drv\\.so$", REG_EXTENDED);
    assert(rc == 0);

    for(i = 0; (modulePaths[i] != NULL) && (!ret); i++) {
	driversDir = opendir(modulePaths[i]);
	if (driversDir == NULL) {
	    fprintf(stderr, "Error opening %s directory\n", modulePaths[i]);
	    ret = 1;
	    break;
	}

	while ( (entry = readdir(driversDir)) ) {
	    rc = regexec(&driversRegex, entry->d_name, 2, matches, 0);
	    if (rc == 0) {
		print_log("\n--- driver: ");
		substrcpy(driverCanonicalName, entry->d_name,
			  matches[1].rm_so, matches[1].rm_eo);
		print_log("%s\n", driverCanonicalName);

		if (!ignoredDriver(driverCanonicalName)) {
		    if (findCardsForDriver(driverCanonicalName, modulePaths[i],
			(lookingForSubModules) ? TRUE : FALSE) != 0) {
			ret = 1;
			break;
		    }
		}

	    } else if (rc != REG_NOMATCH) {
		assert(0);
	    }
	}

	closedir(driversDir);
    }

    regfree(&driversRegex);
    return ret;
}

void printUsage(char *progName)
{
    fprintf(stderr, "Usage: %s [options]\n"
	    "Options:\n"
	    " -d driver  check only \"driver\", not all of them\n"
	    " -m path    change driver module path to \"path\"\n"
	    " -e path    add an extra driver module path to be searched\n"
	    " -a         print all devices supported by each video driver "
	    "instead of only the ones found in your machine\n"
	    " -v         print more information\n"
	    " -h         print this help\n",
	    progName);
}

int main(int argc, char *argv[])
{
    int opt;
    int ret = 0;
    while ((opt = getopt(argc, argv, "d:m:e:avh")) != -1) {
	switch(opt) {
	case 'd':
	    driverToUse = optarg;
	    break;
	case 'm':
	    defaultModulePath = optarg;
	    break;
	case 'e':
	    extraModulePath = optarg;
	    break;
	case 'a':
	    mode = PRINT_DEVS_SUPPORTED;
	    break;
	case 'v':
	    verbose = 1;
	    break;
	case 'h':
	    printUsage(argv[0]);
	    return 0;
	default:
	    printUsage(argv[0]);
	    return 1;
	}
    }

    print_log("driverToUse:       %s\n"
	      "defaultModulePath: %s\n"
	      "extraModulePath:   %s\n"
	      "verbose:           %d\n",
	      driverToUse, defaultModulePath, extraModulePath, verbose);

    assert(pci_system_init() == 0);
    ret = findModulesAndProcess(FALSE);
    pci_system_cleanup();
    return ret;
}
