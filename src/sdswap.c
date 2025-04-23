#define _XOPEN_SOURCE 600

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <getopt.h>

#include <libusbgetdev.h>

#include <libusb.h>

/*
 * Port feature numbers
 * See USB 2.0 spec Table 11-17
 */
#define USB_PORT_FEAT_CONNECTION	0
#define USB_PORT_FEAT_ENABLE		1
#define USB_PORT_FEAT_SUSPEND		2	/* L2 suspend */
#define USB_PORT_FEAT_OVER_CURRENT	3
#define USB_PORT_FEAT_RESET		4
#define USB_PORT_FEAT_L1		5	/* L1 suspend */
#define USB_PORT_FEAT_POWER		8
#define USB_PORT_FEAT_LOWSPEED		9	/* Should never be used */
#define USB_PORT_FEAT_C_CONNECTION	16
#define USB_PORT_FEAT_C_ENABLE		17
#define USB_PORT_FEAT_C_SUSPEND		18
#define USB_PORT_FEAT_C_OVER_CURRENT	19
#define USB_PORT_FEAT_C_RESET		20
#define USB_PORT_FEAT_TEST		21
#define USB_PORT_FEAT_INDICATOR		22
#define USB_PORT_FEAT_C_PORT_L1		23

/*
 * Port feature selectors added by USB 3.0 spec.
 * See USB 3.0 spec Table 10-7
 */
#define USB_PORT_FEAT_LINK_STATE		5
#define USB_PORT_FEAT_U1_TIMEOUT		23
#define USB_PORT_FEAT_U2_TIMEOUT		24
#define USB_PORT_FEAT_C_PORT_LINK_STATE		25
#define USB_PORT_FEAT_C_PORT_CONFIG_ERROR	26
#define USB_PORT_FEAT_REMOTE_WAKE_MASK		27
#define USB_PORT_FEAT_BH_PORT_RESET		28
#define USB_PORT_FEAT_C_BH_PORT_RESET		29
#define USB_PORT_FEAT_FORCE_LINKPM_ACCEPT	30

/*
 * Definitions for PORT_LINK_STATE values
 * (bits 5-8) in wPortStatus
 */
#define USB_SS_PORT_LS_U0		0x0000
#define USB_SS_PORT_LS_U1		0x0020
#define USB_SS_PORT_LS_U2		0x0040
#define USB_SS_PORT_LS_U3		0x0060
#define USB_SS_PORT_LS_SS_DISABLED	0x0080
#define USB_SS_PORT_LS_RX_DETECT	0x00a0
#define USB_SS_PORT_LS_SS_INACTIVE	0x00c0
#define USB_SS_PORT_LS_POLLING		0x00e0
#define USB_SS_PORT_LS_RECOVERY		0x0100
#define USB_SS_PORT_LS_HOT_RESET	0x0120
#define USB_SS_PORT_LS_COMP_MOD		0x0140
#define USB_SS_PORT_LS_LOOPBACK		0x0160

#define USB_CTRL_GET_TIMEOUT	5000
#define USB_SS_BCD		0x0300

#define USB_MAX_DEPTH 7
#define ATTACH_RETRY_CNT 5

static bool opt_print = false;
static bool opt_exact = false;
static int opt_swap = -1;
static uint16_t opt_vendor_id = 0x0bda;
static uint16_t opt_product_id = 0x0316;
static char *opt_serial = NULL;
static uint8_t opt_bus = 0;
static uint8_t opt_addr = 0;
static uint8_t opt_ports[7];
static size_t opt_ports_len = 0;

static int attach_kernel_driver(struct libusb_device_handle *devh);
static int detach_kernel_driver(struct libusb_device_handle *devh);
static int set_port_feature(struct libusb_device_handle *devh, int port, uint16_t feature);
static int hub_set_port_link_state(struct libusb_device_handle *devh, int port,
				   unsigned int link_status);
static int libusb_open_parent(libusb_context *ctx, libusb_device *child,
			      libusb_device_handle **devh);
