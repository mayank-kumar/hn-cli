/**
 * @file display_manager.h
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

#include <condition_variable>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>
#include "interact.h"
#include "story.h"


namespace hackernewscmd {
	/**
	 * Contains type of action, and the data needed to perform that operation
	 */
	struct DisplayThreadData {
		enum Action { DisplayPage, SelectStory, Quit } action;

		struct DisplayPageData {
			std::vector<StoryAndStatus>::const_iterator begin;
			std::vector<StoryAndStatus>::const_iterator end;
		};

		template<Action A, typename T>
		T* GetActionData();

		template<>
		DisplayPageData* GetActionData<DisplayPage>() {
			return static_cast<DisplayPageData*>(mPtr);
		}

		template<>
		Story* GetActionData<SelectStory>() {
			return static_cast<Story*>(mPtr);
		}

		bool redo = true;
		void SetPointer(void* ptr) { mPtr = ptr; }

	private:
		void* mPtr = nullptr;
	}; // struct DisplayThreadData

	/**
	 * Manages display operations.
	 * Consists of a thread that listens on a condition variable.
	 */
	class DisplayManager {
	public:
		DisplayManager(const Interact&);
		~DisplayManager();

		void Go(std::condition_variable&, std::mutex&, DisplayThreadData&, std::condition_variable&);
		void Wait();

	private:
		const Interact& mInteract;
		std::condition_variable *mCV;
		std::condition_variable *mStateManagerCV;
		std::unique_lock<std::mutex> mLock;
		std::thread mDisplayThread;
		DisplayThreadData *mThreadData;
		std::unordered_map<StoryId, StoryDisplayData> mDisplayData;
		const Story *mCurrentlySelectedStory;
		const Story *mToBeSelectedStory;
		const bool mShouldDisplayCommentCount;

		void ThreadCallback();
		bool TryReadNewInstruction();
		bool ShouldBreak() const;
		static std::wstring GetHostNameFromUrl(const std::wstring&);
	}; // class DisplayManager
} // namespace hackernewscmd