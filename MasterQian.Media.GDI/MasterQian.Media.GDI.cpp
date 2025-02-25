﻿#include "../include/MasterQian.Meta.h"
#include <Windows.h>
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")
import MasterQian.freestanding;
import MasterQian.Media.Color;
using namespace MasterQian;
#define MasterQianModuleName(name) MasterQian_Media_GDI_##name
META_EXPORT_API_VERSION(20240131ULL)

extern "C" {
	mqf32 __cdecl cosf(mqf32 _X);
	mqf32 __cdecl sinf(mqf32 _X);
}

mqui64 GDIHANDLE;

struct GDIText {
	mqcstr content;
	mqui32 size;
	Media::Color color;
	mqcstr font;
};

enum class ImageFormat : mqenum {
	BMP = 0, JPG = 1, GIF = 2, TIFF = 5, PNG = 6,
	ICO = 10,
	TGA = 20,
	MNG = 30,
	RAW = 40,
	PSD = 50,
	UNKNOWN = static_cast<mqenum>(-1),
};

enum class AlgorithmMode : bool {
	FAST, QUALITY
};

struct AlgorithmModes {
	AlgorithmMode compositing;
	AlgorithmMode pixeloffset;
	AlgorithmMode smoothing;
	AlgorithmMode interpolation;
};

struct ImageCLSID {
private:
	static constexpr mqguid BMP_E{ 0x557cf400, 0x1a04, 0x11d3, { 0x9a, 0x73, 0x00, 0x00, 0xf8, 0x1e, 0xf3, 0x2e } };
	static constexpr mqguid JPG_E{ 0x557cf401, 0x1a04, 0x11d3, { 0x9a, 0x73, 0x00, 0x00, 0xf8, 0x1e, 0xf3, 0x2e } };
	static constexpr mqguid GIF_E{ 0x557cf402, 0x1a04, 0x11d3, { 0x9a, 0x73, 0x00, 0x00, 0xf8, 0x1e, 0xf3, 0x2e } };
	static constexpr mqguid TIFF_E{ 0x557cf405, 0x1a04, 0x11d3, { 0x9a, 0x73, 0x00, 0x00, 0xf8, 0x1e, 0xf3, 0x2e } };
	static constexpr mqguid PNG_E{ 0x557cf406, 0x1a04, 0x11d3, { 0x9a, 0x73, 0x00, 0x00, 0xf8, 0x1e, 0xf3, 0x2e } };

	static constexpr auto FormatCountMaxValue{ 7U };
	static constexpr mqguid _Encoders[FormatCountMaxValue] = { BMP_E, JPG_E, GIF_E, { }, { }, TIFF_E, PNG_E };

public:
	ImageCLSID() = delete;
	[[nodiscard]] static constexpr bool _BuiltinFormat(ImageFormat ifmt) noexcept {
		return static_cast<mqui32>(ifmt) < FormatCountMaxValue;
	}

	[[nodiscard]] static constexpr const mqguid& _GetEncoder(ImageFormat ifmt) noexcept {
		return _Encoders[static_cast<mqui32>(ifmt)];
	}
};

static void SetGraphicsMode(Gdiplus::Graphics& graphics, AlgorithmModes mode) {
	graphics.SetCompositingMode(mode.compositing == AlgorithmMode::FAST ?
		Gdiplus::CompositingMode::CompositingModeSourceCopy :
		Gdiplus::CompositingMode::CompositingModeSourceOver);
	graphics.SetCompositingQuality(mode.compositing == AlgorithmMode::FAST ?
		Gdiplus::CompositingQuality::CompositingQualityHighSpeed :
		Gdiplus::CompositingQuality::CompositingQualityHighQuality);
	graphics.SetPixelOffsetMode(mode.pixeloffset == AlgorithmMode::FAST ?
		Gdiplus::PixelOffsetMode::PixelOffsetModeHighSpeed :
		Gdiplus::PixelOffsetMode::PixelOffsetModeHighQuality);
	graphics.SetSmoothingMode(mode.smoothing == AlgorithmMode::FAST ?
		Gdiplus::SmoothingMode::SmoothingModeHighSpeed :
		Gdiplus::SmoothingMode::SmoothingModeHighQuality);
	graphics.SetInterpolationMode(mode.interpolation == AlgorithmMode::FAST ?
		Gdiplus::InterpolationMode::InterpolationModeLowQuality :
		Gdiplus::InterpolationMode::InterpolationModeHighQualityBicubic);
	graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);
}