static int parse_path(const char *path, uint8_t *bus, uint8_t *ports, size_t *ports_len);
static int parse_busaddr(const char *path, uint8_t *bus, uint8_t *addr);
static int parse_id(const char *id, uint16_t *vendor_id, uint16_t *product_id);
static int do_swap(struct libusb_device *dev);
static int do_print(struct libusb_device *dev);
static int print_status(struct libusb_device_handle *devh);
static int is_superspeed(struct libusb_device *dev);
static bool check_busaddr(libusb_device *dev, uint8_t bus, uint8_t addr);
static int check_serial(struct libusb_device * dev, uint8_t desc_index);
static int check_path(libusb_device *dev, uint8_t bus, uint8_t *ports, size_t ports_len);
static int get_device(libusb_context *ctx, struct libusb_device **dev);
static void do_help(int code);

int main(int argc, char *argv[]) {
	int rc, opt;
	struct libusb_device *dev = NULL;
	int option_index = 0;
	static const struct option long_options[] = {
		{"help",	no_argument,		NULL, 'h'},
		{"print",	no_argument,		NULL, 'p'},
		{"controller",	no_argument,		NULL, 'c'},
		{"target",	no_argument,		NULL, 't'},
		{"exact",	no_argument,		NULL, 'e'},
		{"location",	required_argument,	NULL, 'l'},
		{"address",	required_argument,	NULL, 'a'},
		{"serial",	required_argument,	NULL, 's'},
		{"id",		required_argument,	NULL, 'i'},
		{NULL,		0,			NULL,  0}
	};

	while (1) {
		opt = getopt_long(argc, argv, "hpctel:a:s:i:", long_options, &option_index);
		if (opt == -1)
			break;

		switch (opt) {
		case 'h':
			do_help(EXIT_SUCCESS);
			break;
		case 'p':
			// Print block device
			opt_print = true;
			break;
		case 'c':
			// Switch to PC
			opt_swap = 1;
			break;
		case 't':
			// Switch to target
			opt_swap = 0;
			break;
		case 'e':
			opt_exact = true;
			break;
		case 'l':
			// Set location
			rc = parse_path(optarg, &opt_bus, opt_ports, &opt_ports_len);
			if (rc < 0)
				exit(EXIT_FAILURE);
			break;
		case 'a':
			// Set address
			rc = parse_busaddr(optarg, &opt_bus, &opt_addr);
			if (rc < 0)
				exit(EXIT_FAILURE);
			break;
		case 's':
			// Set serial number
			opt_serial = optarg;
			break;
		case 'i':
			// Set PID and VID
			rc = parse_id(optarg, &opt_vendor_id, &opt_product_id);
			if (rc < 0)
				exit(EXIT_FAILURE);
			break;
		default:
			do_help(EXIT_FAILURE);
		}
	}

	rc = libusb_init_context(/*ctx=*/NULL, /*options=*/NULL, /*num_options=*/0);
	if (rc < 0) {
		fprintf(stderr, "Error initializing libusb: %s\n", libusb_error_name(rc));
		exit(EXIT_FAILURE);
	}

	rc = get_device(NULL, &dev);
	if (rc < 0)
		goto out;

	rc = do_swap(dev);
	if (rc < 0)
		goto out;

	rc = do_print(dev);
	if (rc < 0)
		goto out;

out:
	if (dev)
		libusb_unref_device(dev);

	libusb_exit(NULL);

	return rc;
}

static void do_help(int code) {
	fprintf(stderr, "Usage: sdswap [-h] [-p] [-c | -t] [-l location] [-a address]");
	fprintf(stderr, "[-s serial] [-i VID:PID]\n");
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "  -h, --help          Show this help message.\n");
	fprintf(stderr, "  -p, --print         Prints the block device.\n");
	fprintf(stderr, "  -c, --controller    Switches to PC.\n");
	fprintf(stderr, "  -t, --target        Switchest to target.\n");
	fprintf(stderr, "  -l, --location      Set location.\n");
	fprintf(stderr, "  -a, --address       Set address.\n");
	fprintf(stderr, "  -s, --serial        Serial number.\n");
	fprintf(stderr, "  -i, --id            Set the PID and VID.\n");
	fprintf(stderr, "  -e, --exact         Use exact location.\n");
	fprintf(stderr, "If no options are given, the script will print the current status");
	fprintf(stderr, "of the SDswap device.\n");
	fprintf(stderr, "If --print and --controller are given at the same time, the script will");
	fprintf(stderr, "wait for the\n");
	fprintf(stderr, "SDswap device to be attached to the PC and then print the block");
	fprintf(stderr, "device path.\n");
	exit(code);
}

