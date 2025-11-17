#include "reportBug.h"

#include "teaForGod.h"
#include "game\game.h"

#include "..\framework\game\player.h"
#include "..\framework\jobSystem\jobSystem.h"
#include "..\framework\object\actor.h"

#include "..\core\buildInformation.h"
#include "..\core\compression\compression.h"
#include "..\core\debug\debug.h"
#include "..\core\globalSettings.h"
#include "..\core\io\memoryStorage.h"
#include "..\core\math\math.h"
#include "..\core\system\core.h"
#include "..\core\system\faultHandler.h"
#include "..\core\system\recentCapture.h"
#include "..\core\system\video\pixelUtils.h"
#include "..\core\system\video\imageSavers\is_tga.h"

//

#ifdef AN_ALLOW_EXTENSIVE_LOGS
//#define OUTPUT_REPORT_BUG_INFO
#endif

//

using namespace TeaForGodEmperor;

//

#ifdef AN_WINDOWS
#include <windef.h>
#include <Windows.h>
#include "CommCtrl.h"

struct TeaForGodEmperor::ReportBugImpl
{
public:
	ReportBugImpl()
	{		
		an_assert(!impl);
		impl = this;

		register_class();

		create_window();
	}

	~ReportBugImpl()
	{
		if (window)
		{
			DestroyWindow(window);
		}

		unregister_class();

		an_assert(impl);
		impl = nullptr;
	}

private:
	static ReportBugImpl* impl;
	const tchar * const windowClassName = TXT("Report bug");

	HWND window = nullptr;
	bool classRegistered = false;
	VectorInt2 windowSize;

	void register_class()
	{
		WNDCLASS splashClass;

		splashClass.style = CS_HREDRAW | CS_VREDRAW | CS_NOCLOSE;
		splashClass.lpfnWndProc = window_func;
		splashClass.cbClsExtra = 0;
		splashClass.cbWndExtra = 0;
		splashClass.hInstance = nullptr; // instance
		splashClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		splashClass.hCursor = LoadCursor(NULL, IDC_ARROW);
		splashClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		splashClass.lpszMenuName = NULL;
		splashClass.lpszClassName = windowClassName;

		if (RegisterClass(&splashClass))
		{
			classRegistered = true;
		}
	}

	void unregister_class()
	{
		if (classRegistered)
		{
			UnregisterClass(windowClassName, nullptr);
		}
	}

	void create_window()
	{
		if (!classRegistered)
		{
			return;
		}

		windowSize.x = 700;
		windowSize.y = 400;

		int left = (GetSystemMetrics(SM_CXSCREEN) - windowSize.x) / 2;
		int top = (GetSystemMetrics(SM_CYSCREEN) - windowSize.y) / 2;

		window = CreateWindowEx(WS_EX_NOACTIVATE, windowClassName, TXT("report bug"), WS_POPUP | WS_BORDER, left, top, windowSize.x, windowSize.y, nullptr, nullptr, nullptr, nullptr);

		RECT windowRect;
		GetClientRect(window, &windowRect);

		int newWidth = windowSize.x +(windowSize.x - windowRect.right);
		int newHeight = windowSize.y +(windowSize.y - windowRect.bottom);
		SetWindowPos(window, HWND_TOP, left, top, newWidth, newHeight, 0); // show it on top

		ShowWindow(window, SW_SHOW);
		UpdateWindow(window);
		RedrawWindow(window, nullptr, nullptr, 0);
	}

	static void draw_text(HDC _hdc, tchar const * _text, int _x, int _y, DWORD _textColor = RGB(0, 0, 0))
	{
		SetBkMode(_hdc, TRANSPARENT);
		RECT rect;
		rect.left = _x - 10000;
		rect.right = _x + 10000;
		rect.bottom = _y + 50;
		rect.top = _y - 50;
		SetTextColor(_hdc, _textColor);
		DrawText(_hdc, _text, -1, &rect, DT_CENTER | DT_VCENTER);
	}

