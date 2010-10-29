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
char *driverToUse = NULL;
char *defaultModulePath = MODULE_DIR "/drivers";
#ifdef EXTRA_MODULE_DIR
char *extraModulePath = EXTRA_MODULE_DIR "/drivers";
#else
char *extraModulePath = NULL;
#endif
int verbose = 0;


int xf86MatchDevice(const char *driverName, GDevPtr **driverSectList)
{

    /* This means: there are 42 driver sections for you in xorg.conf, but your
     * list is NULL! */
    *driverSectList = NULL;
    return 42;
}

pointer xf86LoadDrvSubModule(DriverPtr drv, const char *name)
{
    /* XXX: actually load the module! */
    print_log("not loading submodule %s\n", name);
    return NULL;
}

void detected(uint32_t vendor_id, uint32_t device_id, char *driverName,
	      char *detectionMethod)
{
    printf("%s %.4x:%.4x %s\n", driverName, vendor_id, device_id,
	   detectionMethod);
}

void probeUsingSupportedDevices(DriverPtr driver)
{
    const struct pci_id_match *supported_devs = driver->supported_devices;
    int i;
    struct pci_device *dev;
    struct pci_device_iterator *iter;

    pci_system_init();

    print_log("\nMatching:\n");
    iter = pci_id_match_iterator_create(NULL);
    while ((dev = pci_device_next(iter)) != NULL) {
	for (i = 0; ; i++) {
	    if (supported_devs[i].vendor_id == 0 &&
		supported_devs[i].device_id == 0 &&
		supported_devs[i].subvendor_id == 0)
		break;

	    if (PCI_ID_COMPARE(supported_devs[i].vendor_id, dev->vendor_id) &&
		PCI_ID_COMPARE(supported_devs[i].device_id, dev->device_id) &&
		((supported_devs[i].device_class_mask & dev->device_class)
		 == supported_devs[i].device_class)) {
		detected(dev->vendor_id, dev->device_id, driver->driverName,
			 "supported_devices");
	    }
	}
    }

    pci_iterator_destroy(iter);
    pci_system_cleanup();
}


int xf86MatchPciInstances(const char *driverName, int vendorID,
			  SymTabPtr chipsets, PciChipsets *PCIchipsets,
			  GDevPtr *devList, int numDevs, DriverPtr driver,
			  int **foundEntities)
{
    int i;
    struct pci_device_iterator *iter;
    struct pci_device *dev;

    unsigned int vendorId, deviceId, matchClass;

    print_log("xf86MatchPciInstances:\n");
    print_log("driverName: %s\n", driverName);
    print_log("vendorId:   %d\n", vendorID);
    print_log("numDevs:    %d\n", numDevs);

    pci_system_init();

    iter = pci_id_match_iterator_create(NULL);
    while ((dev = pci_device_next(iter)) != NULL) {
	for(i = 0; PCIchipsets[i].PCIid != -1; i++) {
	    vendorId = (PCIchipsets[i].PCIid & 0xFFFF0000) >> 16;
	    deviceId = (PCIchipsets[i].PCIid & 0x0000FFFF);
	    /* Read comments inside the PciChipsets struct definition to try to
	     * understand this matchClass hack */
	    matchClass = 0x00030000 | PCIchipsets[i].PCIid;

	    /* XXX: convert dev->device_class from 0x00000101 to 0x00030000? */
	    /* XXX: can PCI_ID_COMPARE be used here? */
	    if ((vendorID == PCI_VENDOR_GENERIC) &&
		(matchClass == dev->device_class)) {
		/* It looks like the generic drivers have the supported_devices
		 * field, so they won't get here... */
		detected(dev->vendor_id, dev->device_id, driver->driverName,
			 "probe generic");
	    }
	    if ((vendorId == dev->vendor_id) &&
		(deviceId == dev->device_id)) {
		/* XXX: compare dev->device_class? */
		detected(dev->vendor_id, dev->device_id, driver->driverName,
			 "probe");
	    }
	}
    }

    pci_system_cleanup();
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
int findCardsForDriver(char *driverCanonicalName, char *driverDir)
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

    rc = snprintf(driverPath, bufferSize, "%s/%s_drv.so",
		  driverDir, driverCanonicalName);
    assert(rc != bufferSize && rc >= 0);
    rc = snprintf(driverData, bufferSize, "%sModuleData", driverCanonicalName);
    assert(rc != bufferSize && rc >= 0);

    handle = dlopen(driverPath, RTLD_LAZY);
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

    /* XXX: do some API/ABI checking! */

    printModuleData(moduleData);

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
	strcmp(name, "sisusb") == 0 || strcmp(name, "ati") == 0) {
	print_log("Ignoring driver %s\n", name);
	return TRUE;
    }
    return FALSE;
}

/* This function looks in defaultModulePath and extraModulePath for files whose
 * name end with "_drv.so" and don't return true for the "ignoredDriver"
 * function. Then it calls "findCardsForDriver" to each match. */
int findCardsForAllDrivers()
{
    DIR *driversDir;
    struct dirent *entry;
    regex_t driversRegex;
    regmatch_t matches[2];
    int rc;
    char driverCanonicalName[NAME_MAX];
    char *modulePaths[3] = { defaultModulePath, extraModulePath, NULL };
    int i;

    for(i = 0; modulePaths[i] != NULL; i++) {
	driversDir = opendir(modulePaths[i]);
	if (driversDir == NULL) {
	    fprintf(stderr, "Error opening %s directory\n", modulePaths[i]);
	    return 1;
	}

	rc = regcomp(&driversRegex, "^(.*)_drv\\.so$", REG_EXTENDED);
	assert(rc == 0);

	while ( (entry = readdir(driversDir)) ) {
	    rc = regexec(&driversRegex, entry->d_name, 2, matches, 0);
	    if (rc == 0) {
		print_log("\n--- driver: ");
		substrcpy(driverCanonicalName, entry->d_name,
			  matches[1].rm_so, matches[1].rm_eo);
		print_log("%s\n", driverCanonicalName);

		if (!ignoredDriver(driverCanonicalName)) {
		    if (findCardsForDriver(driverCanonicalName,
					   modulePaths[i]) != 0) {
			return 1;
		    }
		}

	    } else if (rc != REG_NOMATCH) {
		assert(0);
	    }
	}

	closedir(driversDir);
    }
    return 0;
}

void printUsage(char *progName)
{
    fprintf(stderr, "Usage: %s [options]\n"
	    "Options:\n"
	    " -d driver  check only driver, not all of them\n"
	    " -m path    change driver module path to \"path\"\n"
	    " -e path    add an extra driver module path to be searched\n"
	    " -v         print more information\n"
	    " -h         prints this help\n",
	    progName);
}

int main(int argc, char *argv[])
{
    int opt;
    while ((opt = getopt(argc, argv, "d:m:e:vh")) != -1) {
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

    return findCardsForAllDrivers();
}
