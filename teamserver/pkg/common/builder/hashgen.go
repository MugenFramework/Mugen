package builder

import (
	"fmt"
	"os"
	"regexp"
	"strings"
	"unicode/utf16"
)

// hashExAscii mirrors Demon's HashEx(s, 0, TRUE): djb2 seeded with key, uppercase ASCII.
// Used for H_FUNC_ constants.
func hashExAscii(s string, key uint32) uint32 {
	h := key
	for _, c := range s {
		ch := uint32(c)
		if ch >= 'a' {
			ch -= 0x20
		}
		h = (h<<5 + h) + ch
	}
	return h
}

// hashExWide mirrors Demon's HashEx(wcharBuf, byteLen, TRUE): byte-by-byte iteration of a
// UTF-16LE buffer with Upper=TRUE. Null bytes (high bytes of ASCII-range wide chars) are
// hashed as 0 and cause an extra pointer advance, which is the behaviour the C algorithm
// exhibits when called with a non-zero Length on a UNICODE_STRING buffer.
// Used for H_MODULE_ constants.
func hashExWide(s string, key uint32) uint32 {
	// Encode as UTF-16LE exactly as Windows stores DLL names in UNICODE_STRING.
	runes := []rune(s)
	u16 := utf16.Encode(runes)
	var buf []byte
	for _, w := range u16 {
		buf = append(buf, byte(w), byte(w>>8))
	}
	length := len(buf)
	h := key
	ptr := 0
	for ptr < length {
		ch := uint32(buf[ptr])
		if ch == 0 {
			ptr++ // extra advance past null byte
		}
		if ch >= 'a' {
			ch -= 0x20
		}
		h = (h<<5 + h) + ch
		ptr++
	}
	return h
}

// hashStringA mirrors Demon's HashStringA: djb2 seeded with key, case-sensitive.
// Used for H_COFFAPI_ constants.
func hashStringA(s string, key uint32) uint32 {
	h := key
	for _, c := range s {
		h = (h<<5 + h) + uint32(c)
	}
	return h
}

// coffApiStrings maps H_COFFAPI_ suffix (uppercase) to the original mixed-case string
// used when the constants were first generated. These are case-sensitive hashes.
var coffApiStrings = map[string]string{
	// Beacon data API
	"BEACONDATAPARSER":             "BeaconDataParse", // intentional: original string has no trailing 'r'
	"BEACONDATAINT":                "BeaconDataInt",
	"BEACONDATASHORT":              "BeaconDataShort",
	"BEACONDATALENGTH":             "BeaconDataLength",
	"BEACONDATAEXTRACT":            "BeaconDataExtract",
	// Beacon format API
	"BEACONFORMATALLOC":            "BeaconFormatAlloc",
	"BEACONFORMATRESET":            "BeaconFormatReset",
	"BEACONFORMATFREE":             "BeaconFormatFree",
	"BEACONFORMATAPPEND":           "BeaconFormatAppend",
	"BEACONFORMATPRINTF":           "BeaconFormatPrintf",
	"BEACONFORMATTOSTRING":         "BeaconFormatToString",
	"BEACONFORMATINT":              "BeaconFormatInt",
	// Beacon output / control
	"BEACONPRINTF":                 "BeaconPrintf",
	"BEACONOUTPUT":                 "BeaconOutput",
	"BEACONUSETOKEN":               "BeaconUseToken",
	"BEACONREVERTTOKEN":            "BeaconRevertToken",
	"BEACONISADMIN":                "BeaconIsAdmin",
	"BEACONGETSPAWNTO":             "BeaconGetSpawnTo",
	"BEACONSPAWNTEMPORARYPROCESS":  "BeaconSpawnTemporaryProcess",
	"BEACONINJECTPROCESS":          "BeaconInjectProcess",
	"BEACONINJECTTEMPORARYPROCESS": "BeaconInjectTemporaryProcess",
	"BEACONCLEANUPPROCESS":         "BeaconCleanupProcess",
	"BEACONINFORMATION":            "BeaconInformation",
	"BEACONADDVALUE":               "BeaconAddValue",
	"BEACONGETVALUE":               "BeaconGetValue",
	"BEACONREMOVEVALUE":            "BeaconRemoveValue",
	"BEACONDATASTOREGETITEM":       "BeaconDataStoreGetItem",
	"BEACONDATASTOREPROTECTITEM":   "BeaconDataStoreProtectItem",
	"BEACONDATASTOREUNPROTECTITEM": "BeaconDataStoreUnprotectItem",
	"BEACONDATASTOREMAXENTRIES":    "BeaconDataStoreMaxEntries",
	"BEACONGETCUSTOMUSERDATA":      "BeaconGetCustomUserData",
	// Windows API shims used by COFF loader
	"TOWIDECHAR":                   "toWideChar",
	"LOADLIBRARYA":                 "LoadLibraryA",
	"GETPROCADDRESS":               "GetProcAddress",
	"GETMODULEHANDLE":              "GetModuleHandleA",
	"FREELIBRARY":                  "FreeLibrary",
	"LOCALFREE":                    "LocalFree",
	// NT native API
	"NTOPENTHREAD":                  "NtOpenThread",
	"NTOPENPROCESS":                 "NtOpenProcess",
	"NTTERMINATEPROCESS":            "NtTerminateProcess",
	"NTOPENTHREADTOKEN":             "NtOpenThreadToken",
	"NTOPENPROCESSTOKEN":            "NtOpenProcessToken",
	"NTDUPLICATETOKEN":              "NtDuplicateToken",
	"NTQUEUEAPCTHREAD":              "NtQueueApcThread",
	"NTSUSPENDTHREAD":               "NtSuspendThread",
	"NTRESUMETHREAD":                "NtResumeThread",
	"NTCREATEEVENT":                 "NtCreateEvent",
	"NTCREATETHREADEX":              "NtCreateThreadEx",
	"NTDUPLICATEOBJECT":             "NtDuplicateObject",
	"NTGETCONTEXTTHREAD":            "NtGetContextThread",
	"NTSETCONTEXTTHREAD":            "NtSetContextThread",
	"NTQUERYINFORMATIONPROCESS":     "NtQueryInformationProcess",
	"NTQUERYSYSTEMINFORMATION":      "NtQuerySystemInformation",
	"NTWAITFORSINGLEOBJECT":         "NtWaitForSingleObject",
	"NTALLOCATEVIRTUALMEMORY":       "NtAllocateVirtualMemory",
	"NTWRITEVIRTUALMEMORY":          "NtWriteVirtualMemory",
	"NTFREEVIRTUALMEMORY":           "NtFreeVirtualMemory",
	"NTUNMAPVIEWOFSECTION":          "NtUnmapViewOfSection",
	"NTPROTECTVIRTUALMEMORY":        "NtProtectVirtualMemory",
	"NTREADVIRTUALMEMORY":           "NtReadVirtualMemory",
	"NTTERMINATETHREAD":             "NtTerminateThread",
	"NTALERTRESUMETHREAD":           "NtAlertResumeThread",
	"NTSIGNALANDWAITFORSINGLEOBJECT": "NtSignalAndWaitForSingleObject",
	"NTQUERYVIRTUALMEMORY":          "NtQueryVirtualMemory",
	"NTQUERYINFORMATIONTOKEN":       "NtQueryInformationToken",
	"NTQUERYINFORMATIONTHREAD":      "NtQueryInformationThread",
	"NTQUERYOBJECT":                 "NtQueryObject",
	"NTCLOSE":                       "NtClose",
	"NTSETINFORMATIONTHREAD":        "NtSetInformationThread",
	"NTSETINFORMATIONVIRTUALMEMORY": "NtSetInformationVirtualMemory",
	"NTGETNEXTTHREAD":               "NtGetNextThread",
}