	static LRESULT CALLBACK window_func(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
		case WM_PAINT:
		{
			HDC hdc;
			PAINTSTRUCT ps;
			RECT rt;
			GetWindowRect(hwnd, &rt);
			hdc = BeginPaint(hwnd, &ps);

			RECT rtLocal = rt;
			rtLocal.right = rtLocal.left;
			rtLocal.top = 0;
			rtLocal.left = 0;
			rtLocal.bottom -= rtLocal.top;
			FillRect(hdc, &rtLocal, (HBRUSH)(COLOR_WINDOW + 1));

			draw_text(hdc, TXT("Sending a report.\n\nPlease wait..."), impl->windowSize.x / 2, impl->windowSize.y / 2);

			EndPaint(hwnd, &ps);
			return 0;
		}

		break;

		case WM_CLOSE:
			return 0;
		}
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
};

ReportBugImpl* ReportBugImpl::impl = nullptr;

#else
#ifdef AN_ANDROID
struct TeaForGodEmperor::ReportBugImpl
{
	int temp; // nothing, is not used
};
#endif
#endif

//

bool ReportBug::send_quick_note_in_background(String const& _text, String const& _additionalText)
{
	return send_in_background(_text, _additionalText, String::empty(), TXT("quick-note"), TXT("QuickNotes"), TXT("quickNote"));
}

bool ReportBug::send_full_log_info_in_background(String const& _text, String const& _additionalText)
{
	String logName = copy_log(TXT("info"), true);
	return send_in_background(_text, _additionalText, logName, TXT("full-info"), TXT("FullInfos"), TXT("fullInfo"));
}

bool ReportBug::send_in_background(String const& _text, String const & _additionalText, String const & _fileName, tchar const* _reason, tchar const* _category, tchar const* _logNameInfo)
{
	scoped_call_stack_info(TXT("send_quick_note_in_background"));

	String summary = String::printf(TXT("%S (%S), [%S] %S"), get_build_version(), ::System::Core::get_system_precise_name().to_char(), _reason, _text.get_left(60).to_char());
	String info = _text;
	info += TXT("\n");
	info += TXT("\n");
	if (!_additionalText.is_empty())
	{
		info += _additionalText;
		info += TXT("\n");
		info += TXT("\n");
	}
	if (auto* piow = TeaForGodEmperor::PilgrimageInstanceOpenWorld::get())
	{
		info += TXT("[QUICK NOTE] sent from location:");
		if (auto* game = Game::get_as<Game>())
		{
			List<String> list = piow->build_dev_info(game->access_player().get_actor());
			for_every(line, list)
			{
				info += *line + TXT("\n");
			}
		}
	}

	{
		info += TXT("\n");
		info += ::System::FaultHandler::get_custom_info();
		info += TXT("\n");
	}

	{
		info += TXT("\n");
		info += get_call_stack_info();
		info += TXT("\n");
	}

	struct SendReportData
	{
		String summary;
		String info;
		String fileName;
		tchar const* reason;
		tchar const* category;
		tchar const* logNameInfo;

		SendReportData() {}
		SendReportData(String const& _summary, String const& _info, String const & _fileName, tchar const* _reason, tchar const* _category, tchar const* _logNameInfo) : summary(_summary), info(_info), fileName(_fileName), reason(_reason), category(_category), logNameInfo(_logNameInfo) {}

		static void send(void* _data)
		{
			SendReportData* srData = (SendReportData*)_data;
			String summary = srData->summary;
			String info = srData->info;
			String fileName = srData->fileName;
			tchar const* reason = srData->reason;
			tchar const* category = srData->category;
			tchar const* logNameInfo = srData->logNameInfo;

			TeaForGodEmperor::ReportBug::send(summary, info, fileName, reason, category, logNameInfo);

			delete srData;
		}
	};

	if (auto* game = Game::get())
	{
		game->get_job_system()->do_asynchronous_off_system_job(SendReportData::send, new SendReportData(summary, info, _fileName, _reason, _category, _logNameInfo));
		return true;
	}
	else
	{
		return false;
	}
}

bool ReportBug::send(String const& _summary, String const& _info, String const& _filename, tchar const* _reason, tchar const* _category, tchar const * _logNameInfo, std::function<void(String const & _info)> _update_info)
{
	ReportBug reportBug;
	return reportBug.send_report(_summary, _info, _filename, _reason, _category, _logNameInfo, _update_info);
}

ReportBug::ReportBug()
{
	impl = new ReportBugImpl();
}

