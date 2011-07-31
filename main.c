/* 
 * Copyright (C) 2009-2011 Chris McClelland
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *  
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <liberror.h>
#include <libusbwrap.h>
#include <usb.h>
#include <argtable2.h>
#ifdef WIN32
#include <fcntl.h>
#include <io.h>
#endif

#define BUFFER_SIZE 4096

int main(int argc, char* argv[]) {

	struct arg_str *vpOpt = arg_str1("v", "vidpid", "<VID:PID>", " vendor ID and product ID (e.g 04B4:8613)");
	struct arg_lit *inOpt  = arg_lit0("i", "in", "             this is an IN message (device->host)");
	struct arg_lit *outOpt  = arg_lit0("o", "out", "             this is an OUT message (host->device)");
	struct arg_file *fileOpt = arg_file0("f", "file", "<fileName>", "  file to read from or write to (default stdin/stdout)");
	struct arg_lit *helpOpt  = arg_lit0("h", "help", "             print this help and exit\n");
	struct arg_uint *reqOpt = arg_uint1(NULL, NULL, "<bRequest>", "             the bRequest byte");
	struct arg_uint *valOpt = arg_uint1(NULL, NULL, "<wValue>", "             the wValue word");
	struct arg_uint *idxOpt = arg_uint1(NULL, NULL, "<wIndex>", "             the wIndex word");
	struct arg_uint *lenOpt = arg_uint1(NULL, NULL, "<wLength>", "             the wLength word");
	struct arg_end *endOpt   = arg_end(20);
	void *argTable[] = {vpOpt, inOpt, outOpt, fileOpt, helpOpt, reqOpt, valOpt, idxOpt, lenOpt, endOpt};
	const char *progName = "ucm";
	int returnCode = 0;
	int numErrors;
	const char *error = NULL;

	uint8 bRequest;
	uint16 wValue, wIndex, wLength;
	char buffer[BUFFER_SIZE];
	bool isOut = false;
	FILE *outFile = NULL;
	struct usb_dev_handle *deviceHandle = NULL;

	if ( arg_nullcheck(argTable) != 0 ) {
		printf("%s: insufficient memory\n", progName);
		FAIL(1);
	}

	numErrors = arg_parse(argc, argv, argTable);

	if ( helpOpt->count > 0 ) {
		printf("USB Control Message Tool Copyright (C) 2009-2011 Chris McClelland\n\nUsage: %s", progName);
		arg_print_syntax(stdout, argTable, "\n");
		printf("\nInteract with a USB device's control endpoint.\n\n");
		arg_print_glossary(stdout, argTable,"  %-10s %s\n");
		FAIL(0);
	}

	if ( numErrors > 0 ) {
		arg_print_errors(stdout, endOpt, progName);
		printf("Try '%s --help' for more information.\n", progName);
		FAIL(2);
	}

	if ( inOpt->count && outOpt->count ) {
		fprintf(stderr, "You cannot supply both -i and -o\n");
		FAIL(3);
	} else if ( inOpt->count ) {
		isOut = false;
	} else if ( outOpt->count ) {
		isOut = true;
	} else {
		fprintf(stderr, "You must supply either -i or -o\n");
		FAIL(4);
	}

	bRequest = (uint8)reqOpt->ival[0];
	wValue = (uint16)valOpt->ival[0];
	wIndex = (uint16)idxOpt->ival[0];
	wLength = (uint16)lenOpt->ival[0];

	if ( wLength > BUFFER_SIZE ) {
		fprintf(stderr, "Cannot %s more than %d bytes\n", isOut?"write":"read", BUFFER_SIZE);
		FAIL(5);
	}

	if ( isOut ) {
		if ( fileOpt->count ) {
			// Read OUT data from specified file
			//
			size_t bytesRead;
			FILE *inFile = fopen(fileOpt->filename[0], "rb");
			if ( inFile == NULL ) {
				fprintf(stderr, "Cannot open file %s\n", argv[6]);
				FAIL(6);
			}
			bytesRead = fread(buffer, 1, wLength, inFile);
			fclose(inFile);
			if ( bytesRead != wLength ) {
				fprintf(
					stderr, "Whilst reading from \"%s\", expected 0x%04X bytes but got 0x%04X\n",
					argv[6], wLength, (unsigned int)bytesRead);
				FAIL(7);
			}
		} else {
			// Read OUT data from stdin
			//
			uint32 bytesRead;
			#ifdef WIN32
				_setmode(_fileno(stdin), O_BINARY);
			#endif
			bytesRead = fread(buffer, 1, wLength, stdin);
			if ( bytesRead != wLength ) {
				fprintf(stderr, "Unable to read %d bytes from stdin\n", wLength);
				FAIL(8);
			}
		}
	} else {
		if ( fileOpt->count ) {
			// Write IN data to specified file
			//
			outFile = fopen(fileOpt->filename[0], "wb");
		} else {
			// Write IN data to stdout
			//
			#ifdef WIN32
				_setmode(_fileno(stdout), O_BINARY);
			#endif
			outFile = stdout;
		}
	}

	usbInitialise();
	returnCode = usbOpenDeviceVP(vpOpt->sval[0], 1, 0, 0, &deviceHandle, &error);
	if ( returnCode ) {
		fprintf(stderr, "%s\n", error);
		errFree(error);
		FAIL(9);
	}
	returnCode = usb_control_msg(
		deviceHandle,
		((isOut?USB_ENDPOINT_OUT:USB_ENDPOINT_IN) | USB_TYPE_VENDOR | USB_RECIP_DEVICE),
		bRequest, wValue, wIndex, buffer, wLength, 5000
	);
	if ( isOut ) {
		if ( returnCode != wLength ) {
			fprintf(stderr, "Expected to write 0x%04X bytes but actually wrote 0x%04X: %s\n", wLength, returnCode, usb_strerror());
			FAIL(10);
		}
	} else {
		if ( returnCode < 0 ) {
			fprintf(stderr, "usb_control_msg() failed returnCode %d: %s\n", returnCode, usb_strerror());
			FAIL(11);
		} else if ( returnCode > 0 ) {
			fwrite(buffer, 1, returnCode, outFile);
		}
	}

cleanup:
	if ( deviceHandle ) {
		usb_release_interface(deviceHandle, 0);
		usb_close(deviceHandle);
	}
	if ( outFile != NULL && outFile != stdout ) {
		fclose(outFile);
	}
	arg_freetable(argTable, sizeof(argTable)/sizeof(argTable[0]));
	return returnCode;
}
