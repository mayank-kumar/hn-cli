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
#include <utility>
#include <vector>
#include "story.h"


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
		RefreshStories,
		Quit
	};

	struct StoryDisplayData {
		SMALL_RECT margin;
		SMALL_RECT text;
	};

	class Interact {
		struct Key{};
	public:
		Interact(const Key&) : Interact() {};
		~Interact();
		Interact(const Interact&) = delete;
		Interact& operator=(const Interact&) = delete;

		StoryDisplayData ShowStory(const Story&) const;
		void SwapSelectedStories(const StoryDisplayData&, const StoryDisplayData&) const;
		void ClearScreen() const;
		std::wstring ReadChars() const;
		std::vector<InputAction> ReadActions() const;

		static const Interact& GetInstance();
	private:
		Interact();
		void Init();

		HANDLE mInputHandle;
		HANDLE mOutputHandle;
		HANDLE mOriginalOutputHandle;
		COORD mBufferSize;
		unsigned short mBufferAttributes;
		static std::unique_ptr<Interact> mInstance;
	}; // class Interact
} // namespace hackernewscmd