static Gdiplus::Bitmap* CreateEmptyBitmap() {
	return new Gdiplus::Bitmap{ 0, 0 };
}

template<typename T1, typename T2>
requires (sizeof(T1) == sizeof(T2))
inline T1 cast(T2 const& t2) noexcept {
	T1 t1{ };
	freestanding::copy(&t1, &t2, sizeof(T1));
	return t1;
}

template<>
inline Gdiplus::PointF cast<Gdiplus::PointF, mqpoint>(mqpoint const& t2) noexcept {
	return { static_cast<mqf32>(t2.x), static_cast<mqf32>(t2.y) };
}

META_EXPORT_API(void, StartupGDI) {
	Gdiplus::GdiplusStartupInput StartUpInput;
	Gdiplus::GdiplusStartup(&GDIHANDLE, &StartUpInput, nullptr);
}

META_EXPORT_API(void, ShutdownGDI) {
	Gdiplus::GdiplusShutdown(GDIHANDLE);
}

META_EXPORT_API(Gdiplus::Bitmap*, CreateImageFromSize, mqsize imageSize, Media::Color color) {
	auto image{ new Gdiplus::Bitmap(
		static_cast<mqi32>(imageSize.width), static_cast<mqi32>(imageSize.height)) };
	image->SetResolution(72.0f, 72.0f);
	Gdiplus::Graphics graphics{ image };
	Gdiplus::SolidBrush brush{ Gdiplus::Color{ color } };
	graphics.FillRectangle(&brush, 0, 0, static_cast<mqi32>(imageSize.width), static_cast<mqi32>(imageSize.height));
	return image;
}

META_EXPORT_API(Gdiplus::Bitmap*, CreateImageFromMemory, mqcbytes mem, mqui32 size) {
	IStream* stream{ };
	if (HGLOBAL hGlobal{ GlobalAlloc(GMEM_MOVEABLE, size) }) {
		if (auto hMem{ GlobalLock(hGlobal) }) {
			memcpy(hMem, mem, size);
			(void)CreateStreamOnHGlobal(hGlobal, true, &stream);
		}
	}
	auto image{ new Gdiplus::Bitmap(stream) };
	stream->Release();
	return image;
}

META_EXPORT_API(Gdiplus::Bitmap*, CreateImageFromFile, mqcstr fn) {
	Gdiplus::Bitmap* image{ };
	if (auto hFile{ CreateFileW(fn, GENERIC_READ, FILE_SHARE_READ,
		nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
		nullptr) }; hFile != INVALID_HANDLE_VALUE) {
		if (auto hFileMap{ CreateFileMappingW(hFile, nullptr,
			PAGE_READONLY, 0, 0, nullptr) }) {
			if (auto data{ MapViewOfFile(hFileMap, FILE_MAP_READ,
				0, 0, 0) }) {
				LARGE_INTEGER size{ };
				GetFileSizeEx(hFile, &size);
				image = MasterQian_Media_GDI_CreateImageFromMemory(static_cast<mqcbytes>(data), size.LowPart);
				UnmapViewOfFile(data);
			}
			CloseHandle(hFileMap);
		}
		CloseHandle(hFile);
	}
	if (image == nullptr) image = CreateEmptyBitmap();
	return image;
}

