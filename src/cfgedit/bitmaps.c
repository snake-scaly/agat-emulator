#include <windows.h>
#include <stdio.h>

PBITMAPINFO CreateBitmapInfoStruct(HBITMAP hBmp)
{
	BITMAP bmp; 
	PBITMAPINFO pbmi; 
	WORD    cClrBits; 

	if (!GetObject(hBmp, sizeof(BITMAP), (LPSTR)&bmp)) return NULL;

	cClrBits = (WORD)(bmp.bmPlanes * bmp.bmBitsPixel); 
	if (cClrBits < 16) 
		pbmi = (PBITMAPINFO) LocalAlloc(LPTR, 
			sizeof(BITMAPINFOHEADER) + 
			sizeof(RGBQUAD) * (1<< cClrBits)); 
	else 
		pbmi = (PBITMAPINFO) LocalAlloc(LPTR, 
			sizeof(BITMAPINFOHEADER)); 

	if (!pbmi) return NULL;

	printf("bitmap: %ix%ix%i\n",
		bmp.bmWidth, bmp.bmHeight, bmp.bmBitsPixel);

	pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER); 
	pbmi->bmiHeader.biWidth = bmp.bmWidth; 
	pbmi->bmiHeader.biHeight = bmp.bmHeight; 
	pbmi->bmiHeader.biPlanes = bmp.bmPlanes; 
	pbmi->bmiHeader.biBitCount = bmp.bmBitsPixel; 
	if (cClrBits < 16) pbmi->bmiHeader.biClrUsed = (1<<cClrBits); 

	pbmi->bmiHeader.biCompression = BI_RGB; 

	pbmi->bmiHeader.biSizeImage = ((pbmi->bmiHeader.biWidth * cClrBits +31) & ~31) /8
                                  * pbmi->bmiHeader.biHeight; 
	pbmi->bmiHeader.biClrImportant = 0; 
	return pbmi; 
} 

BOOL CreateBMPFile(LPCTSTR pszFile, HDC hDC, HBITMAP hBMP) { 
	HANDLE hf;
	BITMAPFILEHEADER hdr;
	PBITMAPINFOHEADER pbih;
	LPBYTE lpBits;
	DWORD dwTotal;
	DWORD cb;
	BYTE *hp;
	DWORD dwTmp;
	PBITMAPINFO pbi;

	pbi = CreateBitmapInfoStruct(hBMP);
        if (!pbi) goto err0;

	pbih = (PBITMAPINFOHEADER) pbi; 

	lpBits = (LPBYTE) GlobalAlloc(GMEM_FIXED, pbih->biSizeImage);
	if (!lpBits) goto err1;

	if (!GetDIBits(hDC, hBMP, 0, (WORD) pbih->biHeight, lpBits, pbi, DIB_RGB_COLORS)) 
		goto err2;

	hf = CreateFile(pszFile, 
		GENERIC_READ | GENERIC_WRITE, 
		(DWORD) 0, 
		NULL, 
		CREATE_ALWAYS, 
		FILE_ATTRIBUTE_NORMAL, 
		(HANDLE) NULL); 
	if (hf == INVALID_HANDLE_VALUE) goto err2;

	hdr.bfType = 0x4d42; // Bitmap Signature
	hdr.bfSize = (DWORD) (sizeof(BITMAPFILEHEADER) + 
		pbih->biSize + pbih->biClrUsed 
		* sizeof(RGBQUAD) + pbih->biSizeImage); 
	hdr.bfReserved1 = 0; 
	hdr.bfReserved2 = 0; 
	hdr.bfOffBits = (DWORD) sizeof(BITMAPFILEHEADER) + 
		pbih->biSize + pbih->biClrUsed 
		* sizeof (RGBQUAD); 

	if (!WriteFile(hf, (LPVOID) &hdr, sizeof(BITMAPFILEHEADER), 
		(LPDWORD) &dwTmp,  NULL))  goto err3;

	if (!WriteFile(hf, (LPVOID) pbih, sizeof(BITMAPINFOHEADER) 
		+ pbih->biClrUsed * sizeof (RGBQUAD), 
		(LPDWORD) &dwTmp, ( NULL)))  goto err3;

	dwTotal = cb = pbih->biSizeImage; 
	hp = lpBits; 
	if (!WriteFile(hf, (LPSTR) hp, (int) cb, (LPDWORD) &dwTmp,NULL)) 
		goto err3;

	if (!CloseHandle(hf)) goto err2;
	GlobalFree((HGLOBAL)lpBits);
	LocalFree(pbi);

	return TRUE;

	// clean up resources and return error
err3:
	CloseHandle(hf);
err2:
	GlobalFree((HGLOBAL)lpBits);
err1:
	LocalFree(pbi);
err0:
	return FALSE;
}