ReportBug::~ReportBug()
{
	delete_and_clear(impl);
}

#include "..\core\buildInformation.h"
#include "..\core\io\base64.h"
#include "..\core\io\file.h"
#include "..\framework\rest\rest.h"
#include "..\core\io\json.h"

bool ReportBug::send_report(String const& _summary, String const& _info, String const& _filename, tchar const * _reason, tchar const * _category, tchar const * _logNameInfo, std::function<void(String const& _info)> _update_info)
{
	String logNameSuffix;
	if (_logNameInfo)
	{
		logNameSuffix = String::printf(TXT("_%S"), _logNameInfo);
	}
	// mantis
	typedef IO::JSON::Document JSONDoc;

#ifdef OUTPUT_REPORT_BUG_INFO
	output(TXT("create json message"));
#endif
	
	if (_update_info)
	{
		_update_info(String(TXT("sending report header")));
	}

	RefCountObjectPtr<JSONDoc> msgCreateIssue(new JSONDoc());
	msgCreateIssue->set(TXT("summary"), _summary);
	msgCreateIssue->set(TXT("description"), String::printf(TXT("%S\nversion: %S\nbuild date: %S\nplatform: %S\nlog: %S\ninfo:\n%S"),
		_reason,
		get_build_version(),
		get_build_date(),
		::System::Core::get_system_precise_name().to_char(),
		IO::Utils::get_file(_filename).to_char(), _info.to_char()));
	msgCreateIssue->access_sub(TXT("category")).set(TXT("name"), _category);
	msgCreateIssue->access_sub(TXT("project")).set(TXT("name"), TXT("Tea For God"));

#ifdef OUTPUT_REPORT_BUG_INFO
	output(TXT("send a report"));
#endif

	RefCountObjectPtr<JSONDoc> response = Framework::REST::send(TXT("http://bugs.voidroom.com/api"), TXT("/rest/issues/"), msgCreateIssue.get());

	bool sent = false;
	if (response.is_set())
	{
		if (_update_info)
		{
			_update_info(String(TXT("processing response")));
		}

#ifdef OUTPUT_REPORT_BUG_INFO
		output(TXT("response received"));
		output(TXT("response: %S"), response->to_string().to_char());
#endif

		Optional<int> issueId;
		if (auto * sub = response->get_sub(TXT("issue")))
		{
			issueId = sub->get_as_int(TXT("id"));
#ifdef OUTPUT_REPORT_BUG_INFO
			output(TXT("issue id: %i"), issueId.get());
#endif
		}
		sent = issueId.is_set(); // this means that the issue has been created

		String fullFilenamePath = IO::get_user_game_data_directory() + _filename;

#ifdef OUTPUT_REPORT_BUG_INFO
		output(TXT("file \"%S\", full path \"%S\" %S"), _filename.to_char(), fullFilenamePath.to_char(), IO::File::does_exist(fullFilenamePath)? TXT("file exists") : TXT("file is missing"));
#endif

		if (issueId.is_set() && !_filename.is_empty() && IO::File::does_exist(fullFilenamePath))
		{
			if (_update_info)
			{
				_update_info(String(TXT("processing log data")));
			}

#ifdef OUTPUT_REPORT_BUG_INFO
			output(TXT("file \"%S\" present (%S)"), _filename.to_char(), fullFilenamePath.to_char());
#endif

			// always break as otherwise we may have a buffer overrun or an incomplete buffer
			int contentSize = IO::File::get_size_of(fullFilenamePath);

			int sendFirst = 40 * 8192;
			int sendLast = 80 * 8192;

			if (sendLast + sendFirst >= contentSize)
			{
				sendLast = contentSize;
				sendFirst = 0;
			}

#ifdef OUTPUT_REPORT_BUG_INFO
			output(TXT("content size %i, send first %i, send last %i"), contentSize, sendFirst, sendLast);
#endif

			// these will be deleted at the end via parts clearing
			IO::MemoryStorage* memFirst = new IO::MemoryStorage();
			IO::MemoryStorage* memLast = new IO::MemoryStorage();

			{
				IO::File* file = new IO::File();
				file->open(fullFilenamePath);
				file->set_type(IO::InterfaceType::Text);

				Array<int8> buffer;
				int bufferSize = 4096;
				buffer.set_size(bufferSize);
				if (sendFirst > 0)
				{
					int left = sendFirst;
					file->go_to((uint)0);
					while (left > 0)
					{
						int read = file->read(buffer.get_data(), min(left, bufferSize));
						left -= read;
						if (read == 0)
						{
							break;
						}
						memFirst->write(buffer.get_data(), read);
					}
				}
				// read all, to the end
				if (sendLast > 0)
				{
					file->go_to(contentSize - sendLast);
					while (true)
					{
						int read = file->read(buffer.get_data(), bufferSize);
						if (read == 0)
						{
							break;
						}
						memLast->write(buffer.get_data(), read);
					}
				}

				delete file;
			}

#ifdef OUTPUT_REPORT_BUG_INFO
			output(TXT("loaded into memory storage"));
#endif
			struct Part
			{
				int idx;
				bool image = false;
				String name;
				String specialName;
				IO::MemoryStorage* data;
				Part() {}
				Part(int _idx, String const& _name, IO::MemoryStorage* _data) : idx(_idx), name(_name), data(_data) {}
				Part(int _idx, bool _image, IO::MemoryStorage* _data) : idx(_idx), image(_image), data(_data) {}
				Part(String const &_specialName, bool _image, IO::MemoryStorage* _data) : image(_image), specialName(_specialName), data(_data) {}
			};
			Array<Part> parts;
			if (sendLast > 0)
			{
				parts.push_back(Part(1, String(TXT("end")), memLast));
			}
			else
			{
				delete memLast;
			}
			if (sendFirst > 0)
			{
				parts.push_back(Part(0, String(TXT("begin")), memFirst));
			}
			else
			{
				delete memFirst;
			}

			bool requiresParts = parts.get_size() > 1;

			// screen shots from recent capture and drawing board
			{
				VectorInt2 resolution;
				Array<byte> upsideDownPixels;
				int bytesPerPixel;
				bool convertSRGB;
				for_count(int, i, System::RecentCapture::MAX)
				{
					if (System::RecentCapture::read_pixel_data((System::RecentCapture::RecentCaptureIndex)i, resolution, upsideDownPixels, OUT_ bytesPerPixel, OUT_ convertSRGB))
					{
						System::PixelUtils::remove_alpha(resolution, upsideDownPixels, REF_ bytesPerPixel);

						if (convertSRGB)
						{
							System::PixelUtils::convert_linear_to_srgb(resolution, upsideDownPixels, bytesPerPixel);
						}

						IO::MemoryStorage* data = new IO::MemoryStorage();

						{
							::System::ImageSaver::TGA tgaSaver;
							tgaSaver.set_bytes_per_pixel(bytesPerPixel);
							tgaSaver.set_size(resolution);
							memory_copy(&tgaSaver.access_data()[0], upsideDownPixels.get_data(), upsideDownPixels.get_size());
							tgaSaver.save(data, true /* data is upside down */);
						}

						parts.push_back(Part(i, true, data));
					}
				}

#ifdef WITH_DRAWING_BOARD
				// drawing board, if present
				if (auto* game = Game::get_as<Game>())
				{
					if (game->read_drawing_board_pixel_data(resolution, upsideDownPixels, OUT_ bytesPerPixel, OUT_ convertSRGB))
					{
						System::PixelUtils::remove_alpha(resolution, upsideDownPixels, REF_ bytesPerPixel);

						if (convertSRGB)
						{
							System::PixelUtils::convert_linear_to_srgb(resolution, upsideDownPixels, bytesPerPixel);
						}

						IO::MemoryStorage* data = new IO::MemoryStorage();

						{
							::System::ImageSaver::TGA tgaSaver;
							tgaSaver.set_bytes_per_pixel(bytesPerPixel);
							tgaSaver.set_size(resolution);
							memory_copy(&tgaSaver.access_data()[0], upsideDownPixels.get_data(), upsideDownPixels.get_size());
							tgaSaver.save(data, true /* data is upside down */);
						}

						parts.push_back(Part(String(TXT("drawing_board")), true, data));
					}
				}
#endif
			}

			for_every(part, parts)
			{
				int partIdx = for_everys_index(part);
				int partCount = parts.get_size();

				if (_update_info)
				{
					_update_info(String::printf(TXT("compressing data [%i/%i]"), partIdx + 1, partCount));
				}

				{
#ifdef OUTPUT_REPORT_BUG_INFO
					output(TXT("compress part %i (%iB)"), for_everys_index(part), part->data->get_length());
#endif

					IO::MemoryStorage* memCompressed = new IO::MemoryStorage();
					part->data->go_to(0);
					if (Compression::compress(*part->data, *memCompressed))
					{
#ifdef OUTPUT_REPORT_BUG_INFO
						output(TXT("compressed part %i (%iB)"), for_everys_index(part), memCompressed->get_length());
#endif
						memCompressed->go_to(0);

						Array<char8> asChar8;
						{
							Array<char8> buffer;
							int bufferSize = 4096;
							while (true)
							{
								buffer.set_size(bufferSize);
								int readSize = memCompressed->read(buffer.get_data(), bufferSize);
								if (readSize > 0)
								{
									buffer.set_size(readSize);
									asChar8.add_from(buffer);
								}
								else
								{
									break;
								}
							}
						}
						if (_update_info)
						{
							_update_info(String::printf(TXT("encoding data [%i/%i]"), partIdx + 1, partCount));
						}

						String contentEncoded = IO::Base64::encode(asChar8.get_data(), asChar8.get_size());

#ifdef OUTPUT_REPORT_BUG_INFO
						output(TXT("encoded part %i (%iB to %iB)"), for_everys_index(part), asChar8.get_size(), contentEncoded.get_length());
#endif

#ifdef OUTPUT_REPORT_BUG_INFO
						output(TXT("send part %i"), for_everys_index(part));
#endif

						if (_update_info)
						{
							_update_info(String::printf(TXT("sending data [%i/%i]"), partIdx + 1, partCount));
						}

						RefCountObjectPtr<JSONDoc> msgAddAttachment(new JSONDoc());
						auto* fileElement = msgAddAttachment->add_array_element(TXT("files"));
						String sendName;
						if (!part->specialName.is_empty())
						{
							if (part->image)
							{
								sendName = String::printf(TXT("log_%07i%S_%S.tga.bz2"), issueId.get(), logNameSuffix.to_char(), part->specialName.to_char());
							}
							else
							{
								sendName = String::printf(TXT("log_%07i%S_%S.txt.bz2"), issueId.get(), logNameSuffix.to_char(), part->specialName.to_char());
							}
						}
						else
						{
							if (part->image)
							{
								sendName = String::printf(TXT("log_%07i%S_%02i.tga.bz2"), issueId.get(), logNameSuffix.to_char(), part->idx);
							}
							else
							{
								sendName = requiresParts ? String::printf(TXT("log_%07i%S_%02i.txt.bz2"), issueId.get(), logNameSuffix.to_char(), part->idx) : String::printf(TXT("log_%07i%S.txt.bz2"), issueId.get(), logNameSuffix.to_char());
							}
						}
						fileElement->set(TXT("name"), sendName);
						fileElement->set(TXT("content"), contentEncoded);

						// if there's a problem sending a log, just ignore it
						RefCountObjectPtr<JSONDoc> response = Framework::REST::send(TXT("http://bugs.voidroom.com/api"), String::printf(TXT("/rest/issues/%i/files"), issueId.get()).to_char(), msgAddAttachment.get(),
							[_update_info, partIdx, partCount](float _progress)
							{
								if (_update_info)
								{
									_update_info(String::printf(TXT("sending data [%i/%i] (%.0f%%)"), partIdx + 1, partCount, _progress * 100.0f));
								}
							});
						if (response.is_set())
						{
#ifdef OUTPUT_REPORT_BUG_INFO
							output(TXT(" response ok"));
							output(TXT("%S"), response->to_string().to_char());
#endif
							// ok
						}
						response.clear();

#ifdef OUTPUT_REPORT_BUG_INFO
						output(TXT("sent part %i"), for_everys_index(part));
#endif
					}
				}
			}

			for_every(part, parts)
			{
				delete_and_clear(part->data);
			}
		}
	}

	if (_update_info)
	{
		_update_info(String(TXT("raport sent")));
	}

	return sent;
}
