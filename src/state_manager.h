/**
 * @file state_manager.h
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
#include <cstddef>
#include <memory>
#include <mutex>
#include <unordered_set>
#include <vector>
#include "display_manager.h"
#include "fetcher.h"
#include "storage.h"
#include "story.h"


namespace hackernewscmd {
	/**
	 * Manages state of the application.
	 * Primarily responsible for handling user inputs and directing the
	 * displaying and fetching components.
	 */
	class StateManager {
		struct Key{};
	public:
		StateManager(const Key&) : StateManager(){};
		void Init(Storage&, NewsFetcher&, DisplayManager&);
		void Start();
		void GotoNextPage(bool);
		void GotoPrevPage(bool);
		void SelectNextStory(bool);
		void SelectPrevStory(bool);
		void OpenSelectedStory(bool);
		void Quit();
		static StateManager& GetInstance();

	private:
		StateManager();
		bool mIsInited;

		std::vector<StoryAndStatus> mPagedDisplayBuffer;
		long mCurrentDisplayPage;
		std::size_t mCurrentSelectedStoryIndex;
		std::vector<StoryId> mTopStories;
		std::unordered_set<StoryId> *mSkippedStories;

		std::condition_variable mDisplayCV;
		std::mutex mDisplayMutex;
		std::unique_lock<std::mutex> mDisplayLock;
		std::condition_variable mDisplayReverseCV;
		std::mutex mDisplayReverseMutex;
		std::unique_lock<std::mutex> mDisplayReverseLock;
		DisplayThreadData mDisplayThreadData;
		DisplayThreadData::DisplayPageData mDisplayPageData;

		Storage* mStorage;
		NewsFetcher* mFetcher;
		DisplayManager* mDisplayManager;

		void LoadFromStorage();
		void FetchTopStories();
		void DiffTopStories();
		void GotoPage(const long, const bool);
		void SelectStory(const std::size_t, const bool);

		static std::wstring GetStoryPageUrl(const Story&);

		using PageIndices = std::pair < std::size_t, std::size_t >;
		void FetchDisplayPage(const PageIndices&);
		void DisplayPage(const PageIndices&);
		void SetupDisplayThreadDataForPageDisplay(const PageIndices&);
		bool TryGetIndicesForDisplayPage(long, PageIndices&) const;
		void SetupDisplayThreadDataForSelectedStory(const std::size_t);
		void OnFetchStoryComplete(Story, size_t);

		static std::unique_ptr<StateManager> mInstance;
		static const std::size_t kDisplayPageSize = 10;
		static const std::wstring kHackerNewsItemUrl;
	}; // class SateManager
} // hnamespace hackernewscmd