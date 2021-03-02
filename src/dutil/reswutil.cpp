// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.

#include "precomp.h"


// Exit macros
#define ReswExitOnLastError(x, s, ...) ExitOnLastErrorSource(DUTIL_SOURCE_RESWUTIL, x, s, __VA_ARGS__)
#define ReswExitOnLastErrorDebugTrace(x, s, ...) ExitOnLastErrorDebugTraceSource(DUTIL_SOURCE_RESWUTIL, x, s, __VA_ARGS__)
#define ReswExitWithLastError(x, s, ...) ExitWithLastErrorSource(DUTIL_SOURCE_RESWUTIL, x, s, __VA_ARGS__)
#define ReswExitOnFailure(x, s, ...) ExitOnFailureSource(DUTIL_SOURCE_RESWUTIL, x, s, __VA_ARGS__)
#define ReswExitOnRootFailure(x, s, ...) ExitOnRootFailureSource(DUTIL_SOURCE_RESWUTIL, x, s, __VA_ARGS__)
#define ReswExitOnFailureDebugTrace(x, s, ...) ExitOnFailureDebugTraceSource(DUTIL_SOURCE_RESWUTIL, x, s, __VA_ARGS__)
#define ReswExitOnNull(p, x, e, s, ...) ExitOnNullSource(DUTIL_SOURCE_RESWUTIL, p, x, e, s, __VA_ARGS__)
#define ReswExitOnNullWithLastError(p, x, s, ...) ExitOnNullWithLastErrorSource(DUTIL_SOURCE_RESWUTIL, p, x, s, __VA_ARGS__)
#define ReswExitOnNullDebugTrace(p, x, e, s, ...)  ExitOnNullDebugTraceSource(DUTIL_SOURCE_RESWUTIL, p, x, e, s, __VA_ARGS__)
#define ReswExitOnInvalidHandleWithLastError(p, x, s, ...) ExitOnInvalidHandleWithLastErrorSource(DUTIL_SOURCE_RESWUTIL, p, x, s, __VA_ARGS__)
#define ReswExitOnWin32Error(e, x, s, ...) ExitOnWin32ErrorSource(DUTIL_SOURCE_RESWUTIL, e, x, s, __VA_ARGS__)
#define ReswExitOnGdipFailure(g, x, s, ...) ExitOnGdipFailureSource(DUTIL_SOURCE_RESWUTIL, g, x, s, __VA_ARGS__)

#define RES_STRINGS_PER_BLOCK 16

// Internal data structure format for a string block in a resource table.
// Note: Strings are always stored as UNICODED.
typedef struct _RES_STRING_BLOCK
{
    DWORD dwBlockId;
    WORD wLangId;
    LPWSTR rgwz[RES_STRINGS_PER_BLOCK];
} RES_STRING_BLOCK;


// private functions
static HRESULT StringBlockInitialize(
    __in_opt HINSTANCE hModule, 
    __in DWORD dwBlockId,
    __in WORD wLangId,
    __in RES_STRING_BLOCK* pStrBlock
    );
static void StringBlockUnitialize(
    __in RES_STRING_BLOCK* pStrBlock
    );
static HRESULT StringBlockChangeString(
    __in RES_STRING_BLOCK* pStrBlock,
    __in DWORD dwStringId,
    __in_z LPCWSTR szData
    );
static HRESULT StringBlockConvertToResourceData(
    __in const RES_STRING_BLOCK* pStrBlock,
    __deref_out_bcount(*pcbData) LPVOID* ppvData,
    __out DWORD* pcbData
    );
static HRESULT StringBlockConvertFromResourceData(
    __in RES_STRING_BLOCK* pStrBlock,
    __in_bcount(cbData) LPCVOID pvData,
    __in SIZE_T cbData
    );