static int do_swap(struct libusb_device *dev) {
	struct libusb_device_handle *devh = NULL;
	struct libusb_device_handle *hub = NULL;
	uint8_t port;
	int rc;

	if (opt_swap < 0 && opt_print == true)
		return EXIT_SUCCESS;

	rc = libusb_open(dev, &devh);
	if (rc < 0) {
		fprintf(stderr, "Error opening USB device: %s\n", libusb_error_name(rc));
		return rc;
	}

	if (opt_swap < 0) {
		rc = print_status(devh);
		goto out;
	}

	if (opt_swap) {
		rc = attach_kernel_driver(devh);
		goto out;
	}

	/* check for kernel driver here? */
	rc = detach_kernel_driver(devh);
	if (rc < 0)
		goto out;

	/* Already detached */
	if (rc == 1)
		goto out;

	port = libusb_get_port_number(dev);
	if (port == 0) {
		fprintf(stderr, "Error getting port.\n");
		goto out;
	}

	rc = libusb_open_parent(NULL, dev, &hub);
	if (rc < 0) {
		fprintf(stderr, "Error opening parent hub: %s\n", libusb_error_name(rc));
#ifdef __APPLE__
		fprintf(stderr, "Note: On macOS, opening root hub is not permitted.\n"
			"Plug device into a non-root hub.\n");
#endif
		goto out;
	}

	/* linux/drivers/usb/core/hub.c:usb_port_suspend:3507 */
	/* see 7.1.7.6 */
	if (is_superspeed(dev) && !opt_exact)
		rc = hub_set_port_link_state(hub, port, USB_SS_PORT_LS_U3);
	else
		rc = set_port_feature(hub, port, USB_PORT_FEAT_SUSPEND);
	if (rc < 0) {
		fprintf(stderr, "Error setting port status: %s\n", libusb_error_name(rc));
		goto out;
	}

out:
	if (hub)
		libusb_close(hub);

	libusb_close(devh);

	return rc;
}

