/**
 * @file interact.h
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


#pragma once

#include <Windows.h>
#include <memory>
#include <string>
#include <utility>
#include <vector>


namespace hackernewscmd {
	enum class InputAction {
		Unknown,

		NextStory,
		NextStorySkip,
		PrevStory,
		PrevStorySkip,

		NextPage,
		NextPageSkip,
		PrevPage,
		PrevPageSkip,

		OpenStory,
		OpenStoryPage,
		RefreshStories,
		Quit
	};

	struct StoryDisplayData {
		SMALL_RECT margin;
		SMALL_RECT text;
		SMALL_RECT addendum;
	};

	class Interact {
		struct Key{};
	public:
		Interact(const Key&) : Interact() {};
		~Interact();
		Interact(const Interact&) = delete;
		Interact& operator=(const Interact&) = delete;

		// Without comments
		StoryDisplayData ShowStory(const std::wstring&, const unsigned, const std::wstring&) const;
		// With comments
		StoryDisplayData ShowStory(const std::wstring&, const unsigned, const std::wstring&, const unsigned) const;

		void ShowPagePosition(long currentPage, long totalPages) const;
		StoryDisplayData ShowFailedStory() const;

		void SwapSelectedStories(const StoryDisplayData&, const StoryDisplayData&) const;
		void ClearScreen() const;
		std::wstring ReadChars() const;
		std::vector<InputAction> ReadActions() const;

		static const Interact& GetInstance();
	private:
		Interact();
		void Init();
		StoryDisplayData ShowStoryInternal(const std::wstring&, const unsigned, const std::wstring&, const long) const;
		short PrintLineWithinCols(const std::wstring&, short, short, short, bool = false) const;
		void ChangeBufferAttributes(const SMALL_RECT&, unsigned short) const;
		StoryDisplayData GetStoryDisplayDataStartingAtNextRow() const;

		HANDLE mInputHandle;
		HANDLE mOutputHandle;
		HANDLE mOriginalOutputHandle;
		mutable COORD mBufferSize;
		unsigned short mBufferAttributes;
		unsigned short mSelectedStoryAttributes;
		mutable short mNextRow;
		mutable std::vector<CHAR_INFO> mRowBuffer;
		static std::unique_ptr<Interact> mInstance;
	}; // class Interact
} // namespace hackernewscmd