/********************************************************************
ResWriteString - sets the string into to the specified file's resource name

********************************************************************/
extern "C" HRESULT DAPI ResWriteString(
    __in_z LPCWSTR wzResourceFile,
    __in DWORD dwDataId,
    __in_z LPCWSTR wzData,
    __in WORD wLangId
    )
{
    Assert(wzResourceFile);
    Assert(wzData);

    HRESULT hr = S_OK;
    HINSTANCE hModule = NULL;
    HANDLE hUpdate = NULL;
    RES_STRING_BLOCK StrBlock = { };
    LPVOID pvData = NULL;
    DWORD cbData = 0;

    DWORD dwBlockId = (dwDataId / RES_STRINGS_PER_BLOCK) + 1;
    DWORD dwStringId = (dwDataId % RES_STRINGS_PER_BLOCK);

    hModule = LoadLibraryExW(wzResourceFile, NULL, DONT_RESOLVE_DLL_REFERENCES | LOAD_LIBRARY_AS_DATAFILE);
    ReswExitOnNullWithLastError(hModule, hr, "Failed to load library: %ls", wzResourceFile);

    hr = StringBlockInitialize(hModule, dwBlockId, wLangId, &StrBlock);
    ReswExitOnFailure(hr, "Failed to get string block to update.");

    hr = StringBlockChangeString(&StrBlock, dwStringId, wzData);
    ReswExitOnFailure(hr, "Failed to update string block string.");

    hr = StringBlockConvertToResourceData(&StrBlock, &pvData, &cbData);
    ReswExitOnFailure(hr, "Failed to convert string block to resource data.");

    ::FreeLibrary(hModule);
    hModule = NULL;

    hUpdate = ::BeginUpdateResourceW(wzResourceFile, FALSE);
    ReswExitOnNullWithLastError(hUpdate, hr, "Failed to ::BeginUpdateResourcesW.");

    if (!::UpdateResourceA(hUpdate, RT_STRING, MAKEINTRESOURCE(dwBlockId), wLangId, pvData, cbData))
    {
        ReswExitWithLastError(hr, "Failed to ::UpdateResourceA.");
    }

    if (!::EndUpdateResource(hUpdate, FALSE))
    {
        ReswExitWithLastError(hr, "Failed to ::EndUpdateResourceW.");
    }

    hUpdate = NULL;

LExit:
    ReleaseMem(pvData);

    StringBlockUnitialize(&StrBlock);

    if (hUpdate)
    {
        ::EndUpdateResource(hUpdate, TRUE);
    }

    if (hModule)
    {
        ::FreeLibrary(hModule);
    }

    return hr;
}


/********************************************************************
ResWriteData - sets the data into to the specified file's resource name

********************************************************************/
extern "C" HRESULT DAPI ResWriteData(
    __in_z LPCWSTR wzResourceFile,
    __in_z LPCSTR szDataName,
    __in PVOID pData,
    __in DWORD cbData
    )
{
    Assert(wzResourceFile);
    Assert(szDataName);
    Assert(pData);
    Assert(cbData);

    HRESULT hr = S_OK;
    HANDLE hUpdate = NULL;

    hUpdate = ::BeginUpdateResourceW(wzResourceFile, FALSE);
    ReswExitOnNullWithLastError(hUpdate, hr, "Failed to ::BeginUpdateResourcesW.");

    if (!::UpdateResourceA(hUpdate, RT_RCDATA, szDataName, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), pData, cbData))
    {
        ReswExitWithLastError(hr, "Failed to ::UpdateResourceA.");
    }

    if (!::EndUpdateResource(hUpdate, FALSE))
    {
        ReswExitWithLastError(hr, "Failed to ::EndUpdateResourceW.");
    }

    hUpdate = NULL;

LExit:
    if (hUpdate)
    {
        ::EndUpdateResource(hUpdate, TRUE);
    }

    return hr;
}


/********************************************************************
ResImportDataFromFile - reads a file and sets the data into to the specified file's resource name

********************************************************************/
extern "C" HRESULT DAPI ResImportDataFromFile(
    __in_z LPCWSTR wzTargetFile,
    __in_z LPCWSTR wzSourceFile,
    __in_z LPCSTR szDataName
    )
{
    HRESULT hr = S_OK;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD cbFile = 0;
    HANDLE hMap = NULL;
    PVOID pv = NULL;

    hFile = ::CreateFileW(wzSourceFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE == hFile)
    {
        ReswExitWithLastError(hr, "Failed to CreateFileW for %ls.", wzSourceFile);
    }

    cbFile = ::GetFileSize(hFile, NULL);
    if (!cbFile)
    {
        ReswExitWithLastError(hr, "Failed to GetFileSize for %ls.", wzSourceFile);
    }

    hMap = ::CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    ReswExitOnNullWithLastError(hMap, hr, "Failed to CreateFileMapping for %ls.", wzSourceFile);

    pv = ::MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, cbFile);
    ReswExitOnNullWithLastError(pv, hr, "Failed to MapViewOfFile for %ls.", wzSourceFile);

    hr = ResWriteData(wzTargetFile, szDataName, pv, cbFile);
    ReswExitOnFailure(hr, "Failed to ResSetData %s on file %ls.", szDataName, wzTargetFile);

LExit:
    if (pv)
    {
        ::UnmapViewOfFile(pv);
    }

    if (hMap)
    {
        ::CloseHandle(hMap);
    }

    ReleaseFile(hFile);

    return hr;
}


