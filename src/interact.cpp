/**
 * @file interact.cpp
 *
 * Copyright 2015 Mayank Kumar
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include "interact.h"
#include <algorithm>
#include <cstdlib>
#include <stdexcept>

#undef min


namespace hackernewscmd {
	Interact::Interact() : mInputHandle(NULL), mOutputHandle(NULL), mOriginalOutputHandle(NULL) {}

	Interact::~Interact() {
		::SetConsoleActiveScreenBuffer(mOriginalOutputHandle);
		::CloseHandle(mInputHandle);
		::CloseHandle(mOutputHandle);
	};

	void Interact::Init() {
		if ((mInputHandle = ::CreateFileW(L"CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE) {
			throw std::runtime_error("Couldn't open console input handle");
		}
		unsigned long inputModeClearMask = 0xfffffffd;
		unsigned long currentMode;
		::GetConsoleMode(mInputHandle, &currentMode);
		::SetConsoleMode(mInputHandle, currentMode & inputModeClearMask);

		if ((mOriginalOutputHandle = ::CreateFileW(L"CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE) {
			throw std::runtime_error("Couldn'e open console output handle");
		}
		if ((mOutputHandle = ::CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CONSOLE_TEXTMODE_BUFFER, NULL)) == INVALID_HANDLE_VALUE) {
			throw std::runtime_error("Couldn't create new screen buffer");
		}
		if (::SetConsoleActiveScreenBuffer(mOutputHandle) == 0) {
			throw std::runtime_error("Couldn't set active screen buffer");
		}
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		if (::GetConsoleScreenBufferInfo(mOutputHandle, &csbi) == 0) {
			throw std::runtime_error("Couldn't load screen buffer info");
		}
		mBufferSize = csbi.dwSize;
		mBufferAttributes = csbi.wAttributes;
	}

	StoryDisplayData Interact::ShowStory(const Story& story) const {
		StoryDisplayData sdd;

		CONSOLE_SCREEN_BUFFER_INFO csbi;
		if (::GetConsoleScreenBufferInfo(mOutputHandle, &csbi) == 0) {
			throw std::runtime_error("Couldn't load screen buffer info");
		}

		auto currentY = sdd.margin.Top = sdd.text.Top = csbi.dwCursorPosition.Y;
		sdd.margin.Left = 0;
		sdd.margin.Right = 2;
		sdd.text.Left = 3;
		sdd.text.Right = mBufferSize.X;

		auto textWidth = mBufferSize.X - 2;
		for (std::size_t i = 0; i < story.title.length(); i += textWidth, ++currentY) {
			auto length = std::min(i + textWidth, story.title.length()) - i;
			unsigned long charsWritten;
			if (::WriteConsoleOutputCharacterW(mOutputHandle, story.title.c_str() + i, length, { 2, currentY }, &charsWritten) == 0) {
				throw std::runtime_error("Couldn't write characters");
			}
		}

		sdd.margin.Bottom = sdd.text.Bottom = currentY - 1;

		::SetConsoleCursorPosition(mOutputHandle, { 0, currentY + 1 });

		return sdd;
	}

	void Interact::SwapSelectedStories(const StoryDisplayData& prev, const StoryDisplayData& curr) const {
		unsigned long charsWritten;
		if (::WriteConsoleOutputCharacterW(mOutputHandle, L" ", 1, { prev.margin.Left, prev.margin.Top }, &charsWritten) == 0
			|| ::WriteConsoleOutputCharacterW(mOutputHandle, L"*", 1, { curr.margin.Left, curr.margin.Top }, &charsWritten) == 0) {
			throw std::runtime_error("Couldn't write character");
		}
	}

	void Interact::ClearScreen() const {
		COORD root{ 0, 0 };
		auto bufferSize = mBufferSize.X * mBufferSize.Y;
		unsigned long charsWritten;
		if (::FillConsoleOutputCharacterW(mOutputHandle, L' ', bufferSize, root, &charsWritten) == 0) {
			throw std::runtime_error("Couldn't fill screen with blanks");
		}
		if (::FillConsoleOutputAttribute(mOutputHandle, mBufferAttributes, bufferSize, root, &charsWritten) == 0) {
			throw std::runtime_error("Couldn't set attributes for screen buffer");
		}
		::SetConsoleCursorPosition(mOutputHandle, root);
	}

	std::wstring Interact::ReadChars() const {
		const unsigned long numberOfChars = 4096;
		wchar_t buffer[numberOfChars];
		unsigned long charsRead;
		std::wstring result;
		do {
			if (::ReadConsoleW(mInputHandle, buffer, numberOfChars, &charsRead, NULL) == 0) {
				throw std::runtime_error("Couldn't read ");
			}
			result.append(buffer, buffer + charsRead);
		} while (charsRead == 4096);
		return result;
	}

	std::vector<InputAction> Interact::ReadActions() const {
		std::vector<InputAction> actions;

		auto insertAtEnd = [&actions](const KEY_EVENT_RECORD& event, InputAction action) {
			actions.insert(actions.end(), event.wRepeatCount, action);
		};

		const auto bufferSize = 128UL;
		auto eventsRead = 0UL;
		INPUT_RECORD buff[bufferSize];

		while (actions.empty()) {
			if (!::ReadConsoleInputW(mInputHandle, buff, bufferSize, &eventsRead)) {
				throw std::runtime_error("Couldn't read input buffer");
			}

			for (auto i = 0UL; i < eventsRead; ++i) {
				auto& item = buff[i];
				if (item.EventType != KEY_EVENT || !item.Event.KeyEvent.bKeyDown) {
					continue;
				}
				auto& event = item.Event.KeyEvent;
				if (event.uChar.UnicodeChar) {
					switch (::tolower(event.uChar.UnicodeChar)) {
					case '\r':
						insertAtEnd(event, InputAction::OpenStory);
						break;
					case 'n':
						insertAtEnd(event, InputAction::NextStorySkip);
						break;
					case 'p':
						insertAtEnd(event, InputAction::PrevStorySkip);
						break;
					case 'q':
						insertAtEnd(event, InputAction::Quit);
						break;
					}
				} else {
					switch (event.wVirtualKeyCode) {
					case VK_DOWN:
						insertAtEnd(event, InputAction::NextStory);
						break;
					case VK_F5:
						insertAtEnd(event, InputAction::RefreshStories);
						break;
					case VK_LEFT:
						insertAtEnd(event, InputAction::PrevPage);
						break;
					case VK_NEXT:
						insertAtEnd(event, InputAction::NextPageSkip);
						break;
					case VK_PRIOR:
						insertAtEnd(event, InputAction::PrevPageSkip);
						break;
					case VK_RIGHT:
						insertAtEnd(event, InputAction::NextPage);
						break;
					case VK_UP:
						insertAtEnd(event, InputAction::PrevStory);
						break;
					}
				}
			}
		}

		return actions;
	}

	std::unique_ptr<Interact> Interact::mInstance = nullptr;
	const Interact& Interact::GetInstance() {
		if (mInstance == nullptr) {
			mInstance = std::make_unique<Interact>(Key{});
			mInstance->Init();
		}
		return *mInstance;
	}
} // namespace hackernewscmd