META_EXPORT_API(Gdiplus::Bitmap*, CreateImageFromFileIcon, mqcstr fn, mqui32 index) {
	auto hIcon{ ExtractIconW(nullptr, fn, index) };
	auto image{ new Gdiplus::Bitmap(hIcon) };
	DestroyIcon(hIcon);
	return image;
}

META_EXPORT_API(Gdiplus::Bitmap*, CreateImageFromClone, Gdiplus::Bitmap* image) {
	auto clone{ image->Clone(0, 0, static_cast<mqi32>(image->GetWidth()), static_cast<mqi32>(image->GetHeight()), PixelFormatDontCare) };
	if (clone == nullptr) return CreateEmptyBitmap();
	clone->SetResolution(image->GetHorizontalResolution(), image->GetVerticalResolution());
	return clone;
}

META_EXPORT_API(void, DeleteImage, Gdiplus::Bitmap* image) {
	delete image;
}

META_EXPORT_API(BOOL, OK, Gdiplus::Bitmap* image) {
	return image->GetLastStatus() == Gdiplus::Status::Ok;
}

META_EXPORT_API(BOOL, SaveToFile, Gdiplus::Bitmap* image, mqcstr fn, ImageFormat ifmt) {
	if (ImageCLSID::_BuiltinFormat(ifmt)) {
		LONG lQuality{ 100 };
		Gdiplus::EncoderParameters paras = { 1U, { Gdiplus::EncoderQuality, 1, Gdiplus::EncoderParameterValueTypeLong, &lQuality } };
		return image->Save(fn, reinterpret_cast<CLSID const*>(&ImageCLSID::_GetEncoder(ifmt)), &paras);
	}
	return false;
}

META_EXPORT_API(IStream*, SaveToStream, Gdiplus::Bitmap* image, ImageFormat ifmt, mqui32* pSize) {
	IStream* stream{ };
	*pSize = 0U;
	if (ImageCLSID::_BuiltinFormat(ifmt)) {
		LONG lQuality{ 100 };
		Gdiplus::EncoderParameters paras{ 1U, { Gdiplus::EncoderQuality, 1, Gdiplus::EncoderParameterValueTypeLong, &lQuality } };
		(void)CreateStreamOnHGlobal(nullptr, true, &stream);
		image->Save(stream, reinterpret_cast<CLSID const*>(&ImageCLSID::_GetEncoder(ifmt)), &paras);
		ULARGE_INTEGER size{ };
		stream->Seek({ }, STREAM_SEEK_END, &size);
		if (size.QuadPart) {
			stream->Seek({ }, STREAM_SEEK_SET, nullptr);
			*pSize = size.LowPart;
		}
		else {
			stream->Release();
			stream = nullptr;
		}
	}
	return stream;
}

META_EXPORT_API(void, StreamReadRelease, IStream* stream, mqbytes mem, mqui32 size) {
	stream->Read(mem, size, nullptr);
	stream->Release();
}

META_EXPORT_API(mqsize, Size, Gdiplus::Bitmap* image) {
	return { image->GetWidth(), image->GetHeight() };
}

META_EXPORT_API(mqsize, GetDPI, Gdiplus::Bitmap* image) {
	return { static_cast<mqui32>(image->GetHorizontalResolution()), static_cast<mqui32>(image->GetVerticalResolution()) };
}

META_EXPORT_API(void, SetDPI, Gdiplus::Bitmap* image, mqsize dpi) {
	image->SetResolution(static_cast<Gdiplus::REAL>(dpi.width), static_cast<Gdiplus::REAL>(dpi.height));
}

META_EXPORT_API(mqui32, BitDepth, Gdiplus::Bitmap* image) {
	return static_cast<mqui32>((image->GetPixelFormat() >> 8) & 0xFF);
}

META_EXPORT_API(Gdiplus::Bitmap*, Thumbnail, Gdiplus::Bitmap* image, mqsize imageSize) {
	auto thumbnail{ static_cast<Gdiplus::Bitmap*>(image->GetThumbnailImage(
		imageSize.width, imageSize.height, nullptr, nullptr)) };
	if (thumbnail == nullptr) return CreateEmptyBitmap();
	thumbnail->SetResolution(image->GetHorizontalResolution(), image->GetVerticalResolution());
	return thumbnail;
}