static int do_print(struct libusb_device *dev) {
	char *blockdev_path = NULL;
	int rc;

	if (!opt_print)
		return EXIT_SUCCESS;

	do {
		rc = libusb_get_blockdev_path(dev, 0, &blockdev_path);
	} while (rc == LIBUSB_ERROR_NOT_FOUND && opt_swap == 1);

	if (rc < 0) {
		fprintf(stderr, "Error getting block device path: %s\n", libusb_error_name(rc));
		return rc;
	}

	if (blockdev_path) {
		printf("%s\n", blockdev_path);
		free(blockdev_path);
	} else {
		fprintf(stderr, "Error this should never happen.\n");
		return -EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

static int print_status(struct libusb_device_handle *devh) {
	int rc;

	rc = libusb_kernel_driver_active(devh, 0);
	if (rc < 0) {
		fprintf(stderr, "Error checking kernel driver: %s\n", libusb_error_name(rc));
		return rc;
	}

	if (rc == 1)
		printf("SDswap is attached to PC.\n");
	else
		printf("SDswap is attached to target.\n");

	return EXIT_SUCCESS;
}

static int attach_kernel_driver(struct libusb_device_handle *devh) {
	int rc, i = ATTACH_RETRY_CNT;

	rc = libusb_kernel_driver_active(devh, 0);
	if (rc < 0) {
		fprintf(stderr, "Error checking kernel driver: %s\n", libusb_error_name(rc));
		return rc;
	}

	if (rc == 1)
		return EXIT_SUCCESS;

	/* Attach kernel driver on interface 0 if it is attached */
	do {
		rc = libusb_attach_kernel_driver(devh, 0);
	} while (rc == LIBUSB_ERROR_TIMEOUT && i--);

	/* Macos fix */
	if (rc == LIBUSB_ERROR_NO_DEVICE)
		return EXIT_SUCCESS;

	if (rc < 0)
		fprintf(stderr, "Error attaching kernel driver: %s\n", libusb_error_name(rc));

	return rc;
}

static int detach_kernel_driver(struct libusb_device_handle *devh) {
	int rc;

	rc = libusb_kernel_driver_active(devh, 0);
	if (rc < 0) {
		fprintf(stderr, "Error checking kernel driver: %s\n", libusb_error_name(rc));
		return rc;
	}

	if (rc == 0)
		return 1;

	/* Detach kernel driver on interface 0 if it is attached */
	rc = libusb_detach_kernel_driver(devh, 0);
	if (rc < 0)
		fprintf(stderr, "Error detaching kernel driver: %s\n", libusb_error_name(rc));

	return rc;
}

static int set_port_feature(struct libusb_device_handle *devh, int port, uint16_t feature) {
	return libusb_control_transfer(devh, LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_OTHER,
				       LIBUSB_REQUEST_SET_FEATURE, feature, port, NULL, 0,
				       USB_CTRL_GET_TIMEOUT);
}

 static int hub_set_port_link_state(struct libusb_device_handle *devh, int port,
				    unsigned int link_status) {
	return set_port_feature(devh, port | (link_status << 3), USB_PORT_FEAT_LINK_STATE);
}

static int libusb_open_parent(libusb_context *ctx, libusb_device *child,
			      libusb_device_handle **devh) {
	struct libusb_device **devs;
	struct libusb_device *parent;
	int rc;

	rc = libusb_get_device_list(ctx, &devs);
	if (rc < 0)
		return rc;

	parent = libusb_get_parent(child);
	if (parent) {
		rc = libusb_open(parent, devh);
		if (rc < 0)
			*devh = NULL;
	} else {
		fprintf(stderr, "No parent device found.\n");
		*devh = NULL;
		rc = -EXIT_FAILURE;
	}

	libusb_free_device_list(devs, 1);

	return rc;
}

static int is_superspeed(struct libusb_device *dev) {
	struct libusb_device_descriptor desc;
	int rc;

	rc = libusb_get_device_descriptor(dev, &desc);
	if (rc < 0) {
		fprintf(stderr, "Error getting device descriptor: %s\n", libusb_error_name(rc));
		return rc;
	}

	if (desc.bcdUSB >= USB_SS_BCD)
		return 1;

	return 0;
}

static int check_serial(struct libusb_device * dev, uint8_t desc_index) {
	struct libusb_device_handle *devh = NULL;
	uint8_t buf[255];
	int rc;

	if (!opt_serial)
		return 0;

	rc = libusb_open(dev, &devh);
	if (rc < 0) {
		fprintf(stderr, "Error opening USB device: %s\n", libusb_error_name(rc));
		return rc;
	}

	rc = libusb_get_string_descriptor_ascii(devh, desc_index, buf, sizeof(buf));

	libusb_close(devh);

	if (rc < 0) {
		fprintf(stderr, "Error getting string descriptor: %s\n", libusb_error_name(rc));
		return rc;
	}

	if (strncmp((char *)buf, opt_serial, rc) == 0)
		return 0;

	return 1;
}

static int get_device(libusb_context *ctx, struct libusb_device **dev) {
	struct libusb_device_descriptor desc;
	struct libusb_device **dev_list;
	struct libusb_device *check = NULL;
	size_t i = 0;
	int rc;

	rc = libusb_get_device_list(ctx, &dev_list);
	if (rc < 0)
		return rc;

	while ((check = dev_list[i++]) != NULL) {
		rc = libusb_get_device_descriptor(check, &desc);
		if (rc < 0) {
			fprintf(stderr, "Error getting device descriptor: %s\n", libusb_error_name(rc));
			goto out;
		}

		if (desc.idVendor != opt_vendor_id || desc.idProduct != opt_product_id)
			continue;

		if (check_busaddr(check, opt_bus, opt_addr))
			continue;

		rc = check_path(check, opt_bus, opt_ports, opt_ports_len);
		if (rc < 0)
			goto out;

		if (rc)
			continue;

		rc = check_serial(check, desc.iSerialNumber);
		if (rc < 0)
			goto out;

		if (rc)
			continue;

		if (*dev) {
			*dev = NULL;
			fprintf(stderr, "Multiple devices found.\n");
			rc = -EXIT_FAILURE;
			goto out;
		}

		*dev = check;
	}

	if (*dev) {
		*dev = libusb_ref_device(*dev);
	} else {
		fprintf(stderr, "No device found.\n");
		rc = -EXIT_FAILURE;
	}

out:
	libusb_free_device_list(dev_list, 1);

	return rc;
}

static bool check_busaddr(libusb_device *dev, uint8_t bus, uint8_t addr) {
	if (bus == 0 || addr == 0)
		return false;

	if (libusb_get_bus_number(dev) != bus)
		return true;

	if (libusb_get_device_address(dev) != addr)
		return true;

	return false;
}

static int check_path(libusb_device *dev, uint8_t bus, uint8_t *ports, size_t ports_len) {
	uint8_t path[USB_MAX_DEPTH];
	int cnt;
	size_t i;

	if (bus == 0 || ports_len == 0)
		return 0;

	if (libusb_get_bus_number(dev) != bus)
		return 1;

	cnt = libusb_get_port_numbers(dev, path, sizeof(path));

	if (cnt < 0) {
		fprintf(stderr, "Error getting port numbers: %s\n", libusb_error_name(cnt));
		return cnt;
	}

	if ((size_t)cnt != ports_len)
		return 1;

	for (i = 0; i < ports_len; i++) {
		if (path[i] != ports[i])
			return 1;
	}

	return 0;
}

static int parse_id(const char *id, uint16_t *vendor_id, uint16_t *product_id) {
	uint32_t temp_vid, temp_pid;

	if (strlen(id) != 9)
		goto invalid;

	if (id[4] != ':')
		goto invalid;

	temp_vid = strtoul(id, NULL, 16);
	if (temp_vid == 0 || temp_vid > 0xffff)
		goto invalid;

	temp_pid = strtol(id + 5, NULL, 16);
	if (temp_pid == 0 || temp_pid > 0xffff)
		goto invalid;

	*vendor_id = (uint16_t)temp_vid;
	*product_id = (uint16_t)temp_pid;

	return EXIT_SUCCESS;

invalid:
	fprintf(stderr, "Invalid ID format:  %s\n", id);
	fprintf(stderr, "Expected format is: 1234:5678\n");
	return -EXIT_FAILURE;
}

static int parse_busaddr(const char *path, uint8_t *bus, uint8_t *addr) {
	uint32_t temp_bus, temp_addr;
	char *endptr;

	temp_bus = strtoul(path, &endptr, 10);
	if (temp_bus == 0 || temp_bus > 255)
		goto invalid;

	if (*endptr != '-')
		goto invalid;

	temp_addr = strtol(endptr + 1, NULL, 10);
	if (temp_addr == 0 || temp_addr > 255)
		goto invalid;

	*bus = (uint8_t)temp_bus;
	*addr = (uint8_t)temp_addr;

	return EXIT_SUCCESS;

invalid:
	fprintf(stderr, "Invalid format: %s\n", path);
	fprintf(stderr, "Expected format is: 1-2\n");
	return -EXIT_FAILURE;
}

static int parse_path(const char *path, uint8_t *bus, uint8_t *ports, size_t *ports_len) {
	uint32_t temp_bus, temp_port;
	size_t i;
	char *endptr;

	temp_bus = strtoul(path, &endptr, 10);
	if (temp_bus == 0 || temp_bus > 255)
		goto invalid;

	if (*endptr != '-')
		goto invalid;

	for (i = 0; *endptr; i++) {
		if (i >= USB_MAX_DEPTH) {
			fprintf(stderr, "Too many ports specified, max is %u\n", USB_MAX_DEPTH);
			goto invalid;
		}

		if (*endptr != '.' && i > 0)
			goto invalid;

		temp_port = strtol(endptr + 1, &endptr, 10);
		if (temp_port == 0 || temp_port > 255)
			goto invalid;

		ports[i] = (uint8_t)temp_port;
	}

	*bus = (uint8_t)temp_bus;
	*ports_len = i;

	return EXIT_SUCCESS;

invalid:
	fprintf(stderr, "Invalid format: %s\n", path);
	fprintf(stderr, "Expected format is: 1-2.1\n");
	return -EXIT_FAILURE;
}
