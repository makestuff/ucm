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
#include <makestuff/liberror.h>
#include <makestuff/libusbwrap.h>
#include <sheitmann/libargtable2.h>
#ifdef WIN32
  #include <fcntl.h>
  #include <io.h>
#endif

#define BUFFER_SIZE 4096

int main(int argc, char* argv[]) {
	struct arg_str *vpOpt = arg_str1("v", "vidpid", "<VID:PID>", " vendor ID and product ID (e.g 04B4:8613)");
	struct arg_uint *toOpt = arg_uint0("t", "timeout", "<millis>", " timeout in milliseconds");
	struct arg_lit *inOpt  = arg_lit0("i", "in", "             this is an IN message (device->host)");
	struct arg_lit *outOpt  = arg_lit0("o", "out", "             this is an OUT message (host->device)");
	struct arg_file *fileOpt = arg_file0("f", "file", "<fileName>", "  file to read from or write to (default stdin/stdout)");
	struct arg_lit *helpOpt  = arg_lit0("h", "help", "             print this help and exit\n");
	struct arg_uint *reqOpt = arg_uint1(NULL, NULL, "<bRequest>", "             the bRequest byte");
	struct arg_uint *valOpt = arg_uint1(NULL, NULL, "<wValue>", "             the wValue word");
	struct arg_uint *idxOpt = arg_uint1(NULL, NULL, "<wIndex>", "             the wIndex word");
	struct arg_uint *lenOpt = arg_uint1(NULL, NULL, "<wLength>", "             the wLength word");
	struct arg_end *endOpt   = arg_end(20);
	void *argTable[] = {vpOpt, toOpt, inOpt, outOpt, fileOpt, helpOpt, reqOpt, valOpt, idxOpt, lenOpt, endOpt};
	const char *progName = "ucm";
	int retVal = 0;
	int numErrors;
	uint32 timeout = 5000;
	const char *error = NULL;
	uint8 bRequest;
	uint16 wValue, wIndex, wLength;
	uint8 buffer[BUFFER_SIZE];
	bool isOut = false;
	FILE *outFile = NULL;
	struct USBDevice *deviceHandle = NULL;

	if ( arg_nullcheck(argTable) != 0 ) {
		printf("%s: insufficient memory\n", progName);
		FAIL_RET(1, cleanup);
	}

	numErrors = arg_parse(argc, argv, argTable);
	if ( helpOpt->count > 0 ) {
		printf("USB Control Message Tool Copyright (C) 2009-2011 Chris McClelland\n\nUsage: %s", progName);
		arg_print_syntax(stdout, argTable, "\n");
		printf("\nInteract with a USB device's control endpoint.\n\n");
		arg_print_glossary(stdout, argTable,"  %-10s %s\n");
		FAIL_RET(0, cleanup);
	}

	if ( numErrors > 0 ) {
		arg_print_errors(stdout, endOpt, progName);
		printf("Try '%s --help' for more information.\n", progName);
		FAIL_RET(2, cleanup);
	}

	if ( toOpt->count ) {
		timeout = toOpt->ival[0];
	}

	if ( inOpt->count && outOpt->count ) {
		fprintf(stderr, "You cannot supply both -i and -o\n");
		FAIL_RET(3, cleanup);
	} else if ( inOpt->count ) {
		isOut = false;
	} else if ( outOpt->count ) {
		isOut = true;
	} else {
		fprintf(stderr, "You must supply either -i or -o\n");
		FAIL_RET(4, cleanup);
	}

	bRequest = (uint8)reqOpt->ival[0];
	wValue = (uint16)valOpt->ival[0];
	wIndex = (uint16)idxOpt->ival[0];
	wLength = (uint16)lenOpt->ival[0];
	if ( wLength > BUFFER_SIZE ) {
		fprintf(stderr, "Cannot %s more than %d bytes\n", isOut?"write":"read", BUFFER_SIZE);
		FAIL_RET(5, cleanup);
	}

	if ( isOut ) {
		size_t bytesRead;
		if ( fileOpt->count ) {
			// Read OUT data from specified file
			//
			FILE *inFile = fopen(fileOpt->filename[0], "rb");
			if ( inFile == NULL ) {
				fprintf(stderr, "Cannot open file %s\n", argv[6]);
				FAIL_RET(6, cleanup);
			}
			bytesRead = fread(buffer, 1, wLength, inFile);
			fclose(inFile);
			if ( bytesRead != wLength ) {
				fprintf(
					stderr, "Whilst reading from \"%s\", expected 0x%04X bytes but got 0x%04X\n",
					fileOpt->filename[0], wLength, (unsigned int)bytesRead);
				FAIL_RET(7, cleanup);
			}
		} else {
			// Read OUT data from stdin
			//
			#ifdef WIN32
				_setmode(_fileno(stdin), O_BINARY);
			#endif
			bytesRead = fread(buffer, 1, wLength, stdin);
			if ( bytesRead != wLength ) {
				fprintf(stderr, "Unable to read %d bytes from stdin\n", wLength);
				FAIL_RET(8, cleanup);
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

	CHECK_STATUS(usbInitialise(0, &error), 9, cleanup);
	CHECK_STATUS(usbOpenDevice(vpOpt->sval[0], 1, 0, 0, &deviceHandle, &error), 10, cleanup);

	if ( isOut ) {
		CHECK_STATUS(
			usbControlWrite(deviceHandle, bRequest, wValue, wIndex, buffer, wLength, timeout, &error),
			11, cleanup);
	} else {
		size_t bytesWritten;
		CHECK_STATUS(
			usbControlRead(deviceHandle, bRequest, wValue, wIndex, buffer, wLength, timeout, &error),
			12, cleanup);
		bytesWritten = fwrite(buffer, 1, wLength, outFile);
		if ( bytesWritten != wLength ) {
			if ( outFile == stdout ) {
				fprintf(stderr, "Unable to write %d bytes to stdout\n", wLength);
			} else {
				fprintf(stderr, "Unable to write %d bytes to \"%s\"\n", wLength, fileOpt->filename[0]);
			}
			FAIL_RET(13, cleanup);
		}
	}

cleanup:
	if ( deviceHandle ) {
		usbCloseDevice(deviceHandle, 0);
	}
	if ( outFile != NULL && outFile != stdout ) {
		fclose(outFile);
	}
	if ( error ) {
		fprintf(stderr, "%s\n", error);
		usbFreeError(error);
	}
	arg_freetable(argTable, sizeof(argTable)/sizeof(argTable[0]));
	return retVal;
}