META_EXPORT_API(Gdiplus::Bitmap*, Crop, Gdiplus::Bitmap* image, mqrect rect, AlgorithmModes mode) {
	auto cropImage{ new Gdiplus::Bitmap(rect.width, rect.height) };
	cropImage->SetResolution(image->GetHorizontalResolution(), image->GetVerticalResolution());
	Gdiplus::Graphics graphics{ cropImage };
	SetGraphicsMode(graphics, mode);
	graphics.DrawImage(image, 0, 0, rect.left, rect.top, rect.width, rect.height, Gdiplus::UnitPixel);
	return cropImage;
}

META_EXPORT_API(Gdiplus::Bitmap*, Border, Gdiplus::Bitmap* image, mqrange pos, Media::Color color, AlgorithmModes mode) {
	auto width{ image->GetWidth() }, height{ image->GetHeight() };
	auto new_width{ width + pos.left + pos.right }, new_height{ height + pos.top + pos.bottom };
	auto borderImage{ new Gdiplus::Bitmap(new_width, new_height) };
	borderImage->SetResolution(image->GetHorizontalResolution(), image->GetVerticalResolution());
	Gdiplus::Graphics graphics{ borderImage };
	Gdiplus::SolidBrush brush{ Gdiplus::Color{ color } };
	Gdiplus::Rect rects[]{
		{ 0, 0, static_cast<mqi32>(new_width), static_cast<mqi32>(pos.top) },
		{ 0, static_cast<mqi32>(height + pos.top), static_cast<mqi32>(new_width), static_cast<mqi32>(pos.bottom) },
		{ 0, static_cast<mqi32>(pos.top), static_cast<mqi32>(pos.left), static_cast<mqi32>(height) },
		{ static_cast<mqi32>(width + pos.left), static_cast<mqi32>(pos.top), static_cast<mqi32>(pos.right), static_cast<mqi32>(height) }
	};
	SetGraphicsMode(graphics, mode);
	graphics.FillRectangles(&brush, rects, 4);
	graphics.DrawImage(image, static_cast<mqi32>(pos.left), static_cast<mqi32>(pos.top));
	return borderImage;
}

META_EXPORT_API(Gdiplus::Bitmap*, Rotate, Gdiplus::Bitmap* image, mqf64 angle, Media::Color color, AlgorithmModes mode) {
	mqi32 old_width{ static_cast<mqi32>(image->GetWidth()) };
	mqi32 old_height{ static_cast<mqi32>(image->GetHeight()) };

	mqf32 angle_f{ static_cast<mqf32>(angle - static_cast<mqi32>(angle) + static_cast<mqi32>(angle) % 360) };
	if (angle_f < 0.0f) angle_f = angle_f + 360.0f;
	mqf32 angle_r{ angle_f * 3.1415926f / 180.0f };
	mqf32 cos_angle_r{ cosf(angle_f) }, sin_angle_f{ sinf(angle_f) };

	mqf32 dx1{ old_width * cos_angle_r }, dx2{ old_height * sin_angle_f };
	mqf32 dy1{ old_height * cos_angle_r }, dy2{ old_width * sin_angle_f };
	if (dx1 < 0.0f) dx1 = -dx1;
	if (dx2 < 0.0f) dx2 = -dx2;
	if (dy1 < 0.0f) dy1 = -dy1;
	if (dy2 < 0.0f) dy2 = -dy2;
	mqi32 new_width{ static_cast<mqi32>(dx1 + dx2) }, new_height{ static_cast<mqi32>(dy1 + dy2) };

	mqf32 tL{ }, tT{ };
	if (0.0f <= angle_f && angle_f <= 90.0f) {
		tL = dx2;
		tT = 0.0f;
	}
	else if (90.0f <= angle_f && angle_f <= 180.0f) {
		tL = static_cast<mqf32>(new_width);
		tT = dy1;
	}
	else if (180.0f <= angle_f && angle_f <= 270.0f) {
		tL = dx1;
		tT = static_cast<mqf32>(new_height);
	}
	else if (270.0f <= angle_f && angle_f <= 360.0f) {
		tL = 0.0f;
		tT = dy2;
	}

	auto rotateImage{ new Gdiplus::Bitmap{ new_width, new_height } };
	rotateImage->SetResolution(image->GetHorizontalResolution(), image->GetVerticalResolution());
	Gdiplus::Graphics graphics(rotateImage);
	SetGraphicsMode(graphics, mode);
	Gdiplus::SolidBrush brush{ Gdiplus::Color{ color } };
	graphics.FillRectangle(&brush, 0, 0, new_width, new_height);

	Gdiplus::Matrix matrix;
	matrix.Rotate(angle_f);
	graphics.SetTransform(&matrix);
	graphics.TranslateTransform(tL, tT, Gdiplus::MatrixOrderAppend);
	graphics.DrawImage(image, 0, 0);
	return rotateImage;
}