static HRESULT StringBlockInitialize(
    __in_opt HINSTANCE hModule, 
    __in DWORD dwBlockId,
    __in WORD wLangId,
    __in RES_STRING_BLOCK* pStrBlock
    )
{
    HRESULT hr = S_OK;
    HRSRC hRsrc = NULL;
    HGLOBAL hData = NULL;
    LPCVOID pvData = NULL; // does not need to be freed
    DWORD cbData = 0;

    hRsrc = ::FindResourceExA(hModule, RT_STRING, MAKEINTRESOURCE(dwBlockId), wLangId);
    ReswExitOnNullWithLastError(hRsrc, hr, "Failed to ::FindResourceExW.");

    hData = ::LoadResource(hModule, hRsrc);
    ReswExitOnNullWithLastError(hData, hr, "Failed to ::LoadResource.");

    cbData = ::SizeofResource(hModule, hRsrc);
    if (!cbData)
    {
        ReswExitWithLastError(hr, "Failed to ::SizeofResource.");
    }

    pvData = ::LockResource(hData);
    ReswExitOnNullWithLastError(pvData, hr, "Failed to lock data resource.");

    pStrBlock->dwBlockId = dwBlockId;
    pStrBlock->wLangId = wLangId;

    hr = StringBlockConvertFromResourceData(pStrBlock, pvData, cbData);
    ReswExitOnFailure(hr, "Failed to convert string block from resource data.");

LExit:
    return hr;
}


static void StringBlockUnitialize(
    __in RES_STRING_BLOCK* pStrBlock
    )
{
    if (pStrBlock)
    {
        for (DWORD i = 0; i < RES_STRINGS_PER_BLOCK; ++i)
        {
            ReleaseNullMem(pStrBlock->rgwz[i]);
        }
    }
}


static HRESULT StringBlockChangeString(
    __in RES_STRING_BLOCK* pStrBlock,
    __in DWORD dwStringId,
    __in_z LPCWSTR szData
    )
{
    HRESULT hr = S_OK;
    LPWSTR pwzData = NULL;
    DWORD cchData = lstrlenW(szData);

    pwzData = static_cast<LPWSTR>(MemAlloc((cchData + 1) * sizeof(WCHAR), TRUE));
    ReswExitOnNull(pwzData, hr, E_OUTOFMEMORY, "Failed to allocate new block string.");

    hr = ::StringCchCopyW(pwzData, cchData + 1, szData);
    ReswExitOnFailure(hr, "Failed to copy new block string.");

    ReleaseNullMem(pStrBlock->rgwz[dwStringId]);

    pStrBlock->rgwz[dwStringId] = pwzData;
    pwzData = NULL;

LExit:
    ReleaseMem(pwzData);

    return hr;
}


static HRESULT StringBlockConvertToResourceData(
    __in const RES_STRING_BLOCK* pStrBlock,
    __deref_out_bcount(*pcbData) LPVOID* ppvData,
    __out DWORD* pcbData
    )
{
    HRESULT hr = S_OK;
    DWORD cbData = 0;
    LPVOID pvData = NULL;
    WCHAR* pwz = NULL;

    for (DWORD i = 0; i < RES_STRINGS_PER_BLOCK; ++i)
    {
        cbData += (lstrlenW(pStrBlock->rgwz[i]) + 1);
    }
    cbData *= sizeof(WCHAR);

    pvData = MemAlloc(cbData, TRUE);
    ReswExitOnNull(pvData, hr, E_OUTOFMEMORY, "Failed to allocate buffer to convert string block.");

    pwz = static_cast<LPWSTR>(pvData);
    for (DWORD i = 0; i < RES_STRINGS_PER_BLOCK; ++i)
    {
        DWORD cch = lstrlenW(pStrBlock->rgwz[i]);

        *pwz = static_cast<WCHAR>(cch);
        ++pwz;

        for (DWORD j = 0; j < cch; ++j)
        {
            *pwz = pStrBlock->rgwz[i][j];
            ++pwz;
        }
    }

    *pcbData = cbData;
    *ppvData = pvData;
    pvData = NULL;

LExit:
    ReleaseMem(pvData);

    return hr;
}


static HRESULT StringBlockConvertFromResourceData(
    __in RES_STRING_BLOCK* pStrBlock,
    __in_bcount(cbData) LPCVOID pvData,
    __in SIZE_T cbData
    )
{
    UNREFERENCED_PARAMETER(cbData);
    HRESULT hr = S_OK;
    LPCWSTR pwzParse = static_cast<LPCWSTR>(pvData);

    for (DWORD i = 0; i < RES_STRINGS_PER_BLOCK; ++i)
    {
        DWORD cchParse = static_cast<DWORD>(*pwzParse);
        ++pwzParse;

        pStrBlock->rgwz[i] = static_cast<LPWSTR>(MemAlloc((cchParse + 1) * sizeof(WCHAR), TRUE));
        ReswExitOnNull(pStrBlock->rgwz[i], hr, E_OUTOFMEMORY, "Failed to populate pStrBlock.");

        hr = ::StringCchCopyNExW(pStrBlock->rgwz[i], cchParse + 1, pwzParse, cchParse, NULL, NULL, STRSAFE_FILL_BEHIND_NULL);
        ReswExitOnFailure(hr, "Failed to copy parsed resource data into string block.");

        pwzParse += cchParse;
    }

LExit:
    return hr;
}