var reHashLine = regexp.MustCompile(`^(#define\s+H_(FUNC|MODULE|COFFAPI)_(\w+)\s+)(0x[0-9a-fA-F]+)`)

// recomputeDefinesH reads the original Defines.h, recomputes every H_FUNC_, H_MODULE_,
// and H_COFFAPI_ constant with hashKey, and returns the new content.
// Lines for unknown H_COFFAPI_ suffixes are left unchanged so nothing breaks silently.
func recomputeDefinesH(originalPath string, hashKey uint32) (string, error) {
	data, err := os.ReadFile(originalPath)
	if err != nil {
		return "", err
	}

	lines := strings.Split(string(data), "\n")
	for i, line := range lines {
		m := reHashLine.FindStringSubmatch(line)
		if m == nil {
			continue
		}
		// m[1] = prefix  (#define H_FOO_BAR   )
		// m[2] = kind    (FUNC | MODULE | COFFAPI)
		// m[3] = suffix  (NTCONTINUE, NTDLL, BEACONPRINTF, ...)
		// m[4] = old hex value
		prefix := m[1]
		kind := m[2]
		suffix := m[3]

		var newHash uint32
		switch kind {
		case "FUNC":
			// H_FUNC_NAME: HashEx(NAME, 0, TRUE) - uppercase null-terminated ASCII
			newHash = hashExAscii(suffix, hashKey)

		case "MODULE":
			// H_MODULE_NAME: HashEx(wcharBuf, byteLen, TRUE) on the DLL's UNICODE_STRING.
			// The DLL name is suffix.lower() + ".dll" as stored in the PEB.
			dllName := strings.ToLower(suffix) + ".dll"
			newHash = hashExWide(dllName, hashKey)

		case "COFFAPI":
			// H_COFFAPI_NAME: HashStringA(originalMixedCaseName, key) - case-sensitive
			original, ok := coffApiStrings[suffix]
			if !ok {
				continue // leave unknown entries unchanged
			}
			newHash = hashStringA(original, hashKey)
		}

		lines[i] = prefix + fmt.Sprintf("0x%08x", newHash)
	}

	return strings.Join(lines, "\n"), nil
}