META_EXPORT_API(void, RotateLeft, Gdiplus::Bitmap* image) {
	image->RotateFlip(Gdiplus::Rotate270FlipNone);
}

META_EXPORT_API(void, RotateRight, Gdiplus::Bitmap* image) {
	image->RotateFlip(Gdiplus::Rotate90FlipNone);
}

META_EXPORT_API(void, FlipX, Gdiplus::Bitmap* image) {
	image->RotateFlip(Gdiplus::RotateNoneFlipX);
}

META_EXPORT_API(void, FlipY, Gdiplus::Bitmap* image) {
	image->RotateFlip(Gdiplus::RotateNoneFlipY);
}

META_EXPORT_API(void, GrayScale, Gdiplus::Bitmap* image, AlgorithmModes mode) {
	Gdiplus::ColorMatrix matrix{
		0.299f, 0.299f, 0.299f, 0.000f, 0.000f,
		0.587f, 0.587f, 0.587f, 0.000f, 0.000f,
		0.114f, 0.114f, 0.114f, 0.000f, 0.000f,
		0.000f, 0.000f, 0.000f, 1.000f, 0.000f,
		0.000f, 0.000f, 0.000f, 0.000f, 1.000f
	};
	auto width{ static_cast<mqi32>(image->GetWidth()) }, height{ static_cast<mqi32>(image->GetHeight()) };
	Gdiplus::ImageAttributes attr;
	attr.SetColorMatrix(&matrix, Gdiplus::ColorMatrixFlagsDefault, Gdiplus::ColorAdjustTypeBitmap);
	Gdiplus::Graphics graphics(image);
	SetGraphicsMode(graphics, mode);
	graphics.DrawImage(image, { 0, 0, width, height }, 0, 0, width, height, Gdiplus::UnitPixel, &attr);
}

META_EXPORT_API(Gdiplus::Bitmap*, Resample, Gdiplus::Bitmap* image, mqsize size, AlgorithmModes mode) {
	auto width{ static_cast<mqi32>(size.width) }, height{ static_cast<mqi32>(size.height) };
	auto resampleImage{ new Gdiplus::Bitmap{ width, height } };
	resampleImage->SetResolution(image->GetHorizontalResolution(), image->GetVerticalResolution());
	Gdiplus::Graphics graphics(resampleImage);
	SetGraphicsMode(graphics, mode);
	graphics.DrawImage(image, 0, 0, width, height);
	return resampleImage;
}

META_EXPORT_API(Gdiplus::Bitmap*, Combine, Gdiplus::Bitmap* image1, Gdiplus::Bitmap* image2, mqrect rect1, mqrect rect2, BOOL isHorizontal, AlgorithmModes mode) {
	mqi32 new_width{ }, new_height{ };
	if (isHorizontal) {
		new_width = static_cast<mqi32>(rect1.width + rect2.width);
		new_height = rect1.height > rect2.height ? rect1.height : rect2.height;
	}
	else {
		new_width = rect1.width > rect2.width ? rect1.width : rect2.width;
		new_height = static_cast<mqi32>(rect1.height + rect2.height);
	}
	auto combineImage{ new Gdiplus::Bitmap{ new_width, new_height } };
	combineImage->SetResolution(image1->GetHorizontalResolution(), image1->GetVerticalResolution());
	Gdiplus::Graphics graphics(combineImage);
	SetGraphicsMode(graphics, mode);
	graphics.DrawImage(image1, 0, 0, static_cast<mqi32>(rect1.left),
		static_cast<mqi32>(rect1.top), static_cast<mqi32>(rect1.width),
		static_cast<mqi32>(rect1.height), Gdiplus::UnitPixel);
	if (isHorizontal) {
		graphics.DrawImage(image2, static_cast<mqi32>(rect1.width), 0, static_cast<mqi32>(rect2.left),
			static_cast<mqi32>(rect2.top), static_cast<mqi32>(rect2.width),
			static_cast<mqi32>(rect2.height), Gdiplus::UnitPixel);
	}
	else {
		graphics.DrawImage(image2, 0, static_cast<mqi32>(rect1.height), static_cast<mqi32>(rect2.left),
			static_cast<mqi32>(rect2.top), static_cast<mqi32>(rect2.width),
			static_cast<mqi32>(rect2.height), Gdiplus::UnitPixel);
	}
	return combineImage;
}

META_EXPORT_API(void, DrawLine, Gdiplus::Bitmap* image, mqrect rect, Media::Color color, mqf64 thickness, AlgorithmModes mode) {
	Gdiplus::Graphics graphics(image);
	Gdiplus::Pen pen{ Gdiplus::Color{ color }, static_cast<mqf32>(thickness) };
	SetGraphicsMode(graphics, mode);
	graphics.DrawLine(&pen, static_cast<mqi32>(rect.left), static_cast<mqi32>(rect.top),
		static_cast<mqi32>(rect.left + rect.width),
		static_cast<mqi32>(rect.top + rect.height));
}

META_EXPORT_API(void, DrawRectangle, Gdiplus::Bitmap* image, mqrect rect, Media::Color color, mqf64 thickness, AlgorithmModes mode) {
	Gdiplus::Graphics graphics(image);
	Gdiplus::Pen pen{ Gdiplus::Color{ color }, static_cast<mqf32>(thickness) };
	SetGraphicsMode(graphics, mode);
	graphics.DrawRectangle(&pen, cast<Gdiplus::Rect>(rect));
}

META_EXPORT_API(void, DrawRectangles, Gdiplus::Bitmap* image, mqrect const* rects, mqui32 count, Media::Color color, mqf64 thickness, AlgorithmModes mode) {
	Gdiplus::Graphics graphics(image);
	Gdiplus::Pen pen{ Gdiplus::Color{ color }, static_cast<mqf32>(thickness) };
	SetGraphicsMode(graphics, mode);
	graphics.DrawRectangles(&pen, cast<Gdiplus::Rect const*>(rects), count);
}

META_EXPORT_API(void, DrawCircle, Gdiplus::Bitmap* image, mqpoint point, mqsize r, Media::Color color, mqf64 thickness, AlgorithmModes mode) {
	Gdiplus::Graphics graphics(image);
	Gdiplus::Pen pen{ Gdiplus::Color{ color }, static_cast<mqf32>(thickness) };
	SetGraphicsMode(graphics, mode);
	graphics.DrawEllipse(&pen, static_cast<mqf32>(point.x - r.width), static_cast<mqf32>(point.y - r.height),
		static_cast<mqf32>(r.width * 2), static_cast<mqf32>(r.height * 2));
}

META_EXPORT_API(void, DrawString, Gdiplus::Bitmap* image, mqpoint point, mqcstr content, mqui32 size, Media::Color color, mqcstr fontname, AlgorithmModes mode) {
	Gdiplus::Graphics graphics(image);
	Gdiplus::SolidBrush brush{ Gdiplus::Color{ color } };
	Gdiplus::Font font{ fontname, static_cast<mqf32>(size) };
	Gdiplus::StringFormat format;
	format.SetAlignment(Gdiplus::StringAlignmentCenter);
	format.SetLineAlignment(Gdiplus::StringAlignmentCenter);
	SetGraphicsMode(graphics, mode);
	graphics.DrawString(content, -1, &font, cast<Gdiplus::PointF>(point), &format, &brush);
}

META_EXPORT_API(void, DrawImage, Gdiplus::Bitmap* image1, Gdiplus::Bitmap* image2, mqpoint point, mqsize size, AlgorithmModes mode) {
	Gdiplus::Graphics graphics(image1);
	SetGraphicsMode(graphics, mode);
	if (size == mqsize{ }) {
		graphics.DrawImage(image2, cast<Gdiplus::Point>(point));
	}
	else {
		graphics.DrawImage(image2, Gdiplus::Rect{ static_cast<mqi32>(point.x), static_cast<mqi32>(point.y),
			static_cast<mqi32>(size.width), static_cast<mqi32>(size.height) });
	}
}

META_EXPORT_API(void, FillRectangle, Gdiplus::Bitmap* image, mqrect rect, Media::Color color, AlgorithmModes mode) {
	Gdiplus::Graphics graphics(image);
	Gdiplus::SolidBrush brush{ Gdiplus::Color{ color } };
	SetGraphicsMode(graphics, mode);
	graphics.FillRectangle(&brush, cast<Gdiplus::Rect>(rect));
}

META_EXPORT_API(void, FillRectangles, Gdiplus::Bitmap* image, mqrect const* rects, mqui32 count, Media::Color color, AlgorithmModes mode) {
	Gdiplus::Graphics graphics(image);
	Gdiplus::SolidBrush brush{ Gdiplus::Color{ color } };
	SetGraphicsMode(graphics, mode);
	graphics.FillRectangles(&brush, cast<Gdiplus::Rect const*>(rects), count);
}

META_EXPORT_API(void, FillCircle, Gdiplus::Bitmap* image, mqpoint point, mqsize r, Media::Color color, AlgorithmModes mode) {
	Gdiplus::Graphics graphics(image);
	Gdiplus::SolidBrush brush{ Gdiplus::Color{ color } };
	SetGraphicsMode(graphics, mode);
	graphics.FillEllipse(&brush, static_cast<mqf32>(point.x - r.width), static_cast<mqf32>(point.y - r.height),
		static_cast<mqf32>(r.width * 2), static_cast<mqf32>(r.height * 2));
}

META_EXPORT_API(HDC, StartPrinterFromName, mqcstr name) {
	HDC hdcPrint{ CreateDCW(nullptr, name, nullptr, nullptr) };
	if (hdcPrint) {
		DOCINFO docInfo{ };
		docInfo.cbSize = sizeof(docInfo);
		docInfo.lpszDocName = L"GDIPrint";
		StartDocW(hdcPrint, &docInfo);
		StartPage(hdcPrint);
	}
	return hdcPrint;
}

META_EXPORT_API(void, EndPrinter, HDC hdcPrint) {
	if (hdcPrint) {
		EndPage(hdcPrint);
		EndDoc(hdcPrint);
		DeleteDC(hdcPrint);
	}
}

META_EXPORT_API(void, DrawImageToPrinter, HDC hdcPrint, Gdiplus::Bitmap* image, AlgorithmModes mode) {
	Gdiplus::Graphics graphics(hdcPrint);
	SetGraphicsMode(graphics, mode);
	graphics.DrawImage(image, 0, 0);
}